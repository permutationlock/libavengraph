#ifndef AVEN_GRAPH_BFS_H
#define AVEN_GRAPH_BFS_H

#include <aven.h>
#include <aven/arena.h>

#include "../graph.h"

#define AVEN_GRAPH_BFS_VERTEX_INVALID 0xffffffffU

typedef struct {
    uint32_t len;
    uint32_t parent;
    uint32_t *ptr;
} AvenGraphBfsVertex;

typedef struct {
    Slice(AvenGraphBfsVertex) vertex_info;
    Queue(uint32_t) bfs_queue;
    uint32_t edge_index;
    uint32_t vertex;
} AvenGraphBfsCtx;

static inline AvenGraphBfsCtx aven_graph_bfs_init(
    AvenGraph graph,
    uint32_t root_vertex,
    AvenArena *arena
) {
    assert(root_vertex < graph.len);

    AvenGraphBfsCtx ctx = {
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
        AvenGraphBfsVertex,
        arena,
        ctx.vertex_info.len
    );

    for (uint32_t v = 0; v < ctx.vertex_info.len; v += 1) {
        AvenGraphAdjList v_adj = get(graph, v);
        get(ctx.vertex_info, v) = (AvenGraphBfsVertex){
            .len = (uint32_t)v_adj.len,
            .ptr = v_adj.ptr,
        };
    }

    get(ctx.vertex_info, ctx.vertex).parent = ctx.vertex + 1;

    return ctx;
}

static inline bool aven_graph_bfs_step(AvenGraphBfsCtx *ctx) {
    AvenGraphBfsVertex *v_info = &get(ctx->vertex_info, ctx->vertex);
    if (ctx->edge_index == v_info->len) {
        if (ctx->bfs_queue.used == 0) {
            return true;
        }

        ctx->vertex = queue_pop(ctx->bfs_queue);
        ctx->edge_index = 0;
        return false;
    }

    uint32_t u = get(*v_info, ctx->edge_index);
    AvenGraphBfsVertex *u_info = &get(ctx->vertex_info, u);
    if (u_info->parent == 0) {
        queue_push(ctx->bfs_queue) = u;
        u_info->parent = ctx->vertex + 1;
    }
    ctx->edge_index += 1;

    return false;
}

static inline AvenGraphSubset aven_graph_bfs_path(
    AvenGraphSubset path_space,
    AvenGraphBfsCtx *ctx,
    uint32_t vertex
) {
    if (get(ctx->vertex_info, vertex).parent == 0) {
        return (AvenGraphSubset){ 0 };
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

    return (AvenGraphSubset){ .ptr = path_space.ptr, .len = len };
}

static inline uint32_t aven_graph_bfs_backtrace(AvenGraphBfsCtx *ctx, uint32_t v) {
    uint32_t parent = get(ctx->vertex_info, v).parent;
    if (parent == 0) {
        return v;
    }

    return parent - 1;
}

static inline AvenGraphSubset aven_graph_bfs(
    AvenGraph graph,
    uint32_t root_vertex,
    uint32_t vertex,
    AvenArena *arena
) {
    if (root_vertex == vertex) {
        AvenGraphSubset path = { .len = 1 };
        path.ptr = aven_arena_create(uint32_t, arena);
        get(path, 0) = root_vertex;
        return path;
    }

    AvenGraphSubset path_space = { .len = graph.len };
    path_space.ptr = aven_arena_create_array(
        uint32_t,
        arena,
        path_space.len
    );

    AvenArena temp_arena = *arena;
    AvenGraphBfsCtx ctx = aven_graph_bfs_init(
        graph,
        root_vertex,
        &temp_arena
    );

    while (!aven_graph_bfs_step(&ctx)) {};

    AvenGraphSubset path = aven_graph_bfs_path(path_space, &ctx, vertex);
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

#endif // AVEN_GRAPH_BFS_H
