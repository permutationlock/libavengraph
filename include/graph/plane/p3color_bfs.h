#ifndef GRAPH_PLANE_P3COLOR_BFS_H
#define GRAPH_PLANE_P3COLOR_BFS_H

#include <aven.h>
#include <aven/arena.h>

#include "../../graph.h"

typedef struct {
    uint32_t v1;
    uint32_t vk;
    uint32_t vi;
    uint32_t vi1;
    uint32_t v1vk_index;
    uint32_t uj;
    uint32_t edge_index;
    int32_t mark;
} GraphPlaneP3ColorBfsFrame;
typedef Optional(GraphPlaneP3ColorBfsFrame) GraphPlaneP3ColorBfsFrameOptional;
typedef Slice(GraphPlaneP3ColorBfsFrameOptional)
    GraphPlaneP3ColorBfsFrameOptionalSlice;

typedef struct {
    uint32_t len;
    int32_t mark;
    uint32_t parent;
    uint32_t *ptr;
} GraphPlaneP3ColorBfsVertex;

typedef Queue(uint32_t) GraphPlaneP3ColorBfsQueue;

typedef struct {
    Slice(GraphPlaneP3ColorBfsVertex) vertex_info;
    List(GraphPlaneP3ColorBfsFrame) frames;
} GraphPlaneP3ColorBfsCtx;

static inline GraphPlaneP3ColorBfsCtx graph_plane_p3color_bfs_init(
    Graph graph,
    GraphSubset path1,
    GraphSubset path2,
    AvenArena *arena
) {
    GraphPlaneP3ColorBfsCtx ctx = {
        .vertex_info = { .len = graph.len },
        .frames = { .cap = 3 * graph.len - 6 },
    };

    ctx.vertex_info.ptr = aven_arena_create_array(
        GraphPlaneP3ColorBfsVertex,
        arena,
        ctx.vertex_info.len
    );
    ctx.frames.ptr = aven_arena_create_array(
        GraphPlaneP3ColorBfsFrame,
        arena,
        ctx.frames.cap
    );

    for (uint32_t v = 0; v < ctx.vertex_info.len; v += 1) {
        GraphAdjList v_adj = get(graph, v);
        get(ctx.vertex_info, v) = (GraphPlaneP3ColorBfsVertex){
            .len = (uint32_t)v_adj.len,
            .ptr = v_adj.ptr,
        };
    }

    for (uint32_t i = 0; i < path1.len; i += 1) {
        uint32_t v = get(path1, i);
        get(ctx.vertex_info, v).mark = 1;
    }

    for (uint32_t i = 0; i < path2.len; i += 1) {
        uint32_t v = get(path2, i);
        get(ctx.vertex_info, v).mark = 2;
    }

    uint32_t v1 = get(path1, 0);
    uint32_t vi = get(path1, path1.len - 1);
    uint32_t vk = get(path2, 0);
    uint32_t vi1 = get(path2, path2.len - 1);
    uint32_t v1vk_index = graph_adj_neighbor_index(get(graph, v1), vk);
    list_push(ctx.frames) = (GraphPlaneP3ColorBfsFrame){
        .v1 = v1,
        .vk = vk,
        .vi = vi,
        .vi1 = vi1,
        .v1vk_index = v1vk_index,
        .uj = vk,
        .mark = -1,
    };

    return ctx;
}

static inline GraphPlaneP3ColorBfsFrameOptional
graph_plane_p3color_bfs_next_frame(
    GraphPlaneP3ColorBfsCtx *ctx
) {
    if (ctx->frames.len == 0) {
        return (GraphPlaneP3ColorBfsFrameOptional){ 0 };
    }

    GraphPlaneP3ColorBfsFrame *frame = &list_get(
        ctx->frames,
        ctx->frames.len - 1
    );
    ctx->frames.len -= 1;

    return (GraphPlaneP3ColorBfsFrameOptional){
        .value = *frame,
        .valid = true,
    };
}

static inline bool graph_plane_p3color_bfs_frame_step(
    GraphPlaneP3ColorBfsCtx *ctx,
    GraphPlaneP3ColorBfsFrame *frame,
    GraphPlaneP3ColorBfsQueue *bfs_queue
) {
    GraphPlaneP3ColorBfsVertex *v1_info = &get(ctx->vertex_info, frame->v1);
    GraphPlaneP3ColorBfsVertex *vk_info = &get(ctx->vertex_info, frame->vk);

    if (frame->uj == frame->vk) {
        if (frame->v1 == frame->vi and frame->vk == frame->vi1) {
            return true;
        }

        GraphAdjList v1_adj = { .len = v1_info->len, .ptr = v1_info->ptr };
        uint32_t v1u_index = graph_adj_next(v1_adj, frame->v1vk_index);
        uint32_t u = get(v1_adj, v1u_index);

        GraphPlaneP3ColorBfsVertex *u_info = &get(ctx->vertex_info, u);
        if (u_info->mark <= 0) {
            frame->uj = u;
            u_info->mark = frame->mark;
            u_info->parent = u;
        } else if (u_info->mark == vk_info->mark) {
            assert(frame->vk != frame->vi1);
            frame->vk = u;
            frame->v1vk_index = v1u_index;
            frame->uj = u;
        } else if (u_info->mark == v1_info->mark) {
            assert(frame->v1 != frame->vi);
            frame->v1 = u;
            frame->v1vk_index = graph_adj_neighbor_index(
                (GraphAdjList){
                    .len = u_info->len,
                    .ptr = u_info->ptr,
                },
                frame->vk
            );
        } else {
            return true;
        }

        return false;
    }

    GraphPlaneP3ColorBfsVertex *uj_info = &get(ctx->vertex_info, frame->uj); 
    GraphAdjList uj_adj = { .len = uj_info->len, .ptr = uj_info->ptr };
    if (frame->edge_index == uj_adj.len) {
        frame->uj = queue_pop(*bfs_queue);
        frame->edge_index = 0;
        return false;
    }

    uint32_t ujy_index = frame->edge_index;
    uint32_t y = get(uj_adj, ujy_index);
    GraphPlaneP3ColorBfsVertex *y_info = &get(ctx->vertex_info, y);
    frame->edge_index += 1;

    if (y_info->mark == vk_info->mark) {
        uint32_t next_index = frame->edge_index;
        if (next_index >= uj_adj.len) {
            next_index -= (uint32_t)uj_adj.len;
        }
        uint32_t x = get(uj_adj, next_index);
        GraphPlaneP3ColorBfsVertex *x_info = &get(ctx->vertex_info, x);

        if (x_info->mark == v1_info->mark) {
            if (x != frame->vi or y != frame->vi1) {
                uint32_t xy_index;
                if (x == frame->v1) {
                    xy_index = frame->v1vk_index;
                    for (uint32_t i = 0; i < x_info->len; i += 1) {
                        xy_index = xy_index + 1;
                        if (xy_index >= x_info->len) {
                            xy_index -= x_info->len;
                        }
                        if (get(*x_info, xy_index) == y) {
                            break;
                        }
                    }
                    assert(xy_index != frame->v1vk_index);
                } else {
                    xy_index = graph_adj_neighbor_index(
                        (GraphAdjList){
                            .len = x_info->len,
                            .ptr = x_info->ptr,
                        },
                        y
                    );
                }
                list_push(ctx->frames) = (GraphPlaneP3ColorBfsFrame){
                    .v1 = x,
                    .vk = y,
                    .vi = frame->vi,
                    .vi1 = frame->vi1,
                    .v1vk_index = xy_index,
                    .uj = y,
                    .mark = frame->mark - 1,
                };
            }

            int32_t p3_color = v1_info->mark ^ vk_info->mark;

            uint32_t w = frame->uj;
            uint32_t u = frame->uj;
            GraphPlaneP3ColorBfsVertex *u_info = &get(ctx->vertex_info, w);

            u_info->mark = p3_color;

            while (u_info->parent != u) {
                u = u_info->parent;
                u_info = &get(ctx->vertex_info, u);
                u_info->mark = p3_color;
            }

            uint32_t uvk_index = graph_adj_neighbor_index(
                (GraphAdjList){
                    .len = u_info->len,
                    .ptr = u_info->ptr,
                },
                frame->vk
            );
            list_push(ctx->frames) = (GraphPlaneP3ColorBfsFrame){
                .v1 = u,
                .vk = frame->vk,
                .vi = w,
                .vi1 = y,
                .v1vk_index = uvk_index,
                .uj = frame->vk,
                .mark = frame->mark - 1,
            };

            uint32_t v1u_index = graph_adj_next(
                (GraphAdjList){
                    .len = v1_info->len,
                    .ptr = v1_info->ptr,
                },
                frame->v1vk_index
            );
            *frame = (GraphPlaneP3ColorBfsFrame){
                .v1 = frame->v1,
                .vk = u,
                .vi = x,
                .vi1 = w,
                .v1vk_index = v1u_index,
                .uj = u,
                .mark = frame->mark - 1,
            };
            queue_clear(*bfs_queue);
        }
    } else if (y_info->mark <= 0 and y_info->mark != frame->mark) {
        y_info->parent = frame->uj;
        y_info->mark = frame->mark;
        queue_push(*bfs_queue) = y;
    }

    return false;
}

static inline GraphPropUint8 graph_plane_p3color_bfs(
    Graph graph,
    GraphSubset p,
    GraphSubset q,
    AvenArena *arena
) {
    GraphPropUint8 coloring = { .len = graph.len };
    coloring.ptr = aven_arena_create_array(uint8_t, arena, coloring.len);

    AvenArena temp_arena = *arena;
    GraphPlaneP3ColorBfsCtx ctx = graph_plane_p3color_bfs_init(
        graph,
        p,
        q,
        &temp_arena
    );

    GraphPlaneP3ColorBfsQueue bfs_queue = aven_arena_create_queue(
        uint32_t,
        &temp_arena,
        graph.len
    );

    GraphPlaneP3ColorBfsFrameOptional cur_frame =
        graph_plane_p3color_bfs_next_frame(&ctx);

    do {
        while (
            !graph_plane_p3color_bfs_frame_step(
                &ctx,
                &cur_frame.value,
                &bfs_queue
            )
        ) {}
        cur_frame = graph_plane_p3color_bfs_next_frame(&ctx);
    } while (cur_frame.valid);

    for (uint32_t v = 0; v < coloring.len; v += 1) {
        int32_t v_mark = get(ctx.vertex_info, v).mark;
        assert(v_mark > 0 and v_mark <= 3);
        get(coloring, v) = (uint8_t)v_mark;
    }

    return coloring;
}

#endif // GRAPH_PLANE_P3COLOR_BFS_H
