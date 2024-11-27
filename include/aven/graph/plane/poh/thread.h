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
    uint32_t u;
    uint32_t w;
    uint32_t t;
    uint32_t x;
    uint32_t edge_index;
    uint32_t face_mark;
    uint8_t q_color;
    uint8_t p_color;
    bool above_path;
    bool last_colored;
} AvenGraphPlanePohThreadFrame;
typedef Optional(AvenGraphPlanePohThreadFrame)
    AvenGraphPlanePohThreadFrameOptional;

typedef struct {
    AvenGraphPlanePohThreadFrame *ptr;
    atomic_size_t len;
    size_t cap;
} AvenGraphPlanePohThreadFrameList;

typedef struct {
    AvenGraph graph;
    AvenGraphPropUint8 coloring;
    Slice(AvenGraphPlanePohVertex) vertex_info;
    AvenGraphPlanePohThreadFrameList frames;
    atomic_int frames_active;
    atomic_bool frames_lock;
} AvenGraphPlanePohThreadCtx;

static inline AvenGraphPlanePohThreadCtx aven_graph_plane_poh_thread_init(
    AvenGraphPropUint8 coloring,
    AvenGraph graph,
    AvenGraphSubset p,
    AvenGraphSubset q,
    AvenArena *arena
) {
    uint32_t p1 = slice_get(p, 0);
    uint32_t q1 = slice_get(q, 0);

    AvenGraphPlanePohThreadCtx ctx = {
        .graph = graph,
        .coloring = coloring,
        .vertex_info = { .len = graph.len },
        .frames = { .cap = graph.len - 2 },
    };

    for (uint32_t i = 0; i < ctx.coloring.len; i += 1) {
        slice_get(ctx.coloring, i) = 0;
    }

    slice_get(ctx.coloring, slice_get(p, 0)) = 1;

    for (uint32_t i = 0; i < q.len; i += 1) {
        slice_get(ctx.coloring, slice_get(q, i)) = 2;
    }

    ctx.vertex_info.ptr = aven_arena_create_array(
        AvenGraphPlanePohVertex,
        arena,
        ctx.vertex_info.len
    );

    for (uint32_t i = 0; i < ctx.vertex_info.len; i += 1) {
        slice_get(ctx.vertex_info, i) = (AvenGraphPlanePohVertex){ 0 };
    }

    for (uint32_t i = 0; i < p.len; i += 1) {
        slice_get(ctx.vertex_info, slice_get(p, i)).mark = 1;
    }

    slice_get(ctx.vertex_info, p1).first_edge = aven_graph_neighbor_index(
        graph,
        p1,
        q1
    );

    ctx.frames.ptr = aven_arena_create_array(
        AvenGraphPlanePohThreadFrame,
        arena,
        ctx.frames.cap
    );

    list_push(ctx.frames) = (AvenGraphPlanePohThreadFrame){
        .p_color = 3,
        .q_color = 2,
        .u = p1,
        .w = p1,
        .t = p1,
        .x = p1,
        .face_mark = 1,
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
    AvenGraphPlanePohThreadFrame frame
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
    AvenGraphPlanePohThreadFrame *frame
) {
    uint8_t path_color = frame->p_color ^ frame->q_color;

    AvenGraphAdjList u_adj = slice_get(ctx->graph, frame->u);

    if (frame->edge_index == u_adj.len) {
        if (frame->x != frame->u) {
            aven_graph_plane_poh_thread_push_internal(
                ctx,
                (AvenGraphPlanePohThreadFrame){
                    .p_color = path_color,
                    .q_color = frame->p_color,
                    .u = frame->x,
                    .w = frame->x,
                    .t = frame->x,
                    .x = frame->x,
                    .face_mark = frame->face_mark + 1,
                }
            );
        }

        if (frame->t == frame->u) {
            assert(frame->w == frame->u);
            return true;
        }

        if (frame->w == frame->u) {
            frame->w = frame->t;
        }
        frame->u = frame->t;
        frame->x = frame->t;
        frame->edge_index = 0;
        frame->above_path = false;
        frame->last_colored = false;
        return false;
    }

    uint32_t n_index = slice_get(ctx->vertex_info, frame->u).first_edge +
        frame->edge_index;
    if (n_index >= u_adj.len) {
        n_index -= (uint32_t)u_adj.len;
    }

    uint32_t n = slice_get(u_adj, n_index);
    uint32_t n_color = slice_get(ctx->coloring, n);
    AvenGraphPlanePohVertex *n_info = &slice_get(ctx->vertex_info, n);

    frame->edge_index += 1;

    if (frame->above_path) {
        if (n_color == 0) {
            if (frame->last_colored) {
                frame->x = n;
                slice_get(ctx->coloring, n) = frame->q_color;
                n_info->first_edge = aven_graph_next_neighbor_index(
                    ctx->graph,
                    n,
                    frame->u
                );
            }
            n_info->mark = frame->face_mark + 1;
            frame->last_colored = false;
        } else {
            frame->last_colored = true;
            if (frame->x != frame->u) {
                aven_graph_plane_poh_thread_push_internal(
                    ctx,
                    (AvenGraphPlanePohThreadFrame){
                        .p_color = path_color,
                        .q_color = frame->p_color,
                        .u = frame->x,
                        .w = frame->x,
                        .t = frame->x,
                        .x = frame->x,
                        .face_mark = frame->face_mark + 1,
                    }
                );
                frame->x = frame->u;
            }
        }
    } else if (n != frame->w) {
        if (n_color != 0) {
            if (n_color == frame->p_color) {
                frame->above_path = true;
                if (n_color == frame->p_color) {
                    frame->last_colored = true;
                }
            }
            if (frame->w != frame->u) {
                aven_graph_plane_poh_thread_push_internal(
                    ctx,
                    (AvenGraphPlanePohThreadFrame){
                        .q_color = frame->q_color,
                        .p_color = path_color,
                        .u = frame->w,
                        .w = frame->w,
                        .t = frame->w,
                        .x = frame->w,
                        .face_mark = frame->face_mark + 1,
                    }
                );

                frame->w = frame->u;
            }
        } else if (n_info->mark == frame->face_mark) {
            slice_get(ctx->coloring, n) = path_color;
            n_info->first_edge = aven_graph_next_neighbor_index(
                ctx->graph,
                n,
                frame->u
            );

            frame->t = n;
            frame->above_path = true;
        } else {
            if (n_color == 0) {
                n_info->mark = frame->face_mark + 1;
            }

            if (frame->w == frame->u) {
                frame->w = n;

                slice_get(ctx->coloring, n) = frame->p_color;
                n_info->first_edge = aven_graph_next_neighbor_index(
                    ctx->graph,
                    n,
                    frame->u
                );
            }
        }
    }

    return false;
}

static inline AvenGraphPlanePohThreadFrameOptional
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
                AvenGraphPlanePohThreadFrameOptional frame = {
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
                return (AvenGraphPlanePohThreadFrameOptional){ 0 };
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

static void aven_graph_poh_thread_worker_internal(void *args) {
    AvenGraphPlanePohThreadCtx *ctx = args;

    atomic_fetch_add_explicit(&ctx->frames_active, 1, memory_order_relaxed);

    AvenGraphPlanePohThreadFrameOptional cur_frame =
        aven_graph_plane_poh_pop_internal(ctx);

    while (cur_frame.valid) {
        while (
            !aven_graph_plane_poh_thread_frame_step(ctx, &cur_frame.value)
        ) {}

        cur_frame = aven_graph_plane_poh_pop_internal(ctx);
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
        coloring,
        graph,
        p,
        q,
        &temp_arena
    );

    for (size_t i = 0; i < thread_pool->workers.len; i += 1) {
        aven_thread_pool_submit(
            thread_pool,
            aven_graph_poh_thread_worker_internal,
            &ctx
        );
    }

    aven_graph_poh_thread_worker_internal(&ctx);

    aven_thread_pool_wait(thread_pool);

    return coloring;
}

#endif // AVEN_GRAPH_PLANE_POH_THREAD_H
