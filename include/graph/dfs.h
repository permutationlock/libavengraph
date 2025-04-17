#ifndef GRAPH_DFS_H
#define GRAPH_DFS_H

#include <aven.h>
#include <aven/arena.h>

#include "../graph.h"

typedef struct {
    uint32_t vertex;
    uint32_t edge_index;
} GraphDfsFrame;

typedef struct {
    uint32_t parent;
    uint32_t number;
    uint32_t least_ancestor;
    uint32_t lowpoint;
} GraphDfsTreeNode;
typedef Slice(GraphDfsTreeNode) GraphDfsTree;
typedef Slice(uint32_t) GraphDfsNumbering;
typedef struct {
    GraphDfsNumbering numbering;
    GraphDfsTree tree;
} GraphDfsData;

typedef struct {
    GraphAdj adj;
    GraphDfsTreeNode node;
} GraphDfsVertex;

typedef struct {
    GraphNbSlice nb;
    Slice(GraphDfsVertex) vertex_info;
    List(uint32_t) dfs_numbering;
    List(GraphDfsFrame) dfs_list;
} GraphDfsCtx;

static inline GraphDfsCtx graph_dfs_init(
    Graph graph,
    uint32_t root_vertex,
    AvenArena *arena
) {
    assert(root_vertex < graph.adj.len);

    GraphDfsCtx ctx = {
        .nb = graph.nb,
        .dfs_list = { .cap = graph.adj.len },
        .dfs_numbering = { .cap = graph.adj.len },
        .vertex_info = { .len = graph.adj.len },
    };

    ctx.vertex_info.ptr = aven_arena_create_array(
        GraphDfsVertex,
        arena,
        ctx.vertex_info.len
    );
    ctx.dfs_numbering.ptr = aven_arena_create_array(
        uint32_t,
        arena,
        ctx.dfs_numbering.cap
    );
    ctx.dfs_list.ptr = aven_arena_create_array(
        GraphDfsFrame,
        arena,
        ctx.dfs_list.cap
    );

    for (uint32_t v = 0; v < ctx.vertex_info.len; v += 1) {
        get(ctx.vertex_info, v) = (GraphDfsVertex){
            .adj = get(graph.adj, v),
        };
    }

    get(ctx.vertex_info, root_vertex).node.parent = root_vertex + 1;
    list_push(ctx.dfs_numbering) = root_vertex;
    list_push(ctx.dfs_list) = (GraphDfsFrame){ .vertex = root_vertex };

    return ctx;
}

static inline bool graph_dfs_step(GraphDfsCtx *ctx) {
    if (ctx->dfs_list.len == 0) {
        return true;
    }

    GraphDfsFrame *frame = &list_back(ctx->dfs_list);
    GraphDfsVertex *v_info = &get(ctx->vertex_info, frame->vertex);
    if (frame->edge_index == v_info->adj.len) {
        uint32_t p = v_info->node.parent - 1;
        if (p != frame->vertex) {
            GraphDfsVertex *p_info = &get(ctx->vertex_info, p);
            p_info->node.lowpoint = min(
                p_info->node.lowpoint,
                v_info->node.lowpoint
            );
        }
        list_pop(ctx->dfs_list);
        return false;
    }
    
    uint32_t u = graph_nb(ctx->nb, v_info->adj, frame->edge_index);
    GraphDfsVertex *u_info = &get(ctx->vertex_info, u);
    if (u_info->node.parent == 0) {
        u_info->node.number = (uint32_t)ctx->dfs_numbering.len;
        u_info->node.least_ancestor = u_info->node.number;
        u_info->node.lowpoint = u_info->node.number;
        u_info->node.parent = frame->vertex + 1;

        list_push(ctx->dfs_list) = (GraphDfsFrame){
            .vertex = u,
            .edge_index = 0,
        };
        list_push(ctx->dfs_numbering) = u;
    } else if (u != v_info->node.parent - 1) {
        v_info->node.least_ancestor = min(
            v_info->node.least_ancestor,
            u_info->node.number
        );
        v_info->node.lowpoint = min(
            v_info->node.lowpoint,
            u_info->node.lowpoint
        );
    }
    frame->edge_index += 1;

    return false;
}

static inline GraphDfsData graph_dfs(
    Graph graph,
    uint32_t root_vertex,
    AvenArena *arena
) {
    GraphDfsTree tree = aven_arena_create_slice(
        GraphDfsTreeNode,
        arena,
        graph.adj.len
    );
    List(uint32_t) numbering = aven_arena_create_list(
        uint32_t,
        arena,
        graph.adj.len
    );

    AvenArena temp_arena = *arena;
    GraphDfsCtx ctx = graph_dfs_init(
        graph,
        root_vertex,
        &temp_arena
    );

    while (!graph_dfs_step(&ctx)) {};

    for (uint32_t v = 0; v < tree.len; v += 1) {
        get(tree, v) = get(ctx.vertex_info, v).node;
    }

    for (uint32_t n = 0; n < ctx.dfs_numbering.len; n += 1) {
        list_push(numbering) = get(ctx.dfs_numbering, n);
    }
    
    return (GraphDfsData){
        .tree = tree,
        .numbering = aven_arena_commit_list_to_slice(
            GraphDfsNumbering,
            arena,
            numbering
        ),
    };
}

static inline bool graph_dfs_tree_contains(
    GraphDfsTree tree,
    uint32_t v
) {
    return get(tree, v).parent != 0;
}

static inline uint32_t graph_dfs_tree_parent(GraphDfsTree tree, uint32_t v) {
    assert(graph_dfs_tree_contains(tree, v));
    return get(tree, v).parent - 1;
}

static inline GraphSubset graph_dfs_tree_path_to_root(
    GraphDfsTree tree,
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
            v = graph_dfs_tree_parent(tree, v);
        } while (last_v != v);
    }

    return aven_arena_commit_list_to_slice(GraphSubset, arena, path_list);
}

#endif // GRAPH_DFS_H
