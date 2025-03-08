#ifndef GRAPH_PLANE_P3COLOR_THREAD_H
#define GRAPH_PLANE_P3COLOR_THREAD_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/thread/pool.h>
#include <aven/thread/spinlock.h>

#if !defined(__STDC_VERSION__) or __STDC_VERSION__ < 201112L
    #error "C11 or later is required"
#endif

#include <stdatomic.h>

#include "../../../graph.h"
#include "../p3color.h"

typedef List(GraphPlaneP3ColorFrame) GraphPlaneP3ColorThreadFrameList;
typedef struct {
    GraphPlaneP3ColorFrame *ptr;
    atomic_size_t len;
    size_t cap;
} GraphPlaneP3ColorThreadAtomicFrameList;

typedef struct {
    GraphNbSlice nb;
    Slice(GraphPlaneP3ColorVertex) vertex_info;
    GraphPlaneP3ColorThreadAtomicFrameList frames;
    atomic_int threads_active;
    atomic_int frames_active;
    AvenThreadSpinlock lock;
} GraphPlaneP3ColorThreadCtx;

static inline GraphPlaneP3ColorThreadCtx graph_plane_p3color_thread_init(
    Graph graph,
    GraphSubset p,
    GraphSubset q,
    AvenArena *arena
) {
    uint32_t p1 = get(p, 0);
    uint32_t q1 = get(q, 0);

    GraphPlaneP3ColorThreadCtx ctx = {
        .nb = graph.nb,
        .vertex_info = { .len = graph.adj.len },
        .frames = { .cap = graph.adj.len - 2 },
    };

    ctx.vertex_info.ptr = aven_arena_create_array(
        GraphPlaneP3ColorVertex,
        arena,
        ctx.vertex_info.len
    );
    ctx.frames.ptr = aven_arena_create_array(
        GraphPlaneP3ColorFrame,
        arena,
        ctx.frames.cap
    );

    atomic_init(&ctx.frames.len, 0);
    atomic_init(&ctx.frames_active, 0);
    atomic_init(&ctx.threads_active, 0);
    aven_thread_spinlock_init(&ctx.lock);

    for (uint32_t v = 0; v < ctx.vertex_info.len; v += 1) {
        get(ctx.vertex_info, v) = (GraphPlaneP3ColorVertex){
            .adj = get(graph.adj, v),
        };
    }

    for (uint32_t i = 0; i < p.len; i += 1) {
        get(ctx.vertex_info, get(p, i)).mark = -1;
    }

    get(ctx.vertex_info, p1).mark = 1;

    for (uint32_t i = 0; i < q.len; i += 1) {
        get(ctx.vertex_info, get(q, i)).mark = 2;
    }

    list_push(ctx.frames) = (GraphPlaneP3ColorFrame){
        .p_color = 3,
        .q_color = 2,
        .u = p1,
        .u_nb_first = graph_nb_index(
            graph.nb,
            get(graph.adj, p1),
            q1
        ),
        .x = p1,
        .y = p1,
        .z = p1,
        .face_mark = -1,
    };

    return ctx;
}

static inline void graph_plane_p3color_thread_push_internal(
    GraphPlaneP3ColorThreadCtx *ctx,
    GraphPlaneP3ColorThreadFrameList *local_frames,
    GraphPlaneP3ColorFrame frame
) {
    list_push(*local_frames) = frame;
    if (local_frames->len == local_frames->cap) {
        aven_thread_spinlock_lock(&ctx->lock);
        size_t len = atomic_fetch_add_explicit(
            &ctx->frames.len,
            local_frames->cap / 2,
            memory_order_relaxed
        );
        assert((len + local_frames->cap / 2) <= ctx->frames.cap);
        for (
            size_t i = 0;
            i < local_frames->cap / 2;
            i += 1
        ) {
            ctx->frames.ptr[len + i] = list_pop(*local_frames);
        }
        aven_thread_spinlock_unlock(&ctx->lock);
    }
}

static inline bool graph_plane_p3color_thread_frame_step(
    GraphPlaneP3ColorThreadCtx *ctx,
    GraphPlaneP3ColorThreadFrameList *local_frames,
    GraphPlaneP3ColorFrame *frame
) {
    uint8_t path_color = frame->p_color ^ frame->q_color;

    GraphPlaneP3ColorVertex *u_info = &get(ctx->vertex_info, frame->u);

    if (frame->edge_index == u_info->adj.len) {
        assert(frame->z == frame->u);

        if (frame->y == frame->u) {
            assert(frame->x == frame->u);
            return true;
        }

        if (frame->x == frame->u) {
            frame->x = frame->y;
        }

        GraphPlaneP3ColorVertex *y_info = &get(ctx->vertex_info, frame->y);
        frame->u_nb_first = graph_adj_next(
            y_info->adj,
            graph_nb_index(ctx->nb, y_info->adj, frame->u)
        );
        frame->u = frame->y;
        frame->z = frame->y;
        frame->edge_index = 0;
        frame->above_path = false;
        frame->last_colored = false;
        return false;
    }

    uint32_t n_index = frame->u_nb_first + frame->edge_index;
    if (n_index >= u_info->adj.len) {
        n_index -= u_info->adj.len;
    }

    uint32_t n = graph_nb(ctx->nb, u_info->adj, n_index);
    GraphPlaneP3ColorVertex *n_info = &get(ctx->vertex_info, n);

    frame->edge_index += 1;

    if (frame->above_path) {
        if (n_info->mark <= 0) {
            if (frame->last_colored) {
                frame->z = n;
                n_info->mark = (int32_t)frame->q_color;
            } else {
                n_info->mark = frame->face_mark - 1;
            }
            frame->last_colored = false;
        } else {
            frame->last_colored = true;
            if (frame->z != frame->u) {
                GraphPlaneP3ColorVertex *z_info = &get(
                    ctx->vertex_info,
                    frame->z
                );
                graph_plane_p3color_thread_push_internal(
                    ctx,
                    local_frames,
                    (GraphPlaneP3ColorFrame){
                        .p_color = path_color,
                        .q_color = frame->p_color,
                        .u = frame->z,
                        .u_nb_first = graph_adj_next(
                            z_info->adj,
                            graph_nb_index(ctx->nb, z_info->adj, frame->u)
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
        if (n_info->mark > 0) {
            if (n_info->mark == (int32_t)frame->p_color) {
                frame->above_path = true;
                frame->last_colored = true;
            }
            if (frame->x != frame->u) {
                graph_plane_p3color_thread_push_internal(
                    ctx,
                    local_frames,
                    (GraphPlaneP3ColorFrame){
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
        } else if (n_info->mark == frame->face_mark) {
            n_info->mark = (int32_t)path_color;
            frame->y = n;
            frame->above_path = true;
        } else {
            if (n_info->mark <= 0) {
                n_info->mark = frame->face_mark - 1;
            }

            if (frame->x == frame->u) {
                frame->x = n;
                frame->x_nb_first = graph_adj_next(
                    n_info->adj,
                    graph_nb_index(ctx->nb, n_info->adj, frame->u)
                );

                n_info->mark = (int32_t)frame->p_color;
            }
        }
    }

    return false;
}

static inline void graph_plane_p3color_pop_internal(
    GraphPlaneP3ColorThreadCtx *ctx,
    GraphPlaneP3ColorThreadFrameList *local_frames
) {
    atomic_fetch_sub_explicit(&ctx->frames_active, 1, memory_order_relaxed);
    for (;;) {
        if (
            atomic_load_explicit(&ctx->frames.len, memory_order_relaxed) > 0 or
            atomic_load_explicit(&ctx->frames_active, memory_order_relaxed) == 0
        ) {
            aven_thread_spinlock_lock(&ctx->lock);
            size_t frames_available = atomic_load_explicit(
                &ctx->frames.len,
                memory_order_relaxed
            );
            if (frames_available != 0) {
                size_t frames_moved = min(
                    (local_frames->cap / 2),
                    frames_available
                );
                size_t frame_index = atomic_fetch_sub_explicit(
                    &ctx->frames.len,
                    frames_moved,
                    memory_order_relaxed
                ) - frames_moved;
                for (size_t i = 0; i < frames_moved; i += 1) {
                    list_push(*local_frames) = ctx->frames.ptr[frame_index + i];
                }
                atomic_fetch_add_explicit(
                    &ctx->frames_active,
                    1,
                    memory_order_relaxed
                );
                aven_thread_spinlock_unlock(&ctx->lock);
                return;
            } else if (
                atomic_load_explicit(
                    &ctx->frames_active,
                    memory_order_relaxed
                ) == 0
            ) {
                aven_thread_spinlock_unlock(&ctx->lock);
                return;
            }
            aven_thread_spinlock_unlock(&ctx->lock);
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
    GraphPropUint8 coloring;
    GraphPlaneP3ColorThreadCtx *ctx;
    uint32_t start_vertex;
    uint32_t end_vertex;
} GraphP3ColorThreadWorker;

static void graph_plane_p3color_thread_worker(void *args) {
    GraphP3ColorThreadWorker *worker = args;
    GraphPlaneP3ColorThreadCtx *ctx = worker->ctx;

    atomic_fetch_add_explicit(&ctx->threads_active, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&ctx->frames_active, 1, memory_order_relaxed);

    GraphPlaneP3ColorFrame local_frame_data[16];
    GraphPlaneP3ColorThreadFrameList local_frames = list_array(
        local_frame_data
    );

    graph_plane_p3color_pop_internal(ctx, &local_frames);

    while (local_frames.len > 0) {
        GraphPlaneP3ColorFrame cur_frame = list_pop(local_frames);
        while (
            !graph_plane_p3color_thread_frame_step(
                ctx,
                &local_frames,
                &cur_frame
            )
        ) {}

        if (local_frames.len == 0) {
            graph_plane_p3color_pop_internal(ctx, &local_frames);
        }
    }

    // synchronize all threads writes to the marks array
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

    for (uint32_t v = worker->start_vertex; v != worker->end_vertex; v += 1) {
        int32_t v_mark = get(ctx->vertex_info, v).mark;
        assert(v_mark > 0 and v_mark <= 3);
        get(worker->coloring, v) = (uint8_t)v_mark;
    }
}

static inline GraphPropUint8 graph_plane_p3color_thread(
    Graph graph,
    GraphSubset p,
    GraphSubset q,
    AvenThreadPool *thread_pool,
    size_t nthreads,
    AvenArena *arena
) {
    GraphPropUint8 coloring = { .len = graph.adj.len };
    coloring.ptr = aven_arena_create_array(uint8_t, arena, coloring.len);

    AvenArena temp_arena = *arena;

    GraphPlaneP3ColorThreadCtx ctx = graph_plane_p3color_thread_init(
        graph,
        p,
        q,
        &temp_arena
    );

    Slice(GraphP3ColorThreadWorker) workers = aven_arena_create_slice(
        GraphP3ColorThreadWorker,
        &temp_arena,
        nthreads
    );
    AvenThreadPoolJobSlice jobs = aven_arena_create_slice(
        AvenThreadPoolJob,
        &temp_arena,
        nthreads - 1
    );

    uint32_t chunk_size = (uint32_t)(graph.adj.len / workers.len);
    for (uint32_t i = 0; i < workers.len; i += 1) {
        uint32_t start_vertex = i * chunk_size;
        uint32_t end_vertex = (i + 1) * chunk_size;
        if (i + 1 == workers.len) {
            end_vertex = (uint32_t)graph.adj.len;
        }

        get(workers, i) = (GraphP3ColorThreadWorker){
            .coloring = coloring,
            .ctx = &ctx,
            .start_vertex = start_vertex,
            .end_vertex = end_vertex,
        };
    }
    for (uint32_t i = 0; i < jobs.len; i += 1) {
        get(jobs, i) = (AvenThreadPoolJob){
            .fn = graph_plane_p3color_thread_worker,
            .args = &get(workers, i),
        };
    }

    aven_thread_pool_submit_slice(thread_pool, jobs);
    graph_plane_p3color_thread_worker(&get(workers, workers.len - 1));

    aven_thread_pool_wait(thread_pool);

    return coloring;
}

#endif // GRAPH_PLANE_P3COLOR_THREAD_H
