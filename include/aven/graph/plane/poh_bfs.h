#ifndef AVEN_GRAPH_PLANE_POH_BFS_H
#define AVEN_GRAPH_PLANE_POH_BFS_H

#include <aven.h>
#include <aven/arena.h>

#include "../../graph.h"

typedef struct {
    uint32_t p1;
    uint32_t p2;
    uint32_t p2p1_index;
    uint32_t v;
    uint32_t edge_index;
    uint32_t mark;
} AvenGraphPlanePohBfsFrame;
typedef Optional(AvenGraphPlanePohBfsFrame) AvenGraphPlanePohBfsFrameOptional;

typedef struct {
    uint32_t mark;
    uint32_t parent;
} AvenGraphPlanePohBfsInfo;

typedef struct {
    AvenGraph graph;
    AvenGraphPropUint8 coloring;
    Queue(uint32_t) vertices;
    Slice(AvenGraphPlanePohBfsInfo) bfs_info;
    Slice(bool) path_end;
    List(AvenGraphPlanePohBfsFrame) frames;
} AvenGraphPlanePohBfsCtx;

static inline AvenGraphPlanePohBfsCtx aven_graph_plane_poh_bfs_init(
    AvenGraphPropUint8 coloring,
    AvenGraph graph,
    AvenGraphSubset path1,
    AvenGraphSubset path2,
    AvenArena *arena
) {
    AvenGraphPlanePohBfsCtx ctx = {
        .graph = graph,
        .coloring = coloring,
        .vertices = { .cap = graph.len },
        .bfs_info = { .len = graph.len },
        .path_end = { .len = graph.len },
        .frames = { .cap = 3 * graph.len - 6 },
    };

    ctx.vertices.ptr = aven_arena_create_array(
        uint32_t,
        arena,
        ctx.vertices.cap
    );
    ctx.bfs_info.ptr = aven_arena_create_array(
        AvenGraphPlanePohBfsInfo,
        arena,
        ctx.bfs_info.len
    );
    ctx.path_end.ptr = aven_arena_create_array(
        bool,
        arena,
        ctx.path_end.len
    );
    ctx.frames.ptr = aven_arena_create_array(
        AvenGraphPlanePohBfsFrame,
        arena,
        ctx.frames.cap
    );

    for (uint32_t i = 0; i < ctx.coloring.len; i += 1) {
        get(ctx.coloring, i) = 0;
    }

    for (uint32_t i = 0; i < ctx.bfs_info.len; i += 1) {
        get(ctx.bfs_info, i) = (AvenGraphPlanePohBfsInfo){ 0 };
    }

    for (uint32_t i = 0; i < ctx.path_end.len; i += 1) {
        get(ctx.path_end, i) = false;
    }

    for (uint32_t i = 0; i < path1.len; i += 1) {
        uint32_t v = get(path1, i);
        get(ctx.coloring, v) = 1;

        if (i + 1 == path1.len) {
            get(ctx.path_end, v) = true;
        }
    }

    for (uint32_t i = 0; i < path2.len; i += 1) {
        uint32_t v = get(path2, i);
        get(ctx.coloring, v) = 2;

        if (i + 1 == path2.len) {
            get(ctx.path_end, v) = true;
        }
    }

    uint32_t p1 = get(path1, 0);
    uint32_t p2 = get(path2, 0);
    uint32_t p2p1_index = aven_graph_neighbor_index(ctx.graph, p2, p1);
    list_push(ctx.frames) = (AvenGraphPlanePohBfsFrame){
        .p1 = p1,
        .p2 = p2,
        .p2p1_index = p2p1_index,
        .v = p2,
        .mark = 1,
    };

    return ctx;
}

static inline AvenGraphPlanePohBfsFrameOptional
aven_graph_plane_poh_bfs_next_frame(
    AvenGraphPlanePohBfsCtx *ctx
) {
    if (ctx->frames.len == 0) {
        return (AvenGraphPlanePohBfsFrameOptional){ 0 };
    }

    AvenGraphPlanePohBfsFrame *frame = &list_get(
        ctx->frames,
        ctx->frames.len - 1
    );
    ctx->frames.len -= 1;

    return (AvenGraphPlanePohBfsFrameOptional){
        .value = *frame,
        .valid = true,
    };
}

static inline bool aven_graph_plane_poh_bfs_frame_step(
    AvenGraphPlanePohBfsCtx *ctx,
    AvenGraphPlanePohBfsFrame *frame
) {
    uint8_t p1_color = get(ctx->coloring, frame->p1);
    uint8_t p2_color = get(ctx->coloring, frame->p2);

    if (frame->v == frame->p2) {
        bool p1_done = get(ctx->path_end, frame->p1);
        bool p2_done = get(ctx->path_end, frame->p2);

        if (p1_done and p2_done) {
            return true;
        }

        AvenGraphAdjList p2_adj = get(ctx->graph, frame->p2);
        uint32_t p2u_index = aven_graph_adj_prev(p2_adj, frame->p2p1_index);
        uint32_t u = get(p2_adj, p2u_index);

        uint8_t u_color = get(ctx->coloring, u);
        if (u_color == p1_color) {
            assert(!p1_done);
            frame->p1 = u;
            frame->p2p1_index = p2u_index;
        } else if (u_color == p2_color) {
            assert(!p2_done);
            frame->p2 = u;
            frame->p2p1_index = aven_graph_neighbor_index(
                ctx->graph,
                u,
                frame->p1
            );
            frame->v = u;
        } else if (u_color == 0){
            frame->v = u;
            get(ctx->bfs_info, u) = (AvenGraphPlanePohBfsInfo){
                .mark = frame->mark,
                .parent = u,
            };
        } else {
            return true;
        }

        return false;
    }

    AvenGraphAdjList v_adj = get(ctx->graph, frame->v);
    if (frame->edge_index == v_adj.len) {
        frame->v = queue_pop(ctx->vertices);
        frame->edge_index = 0;
        return false;
    }

    uint32_t vy_index = frame->edge_index;
    uint32_t y = get(v_adj, vy_index);
    uint8_t y_color = get(ctx->coloring, y);
    frame->edge_index += 1;

    if (y_color == p2_color) {
        uint32_t next_index = frame->edge_index;
        if (next_index >= v_adj.len) {
            next_index -= (uint32_t)v_adj.len;
        }
        uint32_t x = get(v_adj, next_index);

        if (get(ctx->coloring, x) == p1_color) {
            if (
                !get(ctx->path_end, x) or
                !get(ctx->path_end, y)
            ) {
                uint32_t yx_index = aven_graph_neighbor_index(
                    ctx->graph,
                    y,
                    x
                );
                list_push(ctx->frames) = (AvenGraphPlanePohBfsFrame){
                    .p1 = x,
                    .p2 = y,
                    .p2p1_index = yx_index,
                    .v = y,
                    .mark = frame->mark + 1,
                };
            }

            uint8_t p3_color = p1_color ^ p2_color;

            uint32_t w = frame->v;
            AvenGraphPlanePohBfsInfo *w_info = &get(ctx->bfs_info, w);

            get(ctx->coloring, w) = p3_color;
            get(ctx->path_end, w) = true;

            while (w_info->parent != w) {
                get(ctx->coloring, w_info->parent) = p3_color;
                w = w_info->parent;
                w_info = &get(ctx->bfs_info, w);
            }

            uint32_t wp1_index = aven_graph_neighbor_index(
                ctx->graph,
                w,
                frame->p1
            );
            list_push(ctx->frames) = (AvenGraphPlanePohBfsFrame){
                .p1 = frame->p1,
                .p2 = w,
                .p2p1_index = wp1_index,
                .v = w,
                .mark = frame->mark + 1,
            };

            uint32_t p2w_index = aven_graph_adj_prev(
                get(ctx->graph, frame->p2),
                frame->p2p1_index
            );
            *frame = (AvenGraphPlanePohBfsFrame){
                .p1 = w,
                .p2 = frame->p2,
                .p2p1_index = p2w_index,
                .v = frame->p2,
                .mark = frame->mark + 1,
            };
            queue_clear(ctx->vertices);
        }
    } else if (y_color == 0) {
        AvenGraphPlanePohBfsInfo *y_info = &get(ctx->bfs_info, y);
        if (y_info->mark != frame->mark) {
            *y_info = (AvenGraphPlanePohBfsInfo){
                .parent = frame->v,
                .mark = frame->mark,
            };
            queue_push(ctx->vertices) = y;
        }
    }

    return false;
}

static inline AvenGraphPropUint8 aven_graph_plane_poh_bfs(
    AvenGraph graph,
    AvenGraphSubset p,
    AvenGraphSubset q,
    AvenArena *arena
) {
    AvenGraphPropUint8 coloring = { .len = graph.len };
    coloring.ptr = aven_arena_create_array(uint8_t, arena, coloring.len);

    AvenArena temp_arena = *arena;
    AvenGraphPlanePohBfsCtx ctx = aven_graph_plane_poh_bfs_init(
        coloring,
        graph,
        p,
        q,
        &temp_arena
    );

    AvenGraphPlanePohBfsFrameOptional cur_frame =
        aven_graph_plane_poh_bfs_next_frame(&ctx);

    do {
        while (!aven_graph_plane_poh_bfs_frame_step(&ctx, &cur_frame.value)) {}
        cur_frame = aven_graph_plane_poh_bfs_next_frame(&ctx);
    } while (cur_frame.valid);

    return coloring;
}

#endif // AVEN_GRAPH_PLANE_POH_BFS_H
