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
    int32_t mark;
} AvenGraphPlanePohBfsFrame;
typedef Optional(AvenGraphPlanePohBfsFrame) AvenGraphPlanePohBfsFrameOptional;

typedef struct {
    uint32_t len;
    int32_t mark;
    uint32_t parent;
    bool path_end;
    uint32_t *ptr;
} AvenGraphPlanePohBfsVertex;

typedef struct {
    Queue(uint32_t) vertices;
    Slice(AvenGraphPlanePohBfsVertex) vertex_info;
    List(AvenGraphPlanePohBfsFrame) frames;
} AvenGraphPlanePohBfsCtx;

static inline AvenGraphPlanePohBfsCtx aven_graph_plane_poh_bfs_init(
    AvenGraph graph,
    AvenGraphSubset path1,
    AvenGraphSubset path2,
    AvenArena *arena
) {
    AvenGraphPlanePohBfsCtx ctx = {
        .vertices = { .cap = graph.len },
        .vertex_info = { .len = graph.len },
        .frames = { .cap = 3 * graph.len - 6 },
    };

    ctx.vertices.ptr = aven_arena_create_array(
        uint32_t,
        arena,
        ctx.vertices.cap
    );
    ctx.vertex_info.ptr = aven_arena_create_array(
        AvenGraphPlanePohBfsVertex,
        arena,
        ctx.vertex_info.len
    );
    ctx.frames.ptr = aven_arena_create_array(
        AvenGraphPlanePohBfsFrame,
        arena,
        ctx.frames.cap
    );

    for (uint32_t v = 0; v < ctx.vertex_info.len; v += 1) {
        AvenGraphAdjList v_adj = get(graph, v);
        get(ctx.vertex_info, v) = (AvenGraphPlanePohBfsVertex){
            .len = (uint32_t)v_adj.len,
            .ptr = v_adj.ptr,
        };
    }

    for (uint32_t i = 0; i < path1.len; i += 1) {
        uint32_t v = get(path1, i);
        get(ctx.vertex_info, v).mark = 1;

        if (i + 1 == path1.len) {
            get(ctx.vertex_info, v).path_end = true;
        }
    }

    for (uint32_t i = 0; i < path2.len; i += 1) {
        uint32_t v = get(path2, i);
        get(ctx.vertex_info, v).mark = 2;

        if (i + 1 == path2.len) {
            get(ctx.vertex_info, v).path_end = true;
        }
    }

    uint32_t p1 = get(path1, 0);
    uint32_t p2 = get(path2, 0);
    uint32_t p2p1_index = aven_graph_adj_neighbor_index(get(graph, p2), p1);
    list_push(ctx.frames) = (AvenGraphPlanePohBfsFrame){
        .p1 = p1,
        .p2 = p2,
        .p2p1_index = p2p1_index,
        .v = p2,
        .mark = -1,
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
    AvenGraphPlanePohBfsVertex *p1_info = &get(ctx->vertex_info, frame->p1);
    AvenGraphPlanePohBfsVertex *p2_info = &get(ctx->vertex_info, frame->p2);

    if (frame->v == frame->p2) {
        if (p1_info->path_end and p2_info->path_end) {
            return true;
        }

        AvenGraphAdjList p2_adj = { .len = p2_info->len, .ptr = p2_info->ptr };
        uint32_t p2u_index = aven_graph_adj_prev(p2_adj, frame->p2p1_index);
        uint32_t u = get(p2_adj, p2u_index);

        AvenGraphPlanePohBfsVertex *u_info = &get(ctx->vertex_info, u);
        if (u_info->mark <= 0) {
            frame->v = u;
            u_info->mark = frame->mark;
            u_info->parent = u;
        } else if (u_info->mark == p1_info->mark) {
            assert(!p1_info->path_end);
            frame->p1 = u;
            frame->p2p1_index = p2u_index;
        } else if (u_info->mark == p2_info->mark) {
            assert(!p2_info->path_end);
            frame->p2 = u;
            frame->p2p1_index = aven_graph_adj_neighbor_index(
                (AvenGraphAdjList){
                    .len = u_info->len,
                    .ptr = u_info->ptr,
                },
                frame->p1
            );
            frame->v = u;
        } else {
            return true;
        }

        return false;
    }

    AvenGraphPlanePohBfsVertex *v_info = &get(ctx->vertex_info, frame->v); 
    AvenGraphAdjList v_adj = { .len = v_info->len, .ptr = v_info->ptr };
    if (frame->edge_index == v_adj.len) {
        frame->v = queue_pop(ctx->vertices);
        frame->edge_index = 0;
        return false;
    }

    uint32_t vy_index = frame->edge_index;
    uint32_t y = get(v_adj, vy_index);
    AvenGraphPlanePohBfsVertex *y_info = &get(ctx->vertex_info, y);
    frame->edge_index += 1;

    if (y_info->mark == p2_info->mark) {
        uint32_t next_index = frame->edge_index;
        if (next_index >= v_adj.len) {
            next_index -= (uint32_t)v_adj.len;
        }
        uint32_t x = get(v_adj, next_index);
        AvenGraphPlanePohBfsVertex *x_info = &get(ctx->vertex_info, x);

        if (x_info->mark == p1_info->mark) {
            if (!x_info->path_end or !y_info->path_end) {
                uint32_t yx_index = aven_graph_adj_neighbor_index(
                    (AvenGraphAdjList){
                        .len = y_info->len,
                        .ptr = y_info->ptr,
                    },
                    x
                );
                list_push(ctx->frames) = (AvenGraphPlanePohBfsFrame){
                    .p1 = x,
                    .p2 = y,
                    .p2p1_index = yx_index,
                    .v = y,
                    .mark = frame->mark - 1,
                };
            }

            int32_t p3_color = p1_info->mark ^ p2_info->mark;

            uint32_t w = frame->v;
            AvenGraphPlanePohBfsVertex *w_info = &get(ctx->vertex_info, w);

            w_info->mark = p3_color;
            w_info->path_end = true;

            while (w_info->parent != w) {
                w = w_info->parent;
                w_info = &get(ctx->vertex_info, w);
                w_info->mark = p3_color;
            }

            uint32_t wp1_index = aven_graph_adj_neighbor_index(
                (AvenGraphAdjList){
                    .len = w_info->len,
                    .ptr = w_info->ptr,
                },
                frame->p1
            );
            list_push(ctx->frames) = (AvenGraphPlanePohBfsFrame){
                .p1 = frame->p1,
                .p2 = w,
                .p2p1_index = wp1_index,
                .v = w,
                .mark = frame->mark - 1,
            };

            uint32_t p2w_index = aven_graph_adj_prev(
                (AvenGraphAdjList){
                    .len = p2_info->len,
                    .ptr = p2_info->ptr,
                },
                frame->p2p1_index
            );
            *frame = (AvenGraphPlanePohBfsFrame){
                .p1 = w,
                .p2 = frame->p2,
                .p2p1_index = p2w_index,
                .v = frame->p2,
                .mark = frame->mark - 1,
            };
            queue_clear(ctx->vertices);
        }
    } else if (y_info->mark <= 0 and y_info->mark != frame->mark) {
        y_info->parent = frame->v;
        y_info->mark = frame->mark;
        queue_push(ctx->vertices) = y;
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

    for (uint32_t v = 0; v < coloring.len; v += 1) {
        int32_t v_mark = get(ctx.vertex_info, v).mark;
        assert(v_mark > 0 and v_mark <= 3);
        get(coloring, v) = (uint8_t)v_mark;
    }

    return coloring;
}

#endif // AVEN_GRAPH_PLANE_POH_BFS_H
