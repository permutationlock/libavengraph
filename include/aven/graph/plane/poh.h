#ifndef AVEN_GRAPH_PLANE_POH_H
#define AVEN_GRAPH_PLANE_POH_H

#include <aven.h>
#include <aven/arena.h>

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
} AvenGraphPlanePohCtx;

static inline AvenGraphPlanePohCtx aven_graph_plane_poh_init(
    AvenGraphPropUint8 coloring,
    AvenGraph graph,
    AvenGraphSubset p,
    AvenGraphSubset q,
    AvenArena *arena
) {
    uint32_t p1 = get(p, 0);
    uint32_t q1 = get(q, 0);

    AvenGraphPlanePohCtx ctx = {
        .graph = graph,
        .coloring = coloring,
        .vertex_info = { .len = graph.len },
        .frames = { .cap = graph.len - 2 },
    };

    ctx.vertex_info.ptr = aven_arena_create_array(
        AvenGraphPlanePohVertex,
        arena,
        ctx.vertex_info.len
    );
    ctx.frames.ptr = aven_arena_create_array(
        AvenGraphPlanePohFrame,
        arena,
        ctx.frames.cap
    );

    for (uint32_t i = 0; i < ctx.coloring.len; i += 1) {
        get(ctx.coloring, i) = 0;
    }

    get(ctx.coloring, get(p, 0)) = 1;

    for (uint32_t i = 0; i < q.len; i += 1) {
        get(ctx.coloring, get(q, i)) = 2;
    }

    for (uint32_t i = 0; i < ctx.vertex_info.len; i += 1) {
        get(ctx.vertex_info, i).mark = 0;
    }

    for (uint32_t i = 0; i < p.len; i += 1) {
        get(ctx.vertex_info, get(p, i)).mark = 1;
    }

    get(ctx.vertex_info, p1).first_edge = aven_graph_neighbor_index(
        graph,
        p1,
        q1
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

static inline bool aven_graph_plane_poh_frame_step(
    AvenGraphPlanePohCtx *ctx,
    AvenGraphPlanePohFrame *frame
) {
    uint8_t path_color = frame->p_color ^ frame->q_color;

    AvenGraphAdjList u_adj = get(ctx->graph, frame->u);

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

    uint32_t n_index = get(ctx->vertex_info, frame->u).first_edge +
        frame->edge_index;
    if (n_index >= u_adj.len) {
        n_index -= (uint32_t)u_adj.len;
    }

    uint32_t n = get(u_adj, n_index);
    uint32_t n_color = get(ctx->coloring, n);
    AvenGraphPlanePohVertex *n_info = &get(ctx->vertex_info, n);

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
                uint32_t l = get(
                    u_adj,
                    aven_graph_adj_prev(u_adj, n_index)
                );

                get(ctx->coloring, l) = frame->q_color;
                get(ctx->vertex_info, l).first_edge =
                    aven_graph_neighbor_index(ctx->graph, l, frame->u);

                list_push(ctx->frames) = (AvenGraphPlanePohFrame){
                    .q_color = path_color,
                    .p_color = frame->p_color,
                    .u = l,
                    .t = l,
                    .w = l,
                    .face_mark = frame->face_mark,
                };
            }

            frame->last_uncolored = false;
        }
    } else if (n != frame->w) {
        if (n_color != 0) {
            if (n_color == frame->p_color) {
                frame->above_path = true;
            }
            if (frame->w != frame->u) {
                list_push(ctx->frames) = (AvenGraphPlanePohFrame){
                    .q_color = frame->q_color,
                    .p_color = path_color,
                    .u = frame->w,
                    .w = frame->w,
                    .t = frame->w,
                    .face_mark = frame->face_mark + 1,
                };

                frame->w = frame->u;
            }
        } else if (n_info->mark == frame->face_mark) {
            get(ctx->coloring, n) = path_color;
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

                get(ctx->coloring, n) = frame->p_color;
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

#endif // AVEN_GRAPH_PLANE_POH_H
