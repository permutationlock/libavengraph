#ifndef AVEN_GRAPH_BFS_H
#define AVEN_GRAPH_BFS_H

#include <aven.h>
#include <aven/arena.h>

#include "../graph.h"

#define AVEN_GRAPH_BFS_VERTEX_INVALID 0xffffffffU

typedef struct {
    AvenGraph graph;
    AvenGraphSubset vertices;
    AvenGraphPropUint32 parents;
    uint32_t front;
    uint32_t back;
    uint32_t neighbor;
    uint32_t vertex;
} AvenGraphBfsCtx;

static void aven_graph_bfs_push_entry_internal(
    AvenGraphBfsCtx *ctx,
    uint32_t v,
    uint32_t parent
) {
    if (slice_get(ctx->parents, v) != AVEN_GRAPH_BFS_VERTEX_INVALID) {
        return;
    }
    slice_get(ctx->parents, v) = parent;
    slice_get(ctx->vertices, ctx->back) = v;
    ctx->back += 1;
}

static void aven_graph_bfs_pop_entry_internal(AvenGraphBfsCtx *ctx) {
    if (ctx->front == ctx->back) {
        ctx->vertex = AVEN_GRAPH_BFS_VERTEX_INVALID;
        return;
    }

    ctx->vertex = slice_get(ctx->vertices, ctx->front);
    ctx->front += 1;
    ctx->neighbor = 0;
}

static inline AvenGraphBfsCtx aven_graph_bfs_init(
    AvenGraph graph,
    uint32_t root_vertex,
    AvenArena *arena
) {
    assert(root_vertex < graph.len);

    AvenGraphBfsCtx ctx = {
        .graph = graph,
        .vertex = root_vertex,
        .vertices = { .len = graph.len },
        .parents = { .len = graph.len },
    };

    ctx.vertices.ptr = aven_arena_create_array(
        uint32_t,
        arena,
        ctx.vertices.len
    );
    ctx.parents.ptr = aven_arena_create_array(uint32_t, arena, ctx.parents.len);

    for (uint32_t i = 0; i < ctx.parents.len; i += 1) {
        slice_get(ctx.parents, i) = AVEN_GRAPH_BFS_VERTEX_INVALID;
    }

    slice_get(ctx.parents, ctx.vertex) = ctx.vertex;

    return ctx;
}

static inline bool aven_graph_bfs_step(AvenGraphBfsCtx *ctx) {
    if (ctx->neighbor == slice_get(ctx->graph, ctx->vertex).len) {
        aven_graph_bfs_pop_entry_internal(ctx);
        return ctx->vertex == AVEN_GRAPH_BFS_VERTEX_INVALID;
    }

    uint32_t u = slice_get(slice_get(ctx->graph, ctx->vertex), ctx->neighbor);
    aven_graph_bfs_push_entry_internal(ctx, u, ctx->vertex);
    ctx->neighbor += 1;

    return false;
}

static inline AvenGraphSubset aven_graph_bfs_path(
    AvenGraphSubset path_space,
    AvenGraphBfsCtx *ctx,
    uint32_t vertex
) {
    if (slice_get(ctx->parents, vertex) == AVEN_GRAPH_BFS_VERTEX_INVALID) {
        return (AvenGraphSubset){ 0 };
    }

    size_t len = 0;
    uint32_t last_vertex;
    do {
        slice_get(path_space, len) = vertex;
        last_vertex = vertex;
        vertex = slice_get(ctx->parents, last_vertex);
        len += 1;
    } while (last_vertex != vertex);

    return (AvenGraphSubset){ .ptr = path_space.ptr, .len = len };
}

static inline uint32_t aven_graph_bfs_backtrace(AvenGraphBfsCtx *ctx) {
    uint32_t v = ctx->vertex;
    ctx->vertex = slice_get(ctx->parents, v);
    return v;
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
        slice_get(path, 0) = root_vertex;
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
