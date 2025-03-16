#ifndef GRAPH_BFS_H
#define GRAPH_BFS_H

#include <aven.h>
#include <aven/arena.h>

#include "../graph.h"

typedef struct {
    uint32_t parent;
    uint32_t dist;    
} GraphBfsTreeNode;
typedef Slice(GraphBfsTreeNode) GraphBfsTree;

typedef struct {
    GraphAdj adj;
    GraphBfsTreeNode node;
} GraphBfsVertex;

typedef struct {
    GraphNbSlice nb;
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
    assert(root_vertex < graph.adj.len);

    GraphBfsCtx ctx = {
        .nb = graph.nb,
        .vertex = root_vertex,
        .bfs_queue = { .cap = graph.adj.len },
        .vertex_info = { .len = graph.adj.len },
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
        get(ctx.vertex_info, v) = (GraphBfsVertex){
            .adj = get(graph.adj, v),
        };
    }

    get(ctx.vertex_info, ctx.vertex).node.parent = ctx.vertex + 1;

    return ctx;
}

static inline bool graph_bfs_step(GraphBfsCtx *ctx) {
    GraphBfsVertex *v_info = &get(ctx->vertex_info, ctx->vertex);
    if (ctx->edge_index == v_info->adj.len) {
        if (ctx->bfs_queue.used == 0) {
            return true;
        }

        ctx->vertex = queue_pop(ctx->bfs_queue);
        ctx->edge_index = 0;
        return false;
    }

    uint32_t u = graph_nb(ctx->nb, v_info->adj, ctx->edge_index);
    GraphBfsVertex *u_info = &get(ctx->vertex_info, u);
    if (u_info->node.parent == 0) {
        queue_push(ctx->bfs_queue) = u;
        u_info->node.parent = ctx->vertex + 1;
        u_info->node.dist = v_info->node.dist + 1;
    }
    ctx->edge_index += 1;

    return false;
}

static inline GraphBfsTree graph_bfs(
    Graph graph,
    uint32_t root_vertex,
    AvenArena *arena
) {
    GraphBfsTree tree = aven_arena_create_slice(
        GraphBfsTreeNode,
        arena,
        graph.adj.len
    );

    AvenArena temp_arena = *arena;
    GraphBfsCtx ctx = graph_bfs_init(
        graph,
        root_vertex,
        &temp_arena
    );

    while (!graph_bfs_step(&ctx)) {};

    for (uint32_t v = 0; v < tree.len; v += 1) {
        get(tree, v) = get(ctx.vertex_info, v).node;
    }
    
    return tree;
}

static inline bool graph_bfs_tree_contains(
    GraphBfsTree tree,
    uint32_t v
) {
    return get(tree, v).parent != 0;
}

static inline uint32_t graph_bfs_tree_parent(GraphBfsTree tree, uint32_t v) {
    assert(graph_bfs_tree_contains(tree, v));
    return get(tree, v).parent - 1;
}

static inline GraphSubset graph_bfs_tree_path_to_root(
    GraphBfsTree tree,
    uint32_t v,
    AvenArena *arena
) {
    List(uint32_t) path_list = aven_arena_create_list(
        uint32_t,
        arena,
        tree.len
    );

    if (get(tree, v).parent != 0) {
        uint32_t last_v;
        do {
            list_push(path_list) = v;
            last_v = v;
            assert(get(tree, last_v).parent != 0);
            v = get(tree, last_v).parent - 1;
        } while (last_v != v);
    }

    return aven_arena_commit_list_to_slice(GraphSubset, arena, path_list);
}

#endif // GRAPH_BFS_H
