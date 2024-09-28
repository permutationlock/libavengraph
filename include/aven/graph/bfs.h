#ifndef AVEN_GRAPH_BFS_H
#define AVEN_GRAPH_BFS_H

#include <aven.h>
#include <aven/arena.h>

#include "../graph.h"

#define AVEN_GRAPH_BFS_VERTEX_INVALID 0xffffffffU

typedef struct {
    uint32_t *vertices;
    uint32_t *parents;
    uint32_t size;
    uint32_t target_vertex;
    uint32_t front;
    uint32_t back;
    uint32_t vertex;
    uint32_t neighbor;
} AvenGraphBfsCtx;

static void aven_graph_bfs_push_entry_internal(
    AvenGraphBfsCtx *ctx,
    uint32_t v,
    uint32_t parent
) {
    assert(ctx->back < ctx->size);
    if (ctx->parents[v] != AVEN_GRAPH_BFS_VERTEX_INVALID) {
        return;
    }
    ctx->parents[v] = parent;
    ctx->vertices[ctx->back] = v;
    ctx->back += 1;
}

static void aven_graph_bfs_pop_entry_internal(AvenGraphBfsCtx *ctx) {
    if (ctx->front == ctx->back) {
        ctx->vertex = AVEN_GRAPH_BFS_VERTEX_INVALID;
    }

    ctx->vertex = ctx->vertices[ctx->front];
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
        .size = (uint32_t)g.len,
    };

    ctx.vertices = aven_arena_create_array(uint32_t, arena, 2 * g.len);
    ctx.parents = ctx.vertices + g.len;

    for (uint32_t i = 0; i < ctx.size; i += 1) {
        ctx.parents[i] = AVEN_GRAPH_BFS_VERTEX_INVALID;
    }

    ctx.parents[ctx.vertex] = ctx.vertex;

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

    return false;
}

static inline AvenGraphSubset aven_graph_bfs_result(
    AvenGraphBfsCtx *ctx,
    AvenArena *arena
) {
    if (ctx->vertex == AVEN_GRAPH_BFS_VERTEX_INVALID) {
        return (AvenGraphSubset){ 0 };
    }

    size_t len = 1;
    uint32_t vertex = ctx->vertex;
    assert(vertex == ctx->target_vertex);

    while (vertex != ctx->parents[vertex]) {
        len += 1;
        vertex = ctx->parents[vertex];
        assert(vertex < ctx->size);
    }

    AvenGraphSubset path = { .len = len };

    vertex = ctx->vertex;
    while (vertex != ctx->parents[vertex]) {
        len -= 1;
        ctx->vertices[len] = vertex;
        vertex = ctx->parents[vertex];
    }
    assert(len == 1);
    ctx->vertices[0] = vertex;

    path.ptr = aven_arena_resize_array(
        uint32_t,
        arena,
        ctx->vertices,
        2 * (size_t)ctx->size,
        path.len
    );

    return path;
}

static inline AvenGraphSubset aven_graph_bfs(
    AvenGraph g,
    uint32_t start_vertex,
    uint32_t target_vertex,
    AvenArena *arena
) {
    AvenGraphBfsCtx ctx = aven_graph_bfs_init(
        g,
        start_vertex,
        target_vertex,
        arena
    );

    do {
        // code to call at each step of BFs
    } while (!aven_graph_bfs_step(&ctx, g));

    return aven_graph_bfs_result(&ctx, arena);
}

#endif // AVEN_GRAPH_BFS_H

