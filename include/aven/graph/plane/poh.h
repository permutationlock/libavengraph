#ifndef AVEN_GRAPH_PLANE_POH_H
#define AVEN_GRAPH_PLANE_POH_H

#include <aven.h>
#include <aven/arena.h>

#ifdef AVEN_GRAPH_PLANE_POH_X86_PTHREAD
    #include <aven/thread_pool.h>
#endif // AVEN_GRAPH_PLANE_POH_X86_PTHREAD

#include "../../graph.h"

typedef struct {
    uint32_t u;
    uint32_t w;
    uint32_t t;
    uint32_t edge_index;
    uint32_t face_mark;
    uint8_t q_color;
    uint8_t p_color;
    bool above_path;
    bool last_uncolored;
} AvenGraphPlanePohFrame;
typedef Optional(AvenGraphPlanePohFrame) AvenGraphPlanePohFrameOptional;

typedef struct {
    uint32_t mark;
    uint32_t first_edge;
} AvenGraphPlanePohVertex;

typedef struct {
    AvenGraph graph;
    AvenGraphPropUint8 coloring;
    Slice(AvenGraphPlanePohVertex) vertex_info;
    List(AvenGraphPlanePohFrame) frames;
#ifdef AVEN_GRAPH_PLANE_POH_X86_PTHREAD
    int frames_active;
    bool frames_lock;
#endif // AVEN_GRAPH_PLANE_POH_X86_PTHREAD
} AvenGraphPlanePohCtx;

static inline AvenGraphPlanePohCtx aven_graph_plane_poh_init(
    AvenGraphPropUint8 coloring,
    AvenGraph graph,
    AvenGraphSubset p,
    AvenGraphSubset q,
    AvenArena *arena
) {
    uint32_t p1 = slice_get(p, 0);
    uint32_t q1 = slice_get(q, 0);

    AvenGraphPlanePohCtx ctx = {
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
        slice_get(ctx.vertex_info, i).mark = 0;
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
        AvenGraphPlanePohFrame,
        arena,
        ctx.frames.cap
    );

    list_push(ctx.frames) = (AvenGraphPlanePohFrame){
        .p_color = 3,
        .q_color = 2,
        .u = p1,
        .w = p1,
        .t = p1,
        .face_mark = 1,
    };

    return ctx;
}

static inline AvenGraphPlanePohFrameOptional aven_graph_plane_poh_next_frame(
    AvenGraphPlanePohCtx *ctx
) {
    if (ctx->frames.len == 0) {
        return (AvenGraphPlanePohFrameOptional){ 0 };
    }

    AvenGraphPlanePohFrame *frame = &list_get(ctx->frames, ctx->frames.len - 1);
    ctx->frames.len -= 1;

    return (AvenGraphPlanePohFrameOptional){ .value = *frame, .valid = true };
}

#ifdef AVEN_GRAPH_PLANE_POH_X86_PTHREAD
    static inline void aven_graph_plane_poh_lock_internal(
        AvenGraphPlanePohCtx *ctx
    ) {    
        for (;;) {
            if (
                !__atomic_exchange_n(&ctx->frames_lock, true, __ATOMIC_ACQUIRE)
            ) {
                return;
            }
            while (__atomic_load_n(&ctx->frames_lock, __ATOMIC_RELAXED)) {
                __builtin_ia32_pause();
            }
        }
    }

    static inline void aven_graph_plane_poh_unlock_internal(
        AvenGraphPlanePohCtx *ctx
    ) {
        __atomic_store_n(&ctx->frames_lock, false, __ATOMIC_RELEASE);
    }

    static inline void aven_graph_plane_poh_push_internal(
        AvenGraphPlanePohCtx *ctx,
        AvenGraphPlanePohFrame frame
    ) {
        aven_graph_plane_poh_lock_internal(ctx);
        list_push(ctx->frames) = frame;
        // size_t len = __atomic_fetch_add(&ctx->frames.len, 1, __ATOMIC_RELEASE);
        // assert(len < ctx->frames.cap);
        // ctx->frames.ptr[len] = frame;
        aven_graph_plane_poh_unlock_internal(ctx);
    }
#endif

static inline bool aven_graph_plane_poh_frame_step(
    AvenGraphPlanePohCtx *ctx,
    AvenGraphPlanePohFrame *frame
) {
    uint8_t path_color = frame->p_color ^ frame->q_color;

    AvenGraphAdjList u_adj = slice_get(ctx->graph, frame->u);

    if (frame->edge_index == u_adj.len) {
        if (frame->t == frame->u) {
            assert(frame->w == frame->u);
            return true;
        }

        if (frame->w == frame->u) {
            frame->w = frame->t;
        }
        frame->u = frame->t;
        frame->edge_index = 0;
        frame->above_path = false;
        frame->last_uncolored = false;
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
            frame->last_uncolored = true;
        } else {
            if (
                frame->last_uncolored and
                n_color == frame->p_color and
                n_info->mark == (frame->face_mark - 1)
            ) {
                uint32_t l = slice_get(
                    u_adj,
                    (n_index + u_adj.len - 1) % u_adj.len
                );

                slice_get(ctx->coloring, l) = frame->q_color;
                slice_get(ctx->vertex_info, l).first_edge =
                    aven_graph_neighbor_index(ctx->graph, l, frame->u);

#ifdef AVEN_GRAPH_PLANE_POH_X86_PTHREAD
                aven_graph_plane_poh_push_internal(
                    ctx,
                    (AvenGraphPlanePohFrame){
                        .q_color = path_color,
                        .p_color = frame->p_color,
                        .u = l,
                        .t = l,
                        .w = l,
                        .face_mark = frame->face_mark,
                    }
                );
#else // AVEN_GRAPH_PLANE_POH_X86_PTHREAD
                list_push(ctx->frames) = (AvenGraphPlanePohFrame){
                    .q_color = path_color,
                    .p_color = frame->p_color,
                    .u = l,
                    .t = l,
                    .w = l,
                    .face_mark = frame->face_mark,
                };
#endif // AVEN_GRAPH_PLANE_POH_X86_PTHREAD
            }

            frame->last_uncolored = false;
        }
    } else if (n != frame->w) {
        if (n_color != 0) {
            if (n_color == frame->p_color) {
                frame->above_path = true;
            }
            if (frame->w != frame->u) {
#ifdef AVEN_GRAPH_PLANE_POH_X86_PTHREAD
                aven_graph_plane_poh_push_internal(
                    ctx,
                    (AvenGraphPlanePohFrame){
                        .q_color = frame->q_color,
                        .p_color = path_color,
                        .u = frame->w,
                        .w = frame->w,
                        .t = frame->w,
                        .face_mark = frame->face_mark + 1,
                    }
                );
#else // AVEN_GRAPH_PLANE_POH_X86_PTHREAD
                list_push(ctx->frames) = (AvenGraphPlanePohFrame){
                    .q_color = frame->q_color,
                    .p_color = path_color,
                    .u = frame->w,
                    .w = frame->w,
                    .t = frame->w,
                    .face_mark = frame->face_mark + 1,
                };
#endif // AVEN_GRAPH_PLANE_POH_X86_PTHREAD

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
            n_info->mark = frame->face_mark + 1;

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

static inline AvenGraphPropUint8 aven_graph_plane_poh(
    AvenGraph graph,
    AvenGraphSubset p,
    AvenGraphSubset q,
    AvenArena *arena
) {
    AvenGraphPropUint8 coloring = { .len = graph.len };
    coloring.ptr = aven_arena_create_array(uint8_t, arena, coloring.len);

    AvenArena temp_arena = *arena;
    AvenGraphPlanePohCtx ctx = aven_graph_plane_poh_init(
        coloring,
        graph,
        p,
        q,
        &temp_arena
    );

    AvenGraphPlanePohFrameOptional cur_frame = aven_graph_plane_poh_next_frame(
        &ctx
    );

    do {
        while (!aven_graph_plane_poh_frame_step(&ctx, &cur_frame.value)) {}
        cur_frame = aven_graph_plane_poh_next_frame(&ctx);
    } while (cur_frame.valid);

    return coloring;
}

#ifdef AVEN_GRAPH_PLANE_POH_X86_PTHREAD
    static inline AvenGraphPlanePohFrameOptional aven_graph_plane_poh_pop_internal(
        AvenGraphPlanePohCtx *ctx
    ) {
        __atomic_sub_fetch(&ctx->frames_active, 1, __ATOMIC_RELAXED);
        for (;;) {
            if (
                ctx->frames.len > 0 or
                ctx->frames_active == 0
            ) {
                aven_graph_plane_poh_lock_internal(ctx);
                if (ctx->frames.len != 0) {
                    ctx->frames.len -= 1;
                    AvenGraphPlanePohFrameOptional frame = {
                        .value = ctx->frames.ptr[ctx->frames.len],
                        .valid = true,
                    };
                    __atomic_add_fetch(&ctx->frames_active, 1, __ATOMIC_RELAXED);
                    aven_graph_plane_poh_unlock_internal(ctx);
                    return frame;
                } else if (ctx->frames_active == 0) {
                    aven_graph_plane_poh_unlock_internal(ctx);
                    return (AvenGraphPlanePohFrameOptional){ 0 };
                }
                aven_graph_plane_poh_unlock_internal(ctx);
            }
            while (
                ctx->frames.len == 0 and
                ctx->frames_active > 0
            ) {
                __builtin_ia32_pause();
            }
        }
    }

    static void aven_graph_poh_pthread_worker_internal(void *args) {
        AvenGraphPlanePohCtx *ctx = args;

        __atomic_add_fetch(&ctx->frames_active, 1, __ATOMIC_RELAXED);

        AvenGraphPlanePohFrameOptional cur_frame =
            aven_graph_plane_poh_pop_internal(ctx);

        while (cur_frame.valid) {
            while (!aven_graph_plane_poh_frame_step(ctx, &cur_frame.value)) {}

            cur_frame = aven_graph_plane_poh_pop_internal(ctx);
        }
    }

    static inline AvenGraphPropUint8 aven_graph_plane_poh_pthread(
        AvenGraph graph,
        AvenGraphSubset p,
        AvenGraphSubset q,
        AvenThreadPool *thread_pool,
        AvenArena *arena
    ) {
        AvenGraphPropUint8 coloring = { .len = graph.len };
        coloring.ptr = aven_arena_create_array(uint8_t, arena, coloring.len);

        AvenArena temp_arena = *arena;
        
        AvenGraphPlanePohCtx ctx = aven_graph_plane_poh_init(
            coloring,
            graph,
            p,
            q,
            &temp_arena
        );

        for (size_t i = 0; i < thread_pool->active_threads; i += 1) {
            aven_thread_pool_submit(
                thread_pool,
                aven_graph_poh_pthread_worker_internal,
                &ctx
            );
        }

        aven_graph_poh_pthread_worker_internal(&ctx);

        aven_thread_pool_wait(thread_pool);

        return coloring;
    }
#endif // AVEN_GRAPH_PLANE_POH_X86_PTHREAD

#endif // AVEN_GRAPH_PLANE_POH_H
