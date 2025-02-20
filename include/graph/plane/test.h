#ifndef GRAPH_PLANE_TEST_H
#define GRAPH_PLANE_TEST_H

#include <aven.h>
#include <aven/arena.h>

#include "../graph.h"

typedef struct  {
    uint32_t vertex;
    uint32_t back_index;
    uint32_t next_index;
    uint32_t prev_index;
    Optional(uint32_t) rh_face;
} GraphPlaneTestAdjNode;

typedef struct {
    uint32_t last_index;
    uint32_t len;
} GraphPlaneTestAdjList;

typedef struct {
    uint32_t nodes[3];
    uint32_t neighbors[3];
} GraphPlaneTestFace;

typedef struct {
    Slice(GraphPlaneTestAdjList) adj;
    Pool(GraphPlaneTestAdjNode) nodes;
    Pool(GraphPlaneTestFace) faces;
} GraphPlaneTestGraph;

static inline uint32_t graph_plane_test_adj_len(
    GraphPlaneTestGraph *graph,
    uint32_t v
) {
    return get(graph->adj, v).len;
}

static inline uint32_t graph_plane_test_adj_last(
    GraphPlaneTestGraph *graph,
    uint32_t v
) {
    assert(graph_plane_test_adj_len(graph, v) > 0);
    return get(graph->adj, v).last_index;
}

static inline GraphPlaneTestAdjNode *graph_plane_test_adj_node(
    GraphPlaneTestGraph *graph,
    uint32_t i
) {
    return &pool_get(graph->nodes, i);
}

static inline void graph_plane_test_adj_insert_edge(
    GraphPlaneTestGraph *graph,
    uint32_t u,
    uint32_t un_index,
    uint32_t v,
    uint32_t vm_index
) {
    uint32_t uv_index = (uint32_t)pool_create(graph->nodes);
    uint32_t vu_index = (uint32_t)pool_create(graph->nodes);
    GraphPlaneTestAdjNode *uv_node = graph_plane_test_adj_node(graph, uv_index);
    GraphPlaneTestAdjNode *vu_node = graph_plane_test_adj_node(graph, vu_index);
    GraphPlaneTestAdjNode *un_node = graph_plane_test_adj_node(graph, un_index);
    GraphPlaneTestAdjNode *vm_node = graph_plane_test_adj_node(graph, vm_index);
    *uv_node = (GraphPlaneTestAdjNode){
        .back_index = vu_index,
        .next_index = un_node->next_index,
        .prev_index = un_index,
        .vertex = v,
    };
    *vu_node = (GraphPlaneTestAdjNode){
        .back_index = uv_index,
        .next_index = vm_node->next_index,
        .prev_index = vm_index,
        .vertex = u,
    };
    un_node->next_index = uv_index;
    vm_node->next_index = vu_index;
}

static inline void graph_plane_test_adj_delete_edge(
    GraphPlaneTestGraph *graph,
    uint32_t u,
    uint32_t uv_index
) {
    GraphPlaneTestAdjNode *uv_node = graph_plane_test_adj_node(graph, uv_index);
    GraphPlaneTestAdjNode *vu_node = graph_plane_test_adj_node(
        graph,
        uv_node->back_index
    );

    GraphPlaneTestAdjNode *uv_pnode = graph_plane_test_adj_node(
        graph,
        uv_node->prev_index
    );
    GraphPlaneTestAdjNode *uv_nnode = graph_plane_test_adj_node(
        graph,
        uv_node->prev_index
    );
    uv_pnode->next_index = uv_node->next_index;
    uv_nnode->prev_index = uv_node->prev_index;

    GraphPlaneTestAdjNode *vu_pnode = graph_plane_test_adj_node(
        graph,
        vu_node->prev_index
    );
    GraphPlaneTestAdjNode *vu_nnode = graph_plane_test_adj_node(
        graph,
        vu_node->prev_index
    );
    vu_pnode->next_index = vu_node->next_index;
    vu_nnode->prev_index = vu_node->prev_index;

    if (get(graph->adj, u).last_index == uv_index) {
        get(graph->adj, u).last_index = uv_node->next_index;
    }
    get(graph->adj, u).len -= 1;

    if (get(graph->adj, uv_node->vertex).last_index == uv_node->back_index) {
        get(graph->adj, uv_node->vertex).last_index = vu_node->next_index;
    }
    get(graph->adj, uv_node->vertex).len -= 1;

    pool_delete(graph->nodes, uv_node->back_index);
    pool_delete(graph->nodes, uv_index);
}

typedef struct {
    uint32_t vertices[3];
    uint32_t neighbors[3];
} GraphPlaneTestFace;

typedef struct {
    List(uint32_t) neighbor_list;
    Optional(uint32_t) maybe_next;
} GraphPlaneTestAdjList;

typedef Slice(GraphPlaneTestAdjList) GraphPlaneTestGraph;

typedef enum {
    GRAPH_PLANE_TEST_OP_TYPE_SPLIT_TRI,
    GRAPH_PLANE_TEST_OP_TYPE_SPLIT_QUAD,
    GRAPH_PLANE_TEST_OP_TYPE_SPLIT_PENT,
} GraphPlaneTestOpType;

typedef struct {
    GraphPlaneTestOpType type;
    uint32_t vertex;
    uint32_t face;
    uint32_t edge;
} GraphPlaneTestOp;

typedef struct {
    uint32_t degree;
    uint32_t face;
} GraphPlaneTestVertex;

typedef struct {
    Slice(GraphPlaneTextFace) faces;
    Slice(GraphPlaneTestVertex) vertices;
    List(GraphPlaneTestOp) op_stack;
} GraphPlaneTestCtx;

static inline GraphPlaneTestCtx graph_plane_test_init(
    Graph graph,
    AvenArena *arena
) {
    GraphPlaneTestCtx ctx = {
        .graph = { .len = graph.len },
        .op_stack = { .cap = graph.len - 4 },
    };

    ctx.op_stack.ptr = aven_arena_create_array(
        GraphPlaneTestOp,
        arena,
        ctx.op_stack.cap
    );

    ctx.graph = aven_arena_create_array(
        GraphPlaneTestAdjList,
        arena,
        ctx.graph.len
    );

    for (uint32_t i = 0; i < graph.len; i += 1) {
        GraphPlaneTestAdjList *new_adj = &get(ctx.graph, i);
        GraphAdjList old_adj = get(graph, i);

        *new_adge = (GraphPlaneTestAdjList){
            .neighbor_list = {
                .cap = old_adj.len,
                .ptr = aven_arena_create_array(
                    uint32_t,
                    arena,
                    new_adj->len
                ),
            },
        };

        for (uint32_t j = 0; j < len; j += 1) {
            list_push(new_adj->neighbor_list) = get(old_adj, j);
        }
    }
}

static inline bool graph_plane_test_step(GraphPlaneTestCtx *ctx) {
    if (ctx->op_stack.len == ctx->op_stack.cap) {
        return true;
    }

    uint32_t v = ctx->min_vertex;
    uint32_t deg = 0;
    for (; v < ctx->graph.len; v += 1) {
        GraphPlaneTestAdjList *adj = &get(ctx->graph, v);
        deg = adj->neighbor_list.len;
        while (deg < 6 and adj->maybe_next.valid) {
            adj = &get(ctx->graph, adj->maybe_next.value);
            deg += (uint32_t)adj->len;
        }

        if (deg < 3) {
            return true;
        }

        if (deg < 6) {
            break;
        }
    }

    if (v == (uint32_t)ctx->graph.len) {
        return true;
    }

    GraphPlaneTestFace face = { 0 };

    uint32_t u = v;
    uint32_t n_index = 0;
    do {
        GraphPlaneTestAdjList *adj = get(ctx->graph, u);
        for (uint32_t i = 0; i < adj->neighbor_list.len; i += 1) {
            face.vertices[n_index] = list_get(adj->neighbor_list, i);
            n_index += 1;
        }

        if (!adj->maybe_next.valid) {
            break;
        }
        u = adj->mayb_next.value;
    } while (n_index < deg);

    assert(n_index == deg);

    switch (deg) {
        case 3:
            for (uint32_t i = 0; i < 3; i += 1) {
                uint32_t n = face.vertices[i];
                GraphPlaneTestAdjList *n_adj = get(ctx->graph, n);
            }
            break;
        case 4:
            break;
        case 5:
            break;
    }
    GraphPlaneTestAdjList *v_adj = &get(ctx->graph, v);
    for (uint32_t i = 0; i < v_adj->len; i += 1) {
        
    }
}

#endif // GRAPH_PLANE_TEST_H

