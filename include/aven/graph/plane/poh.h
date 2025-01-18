#ifndef AVEN_GRAPH_PLANE_POH_H
#define AVEN_GRAPH_PLANE_POH_H

#include <aven.h>
#include <aven/arena.h>

#include "../../graph.h"

typedef struct {
    uint32_t u;
    uint32_t u_nb_first;
    uint32_t x;
    uint32_t x_nb_first;
    uint32_t y;
    uint32_t z;
    uint32_t edge_index;
    int32_t face_mark;
    uint8_t q_color;
    uint8_t p_color;
    bool above_path;
    bool last_colored;
} AvenGraphPlanePohFrame;
typedef Optional(AvenGraphPlanePohFrame) AvenGraphPlanePohFrameOptional;

typedef struct {
    AvenGraph graph;
    Slice(int32_t) marks;
    List(AvenGraphPlanePohFrame) frames;
} AvenGraphPlanePohCtx;

static inline AvenGraphPlanePohCtx aven_graph_plane_poh_init(
    AvenGraph graph,
    AvenGraphSubset p,
    AvenGraphSubset q,
    AvenArena *arena
) {
    uint32_t p1 = get(p, 0);
    uint32_t q1 = get(q, 0);

    AvenGraphPlanePohCtx ctx = {
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
        assert(frame->z == frame->u);

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
                list_push(ctx->frames) = (AvenGraphPlanePohFrame){
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
                };
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
                list_push(ctx->frames) = (AvenGraphPlanePohFrame){
                    .p_color = path_color,
                    .q_color = frame->q_color,
                    .u = frame->x,
                    .u_nb_first = frame->x_nb_first,
                    .x = frame->x,
                    .y = frame->x,
                    .z = frame->x,
                    .face_mark = frame->face_mark - 1,
                };

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

    for (uint32_t v = 0; v < coloring.len; v += 1) {
        assert(get(ctx.marks, v) > 0);
        get(coloring, v) = (uint8_t)get(ctx.marks, v);
    }

    return coloring;
}

#endif // AVEN_GRAPH_PLANE_POH_H
