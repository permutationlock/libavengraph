#ifndef AVEN_GRAPH_PLANE_POH_THREAD_H
#define AVEN_GRAPH_PLANE_POH_THREAD_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/thread_pool.h>

#if !defined(__STDC_VERSION__) or __STDC_VERSION__ < 201112L
    #error "C11 or later is required"
#endif

#include <stdatomic.h>

#include "../../../graph.h"
#include "../poh.h"

typedef struct {
    AvenGraphPlanePohFrame *ptr;
    atomic_size_t len;
    size_t cap;
} AvenGraphPlanePohThreadFrameList;

typedef struct {
    AvenGraph graph;
    AvenGraphPropUint8 coloring;
    Slice(int32_t) marks;
    AvenGraphPlanePohThreadFrameList frames;
    atomic_int threads_active;
    atomic_int frames_active;
    atomic_bool frames_lock;
} AvenGraphPlanePohThreadCtx;

static inline AvenGraphPlanePohThreadCtx aven_graph_plane_poh_thread_init(
    AvenGraph graph,
    AvenGraphSubset p,
    AvenGraphSubset q,
    AvenArena *arena
) {
    uint32_t p1 = get(p, 0);
    uint32_t q1 = get(q, 0);

    AvenGraphPlanePohThreadCtx ctx = {
        .graph = graph,
        .marks = { .len = graph.len },
        .frames = { .cap = graph.len - 2 },
    };

    ctx.marks.ptr = aven_arena_create_array(
        int32_t,
        arena,
        ctx.marks.len
    );
    ctx.frames.ptr = aven_arena_create_array(
        AvenGraphPlanePohFrame,
        arena,
        ctx.frames.cap
    );

    atomic_init(&ctx.frames.len, 0);
    atomic_init(&ctx.frames_active, 0);
    atomic_init(&ctx.frames_lock, false);
    atomic_init(&ctx.threads_active, 0);

    for (uint32_t i = 0; i < ctx.marks.len; i += 1) {
        get(ctx.marks, i) = 0;
    }

    for (uint32_t i = 0; i < p.len; i += 1) {
        get(ctx.marks, get(p, i)) = -1;
    }

    get(ctx.marks, get(p, 0)) = 1;

    for (uint32_t i = 0; i < q.len; i += 1) {
        get(ctx.marks, get(q, i)) = 2;
    }

    list_push(ctx.frames) = (AvenGraphPlanePohFrame){
        .p_color = 3,
        .q_color = 2,
        .u = p1,
        .u_nb_first = aven_graph_neighbor_index(
            graph,
            p1,
            q1
        ),
        .x = p1,
        .y = p1,
        .z = p1,
        .face_mark = -1,
    };

    return ctx;
}

static inline void aven_graph_plane_poh_thread_lock_internal(
    AvenGraphPlanePohThreadCtx *ctx
) {    
    for (;;) {
        if (
            !atomic_exchange_explicit(
                &ctx->frames_lock,
                true,
                memory_order_acquire
            )
        ) {
            return;
        }
        while (atomic_load_explicit(&ctx->frames_lock, memory_order_relaxed)) {
#if __has_builtin(__builtin_ia32_pause)
            __builtin_ia32_pause();
#endif
        }
    }
}

static inline void aven_graph_plane_poh_thread_unlock_internal(
    AvenGraphPlanePohThreadCtx *ctx
) {
    atomic_store_explicit(&ctx->frames_lock, false, memory_order_release);
}

static inline void aven_graph_plane_poh_thread_push_internal(
    AvenGraphPlanePohThreadCtx *ctx,
    AvenGraphPlanePohFrame frame
) {
    aven_graph_plane_poh_thread_lock_internal(ctx);
    size_t len = atomic_fetch_add_explicit(
        &ctx->frames.len,
        1,
        memory_order_relaxed
    );
    assert(len < ctx->frames.cap);
    ctx->frames.ptr[len] = frame;
    aven_graph_plane_poh_thread_unlock_internal(ctx);
}

static inline bool aven_graph_plane_poh_thread_frame_step(
    AvenGraphPlanePohThreadCtx *ctx,
    AvenGraphPlanePohFrame *frame
) {
    uint8_t path_color = frame->p_color ^ frame->q_color;

    AvenGraphAdjList u_adj = get(ctx->graph, frame->u);

    if (frame->edge_index == u_adj.len) {
        if (frame->z != frame->u) {
            aven_graph_plane_poh_thread_push_internal(
                ctx,
                (AvenGraphPlanePohFrame){
                    .q_color = path_color,
                    .p_color = frame->p_color,
                    .u = frame->z,
                    .x = frame->z,
                    .y = frame->z,
                    .z = frame->z,
                    .face_mark = frame->face_mark - 1,
                }
            );
        }

        if (frame->y == frame->u) {
            assert(frame->x == frame->u);
            return true;
        }

        if (frame->x == frame->u) {
            frame->x = frame->y;
        }

        frame->u_nb_first = aven_graph_next_neighbor_index(
            ctx->graph,
            frame->y,
            frame->u
        );
        frame->u = frame->y;
        frame->z = frame->y;
        frame->edge_index = 0;
        frame->above_path = false;
        frame->last_colored = false;
        return false;
    }

    uint32_t n_index = frame->u_nb_first + frame->edge_index;
    if (n_index >= u_adj.len) {
        n_index -= (uint32_t)u_adj.len;
    }

    uint32_t n = get(u_adj, n_index);
    int32_t n_mark = get(ctx->marks, n);

    frame->edge_index += 1;

    if (frame->above_path) {
        if (n_mark <= 0) {
            if (frame->last_colored) {
                frame->z = n;
                get(ctx->marks, n) = (int32_t)frame->q_color;
            } else {
                get(ctx->marks, n) = frame->face_mark - 1;
            }
            frame->last_colored = false;
        } else {
            frame->last_colored = true;
            if (frame->z != frame->u) {
                aven_graph_plane_poh_thread_push_internal(
                    ctx,
                    (AvenGraphPlanePohFrame){
                        .p_color = path_color,
                        .q_color = frame->p_color,
                        .u = frame->z,
                        .u_nb_first = aven_graph_next_neighbor_index(
                            ctx->graph,
                            frame->z,
                            frame->u
                        ),
                        .x = frame->z,
                        .y = frame->z,
                        .z = frame->z,
                        .face_mark = frame->face_mark - 1,
                    }
                );
                frame->z = frame->u;
            }
        }
    } else if (n != frame->x) {
        if (n_mark > 0) {
            if (n_mark == (int32_t)frame->p_color) {
                frame->above_path = true;
                frame->last_colored = true;
            }
            if (frame->x != frame->u) {
                aven_graph_plane_poh_thread_push_internal(
                    ctx,
                    (AvenGraphPlanePohFrame){
                        .p_color = path_color,
                        .q_color = frame->q_color,
                        .u = frame->x,
                        .u_nb_first = frame->x_nb_first,
                        .x = frame->x,
                        .y = frame->x,
                        .z = frame->x,
                        .face_mark = frame->face_mark - 1,
                    }
                );

                frame->x = frame->u;
            }
        } else if (n_mark == frame->face_mark) {
            get(ctx->marks, n) = (int32_t)path_color;
            frame->y = n;
            frame->above_path = true;
        } else {
            if (n_mark <= 0) {
                get(ctx->marks, n) = frame->face_mark - 1;
            }

            if (frame->x == frame->u) {
                frame->x = n;
                frame->x_nb_first = aven_graph_next_neighbor_index(
                    ctx->graph,
                    n,
                    frame->u
                );

                get(ctx->marks, n) = (int32_t)frame->p_color;
            }
        }
    }

    return false;
}

static inline AvenGraphPlanePohFrameOptional
aven_graph_plane_poh_pop_internal(
    AvenGraphPlanePohThreadCtx *ctx
) {
    atomic_fetch_sub_explicit(&ctx->frames_active, 1, memory_order_relaxed);
    for (;;) {
        if (
            atomic_load_explicit(&ctx->frames.len, memory_order_relaxed) > 0 or
            atomic_load_explicit(&ctx->frames_active, memory_order_relaxed) == 0
        ) {
            aven_graph_plane_poh_thread_lock_internal(ctx);
            if (
                atomic_load_explicit(
                    &ctx->frames.len,
                    memory_order_relaxed
                ) != 0
            ) {
                size_t frame_index = atomic_fetch_sub_explicit(
                    &ctx->frames.len,
                    1,
                    memory_order_relaxed
                ) - 1;
                AvenGraphPlanePohFrameOptional frame = {
                    .value = ctx->frames.ptr[frame_index],
                    .valid = true,
                };
                atomic_fetch_add_explicit(
                    &ctx->frames_active,
                    1,
                    memory_order_relaxed
                );
                aven_graph_plane_poh_thread_unlock_internal(ctx);
                return frame;
            } else if (
                atomic_load_explicit(
                    &ctx->frames_active,
                    memory_order_relaxed
                ) == 0
            ) {
                aven_graph_plane_poh_thread_unlock_internal(ctx);
                return (AvenGraphPlanePohFrameOptional){ 0 };
            }
            aven_graph_plane_poh_thread_unlock_internal(ctx);
        }
        while (
            atomic_load_explicit(
                &ctx->frames.len,
                memory_order_relaxed
            ) == 0 and
            atomic_load_explicit(&ctx->frames_active, memory_order_relaxed) > 0
        ) {
#if __has_builtin(__builtin_ia32_pause)
            __builtin_ia32_pause();
#endif
        }
    }
}

typedef struct {
    AvenGraphPropUint8 coloring;
    AvenGraphPlanePohThreadCtx *ctx;
    uint32_t start_vertex;
    uint32_t end_vertex;
} AvenGraphPohThreadWorkerArgs;

static void aven_graph_poh_thread_worker_internal(void *args) {
    AvenGraphPohThreadWorkerArgs *wargs = args;
    AvenGraphPlanePohThreadCtx *ctx = wargs->ctx;

    atomic_fetch_add_explicit(&ctx->threads_active, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&ctx->frames_active, 1, memory_order_relaxed);

    AvenGraphPlanePohFrameOptional cur_frame =
        aven_graph_plane_poh_pop_internal(ctx);

    while (cur_frame.valid) {
        while (
            !aven_graph_plane_poh_thread_frame_step(ctx, &cur_frame.value)
        ) {}

        cur_frame = aven_graph_plane_poh_pop_internal(ctx);
    }


    atomic_fetch_sub_explicit(&ctx->threads_active, 1, memory_order_release);

    for (;;) {
        int threads_active = atomic_load_explicit(
            &ctx->threads_active,
            memory_order_acquire
        );
        if (threads_active == 0) {
            break;
        }
        while (threads_active != 0) {
            threads_active = atomic_load_explicit(
                &ctx->threads_active,
                memory_order_relaxed
            );
        }
    }

    for (uint32_t v = wargs->start_vertex; v != wargs->end_vertex; v += 1) {
        assert(get(ctx->marks, v) > 0);
        get(wargs->coloring, v) = (uint8_t)get(ctx->marks, v);        
    }
}

static inline AvenGraphPropUint8 aven_graph_plane_poh_thread(
    AvenGraph graph,
    AvenGraphSubset p,
    AvenGraphSubset q,
    AvenThreadPool *thread_pool,
    AvenArena *arena
) {
    AvenGraphPropUint8 coloring = { .len = graph.len };
    coloring.ptr = aven_arena_create_array(uint8_t, arena, coloring.len);

    AvenArena temp_arena = *arena;
    
    AvenGraphPlanePohThreadCtx ctx = aven_graph_plane_poh_thread_init(
        graph,
        p,
        q,
        &temp_arena
    );

    Slice(AvenGraphPohThreadWorkerArgs) wargs_data = aven_arena_create_slice(
        AvenGraphPohThreadWorkerArgs,
        &temp_arena,
        thread_pool->workers.len + 1
    );

    uint32_t chunk_size = (uint32_t)(graph.len / wargs_data.len);
    for (uint32_t i = 0; i < wargs_data.len; i += 1) {
        uint32_t start_vertex = i * chunk_size;
        uint32_t end_vertex = (i + 1) * chunk_size;
        if (i + 1 == wargs_data.len) {
            end_vertex = (uint32_t)graph.len;
        }

        get(wargs_data, i) = (AvenGraphPohThreadWorkerArgs){
            .coloring = coloring,
            .ctx = &ctx,
            .start_vertex = start_vertex,
            .end_vertex = end_vertex,
        };
    }

    for (size_t i = 0; i < thread_pool->workers.len; i += 1) {
        aven_thread_pool_submit(
            thread_pool,
            aven_graph_poh_thread_worker_internal,
            &get(wargs_data, i)
        );
    }

    aven_graph_poh_thread_worker_internal(&get(wargs_data, wargs_data.len - 1));

    aven_thread_pool_wait(thread_pool);

    return coloring;
}

#endif // AVEN_GRAPH_PLANE_POH_THREAD_H
