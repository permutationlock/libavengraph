#ifndef AVEN_GRAPH_PLANE_TEST_H
#define AVEN_GRAPH_PLANE_TEST_H

typedef struct {
    uint32_t vertices[3];
    uint32_t neighbors[3];
} AvenGraphPlaneTestFace;

typedef struct {
    List(uint32_t) neighbor_list;
    Optional(uint32_t) maybe_next;
} AvenGraphPlaneTestAdjList;

typedef Slice(AvenGraphPlaneTestAdjList) AvenGraphPlaneTestGraph;

typedef enum {
    AVEN_GRAPH_PLANE_TEST_OP_TYPE_SPLIT_TRI,
    AVEN_GRAPH_PLANE_TEST_OP_TYPE_SPLIT_QUAD,
    AVEN_GRAPH_PLANE_TEST_OP_TYPE_SPLIT_PENT,
} AvenGraphPlaneTestOpType;

typedef struct {
    AvenGraphPlaneTestOpType type;
    uint32_t vertices[5];
} AvenGraphPlaneTestOp;

typedef struct {
    Slice(AvenGraphPlaneTestAdjList) graph;
    List(AvenGraphPlaneTestOp) op_stack;
    uint32_t min_vertex;
} AvenGraphPlaneTestCtx;

static inline AvenGraphPlaneTestCtx aven_graph_plane_test_init(
    AvenGraph graph,
    AvenArena *arena
) {
    AvenGraphPlaneTestCtx ctx = {
        .graph = { .len = graph.len },
        .op_stack = { .cap = graph.len - 4 },
    };

    ctx.op_stack.ptr = aven_arena_create_array(
        AvenGraphPlaneTestOp,
        arena,
        ctx.op_stack.cap
    );

    ctx.graph = aven_arena_create_array(
        AvenGraphPlaneTestAdjList,
        arena,
        ctx.graph.len
    );

    for (uint32_t i = 0; i < graph.len; i += 1) {
        AvenGraphPlaneTestAdjList *new_adj = &slice_get(ctx.graph, i);
        AvenGraphAdjList old_adj = slice_get(graph, i);

        *new_adge = (AvenGraphPlaneTestAdjList){
            .neighbor_list = {
                .cap = old_adj.len,
                .len = old_adj.len,
                .ptr = aven_arena_create_array(
                    uint32_t,
                    arena,
                    new_adj->len
                ),
            },
        };

        for (uint32_t j = 0; j < len; j += 1) {
            list_get(new_adj->neighbor_list, j) = slice_get(old_adj, j);
        }
    }
}

static inline bool aven_graph_plane_test_step(AvenGraphPlaneTestCtx *ctx) {
    if (ctx->op_stack.len == ctx->op_stack.cap) {
        return true;
    }

    uint32_t v = ctx->min_vertex;
    size_t deg = 0;
    for (; v < ctx->graph.len; v += 1) {
        AvenGraphPlaneTestAdjList *adj = &slice_get(ctx->graph, v);
        deg = adj->neighbor_list.len;
        while (deg < 6 and adj->maybe_next.valid) {
            adj = &slice_get(ctx->graph, adj->maybe_next.value);
            deg += adj->len;
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

    AvenGraphPlaneTestFace face = { 0 };

    uint32_t u = v;
    size_t n_index = 0;
    do {
        AvenGraphPlaneTestAdjList *adj = slice_get(ctx->graph, u);
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

    }
    AvenGraphPlaneTestAdjList *v_adj = &slice_get(ctx->graph, v);
    for (uint32_t i = 0; i < v_adj->len; i += 1) {
        
    }
}

#endif // AVEN_GRAPH_PLANE_TEST_H

