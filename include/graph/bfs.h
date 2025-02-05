#ifndef GRAPH_BFS_H
#define GRAPH_BFS_H

#include <aven.h>
#include <aven/arena.h>

#include "../graph.h"

typedef struct {
    uint32_t len;
    uint32_t parent;
    uint32_t *ptr;
} GraphBfsVertex;

typedef struct {
    Slice(GraphBfsVertex) vertex_info;
    Queue(uint32_t) bfs_queue;
    uint32_t edge_index;
    uint32_t vertex;
} GraphBfsCtx;

static inline GraphBfsCtx graph_bfs_init(
    Graph graph,
    uint32_t root_vertex,
    AvenArena *arena
) {
    assert(root_vertex < graph.len);

    GraphBfsCtx ctx = {
        .vertex = root_vertex,
        .bfs_queue = { .cap = graph.len },
        .vertex_info = { .len = graph.len },
    };

    ctx.bfs_queue.ptr = aven_arena_create_array(
        uint32_t,
        arena,
        ctx.bfs_queue.cap
    );
    ctx.vertex_info.ptr = aven_arena_create_array(
        GraphBfsVertex,
        arena,
        ctx.vertex_info.len
    );

    for (uint32_t v = 0; v < ctx.vertex_info.len; v += 1) {
        GraphAdjList v_adj = get(graph, v);
        get(ctx.vertex_info, v) = (GraphBfsVertex){
            .len = (uint32_t)v_adj.len,
            .ptr = v_adj.ptr,
        };
    }

    get(ctx.vertex_info, ctx.vertex).parent = ctx.vertex + 1;

    return ctx;
}

static inline bool graph_bfs_step(GraphBfsCtx *ctx) {
    GraphBfsVertex *v_info = &get(ctx->vertex_info, ctx->vertex);
    if (ctx->edge_index == v_info->len) {
        if (ctx->bfs_queue.used == 0) {
            return true;
        }

        ctx->vertex = queue_pop(ctx->bfs_queue);
        ctx->edge_index = 0;
        return false;
    }

    uint32_t u = get(*v_info, ctx->edge_index);
    GraphBfsVertex *u_info = &get(ctx->vertex_info, u);
    if (u_info->parent == 0) {
        queue_push(ctx->bfs_queue) = u;
        u_info->parent = ctx->vertex + 1;
    }
    ctx->edge_index += 1;

    return false;
}

static inline GraphSubset graph_bfs_path(
    GraphSubset path_space,
    GraphBfsCtx *ctx,
    uint32_t vertex
) {
    if (get(ctx->vertex_info, vertex).parent == 0) {
        return (GraphSubset){ 0 };
    }

    size_t len = 0;
    uint32_t last_vertex;
    do {
        get(path_space, len) = vertex;
        last_vertex = vertex;
        assert(get(ctx->vertex_info, last_vertex).parent != 0);
        vertex = get(ctx->vertex_info, last_vertex).parent - 1;
        len += 1;
    } while (last_vertex != vertex);

    return (GraphSubset){ .ptr = path_space.ptr, .len = len };
}

static inline uint32_t graph_bfs_backtrace(GraphBfsCtx *ctx, uint32_t v) {
    uint32_t parent = get(ctx->vertex_info, v).parent;
    if (parent == 0) {
        return v;
    }

    return parent - 1;
}

static inline GraphSubset graph_bfs(
    Graph graph,
    uint32_t root_vertex,
    uint32_t vertex,
    AvenArena *arena
) {
    if (root_vertex == vertex) {
        GraphSubset path = { .len = 1 };
        path.ptr = aven_arena_create(uint32_t, arena);
        get(path, 0) = root_vertex;
        return path;
    }

    GraphSubset path_space = { .len = graph.len };
    path_space.ptr = aven_arena_create_array(
        uint32_t,
        arena,
        path_space.len
    );

    AvenArena temp_arena = *arena;
    GraphBfsCtx ctx = graph_bfs_init(
        graph,
        root_vertex,
        &temp_arena
    );

    while (!graph_bfs_step(&ctx)) {};

    GraphSubset path = graph_bfs_path(path_space, &ctx, vertex);
    path.ptr = aven_arena_resize_array(
        uint32_t,
        arena,
        path_space.ptr,
        path_space.len,
        path.len
    );
    assert(path.ptr == path_space.ptr);

    return path;
}

#endif // GRAPH_BFS_H
