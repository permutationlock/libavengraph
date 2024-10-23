#ifndef AVEN_GRAPH_BFS_H
#define AVEN_GRAPH_BFS_H

#include <aven.h>
#include <aven/arena.h>

#include "../graph.h"

#define AVEN_GRAPH_BFS_VERTEX_INVALID 0xffffffffU

typedef struct {
    AvenGraphSubset vertices;
    AvenGraphPropUint32 parents;
    uint32_t front;
    uint32_t back;
    uint32_t neighbor;
    uint32_t target_vertex;
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
    }

    ctx->vertex = slice_get(ctx->vertices, ctx->front);
    ctx->front += 1;
    ctx->neighbor = 0;
}

static inline AvenGraphBfsCtx aven_graph_bfs_init(
    AvenGraph g,
    uint32_t start_vertex,
    uint32_t target_vertex,
    AvenArena *arena
) {
    assert(start_vertex < g.len);
    assert(target_vertex < g.len);

    AvenGraphBfsCtx ctx = {
        .vertex = start_vertex,
        .target_vertex = target_vertex,
        .vertices = { .len = g.len },
        .parents = { .len = g.len },
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

static inline bool aven_graph_bfs_step(AvenGraphBfsCtx *ctx, AvenGraph g) {
    if (ctx->vertex == AVEN_GRAPH_BFS_VERTEX_INVALID) {
        return true;
    }

    if (ctx->vertex == ctx->target_vertex) {
        return true;
    }

    if (ctx->neighbor == slice_get(g, ctx->vertex).len) {
        aven_graph_bfs_pop_entry_internal(ctx);
        return false;
    }

    uint32_t u = slice_get(slice_get(g, ctx->vertex), ctx->neighbor);
    aven_graph_bfs_push_entry_internal(ctx, u, ctx->vertex);
    ctx->neighbor += 1;

    if (u == ctx->target_vertex) {
        ctx->vertex = u;
        return true;
    }

    return false;
}

static inline AvenGraphSubset aven_graph_bfs_path(
    AvenGraphBfsCtx *ctx,
    AvenArena *arena
) {
    if (ctx->vertex != ctx->target_vertex) {
        return (AvenGraphSubset){ 0 };
    }

    size_t len = 1;
    uint32_t vertex = ctx->vertex;
    while (
        vertex != slice_get(ctx->parents, vertex) and
        len < ctx->vertices.len
    ) {
        len += 1;
        vertex = slice_get(ctx->parents, vertex);
    }
    assert(vertex == slice_get(ctx->parents, vertex));

    AvenGraphSubset path = { .len = len };
    path.ptr = aven_arena_create_array(uint32_t, arena, path.len);

    vertex = ctx->vertex;
    while (
        vertex != slice_get(ctx->parents, vertex) and
        len != 0
    ) {
        len -= 1;
        slice_get(path, len) = vertex;
        vertex = slice_get(ctx->parents, vertex);
    }
    assert(len == 1);
    slice_get(path, 0) = vertex;

    return path;
}

static inline uint32_t aven_graph_bfs_backtrace(AvenGraphBfsCtx *ctx) {
    uint32_t v = ctx->vertex;
    ctx->vertex = slice_get(ctx->parents, v);
    return v;
}

static inline AvenGraphSubset aven_graph_bfs(
    AvenGraph g,
    uint32_t start_vertex,
    uint32_t target_vertex,
    AvenArena *arena,
    AvenArena temp_arena
) {
    AvenGraphBfsCtx ctx = aven_graph_bfs_init(
        g,
        start_vertex,
        target_vertex,
        &temp_arena
    );

    while (!aven_graph_bfs_step(&ctx, g)) {};

    return aven_graph_bfs_path(&ctx, arena);
}

#endif // AVEN_GRAPH_BFS_H
