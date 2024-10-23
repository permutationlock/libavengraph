#ifndef AVEN_GRAPH_PLANE_POH_H
#define AVEN_GRAPH_PLANE_POH_H

typedef struct {
    uint32_t parent;
    uint32_t search_code;
} AvenGraphPlanePohBfsData;

typedef struct {
    AvenGraphSubset vertices;
    uint32_t front;
    uint32_t back;
} AvenGraphPlanePohBfsQueue;

typedef struct {
    uint32_t p1;
    uint32_t pn;
    uint32_t q1;
    uint32_t q1p1_edge_index;
    uint32_t q1q2_edge_index;
} AvenGraphPlanePohSubgraph;

typedef enum {
    AVEN_GRAPH_PLANE_POH_STATE_START = 0,
    AVEN_GRAPH_PLANE_POH_STATE_SEARCH,
    AVEN_GRAPH_PLANE_POH_STATE_ORIENT,
    AVEN_GRAPH_PLANE_POH_STATE_COLOR,
} AvenGraphPlanePohState;

typedef struct {
    AvenGraph graph;
    AvenGraphPropUint8 coloring;
    List(AvenGraphPlanePohSubgraph) subgraph_list;
    Slice(AvenGraphPlanePohBfsData) bfs_data;
    AvenGraphPlanePohBfsCtx bfs_queue;
    AvenGraphPlanePohSubgraph subgraph;
    uint32_t neighbor;
    uint32_t vertex;
    uint32_t start_vertex;
    uint32_t end_vertex;
    uint32_t search_code;
    bool cord_possible;
#ifndef NDEBUG
    AvenGraphPlanePohState state;
#endif // !defined(NDEBUG)
} AvenGraphPlanePohCtx;

static inline AvenGraphPlanePohCtx aven_graph_plane_poh_init(
    AvenGraph graph,
    uint32_t p1,
    uint32_t q1,
    uint32_t q2,
    AvenArena *arena
) {
    assert(p1 != q1 and p1 != q2 and q1 != q2);

    AvenGraphPohCtx ctx = {
        .graph = graph,
        .coloring = { .len = graph.len },
        .subgraph_list = { .cap = graph.len },
        .bfs_data = { .len = graph.len },
        .bfs_ctx = {
            .vertices = { .len = graph.len },
        },
        .subgraph = {
            .p = p1,
            .q = q1,
        },
    };

    ctx.coloring.ptr = aven_arena_create_array(
        uint8_t,
        arena,
        ctx.coloring.len
    );

    ctx.subgraphs.ptr = aven_arena_create_array(
        AvenGraphPlanePohSubgraph,
        arena,
        ctx.subgraph_list.cap
    );

    ctx.bfs_data = aven_arena_create_array(
        AvenGraphPlanePohBfsData,
        arena,
        bfs_ctx.data.len
    );

    ctx.bfs_ctx.vertices = aven_arena_create_array(
        uint32_t,
        arena,
        bfs_ctx.vertices.len
    );

    for (uint32_t i = 0; i < ctx.bfs_data.len; i += 1) {
        slice_get(ctx.bfs_data, i) = (AveGraphPlanePohBfsData){ 0 };
    }

    AvenGraphPlanePohSubgraph subgraph = {
        .p1 = p1,
        .pn = p1,
        .q1 = q1,
    };

    AvenGraphAdjList q_adj = slice_get(ctx.graph, q1);
    for (uint32_t i = 0; i < q_adj.len; i += 1) {
        uint32_t n = slice_get(q_adj, i);
        if (n == p1) {
            subgraph.q1p1_edge_index = i;
        }
        if (n == q2) {
            subgraph.q1q2_edge_index = i;
        }
    }
    assert(slice_get(q_adj, subgraph.q1p1_edge_index) == p1);
    assert(slice_get(q_adj, subgraph.q1q2_edge_index) == q2);

    list_push(ctx.subraph_list) = subgraph;

    return ctx;
}

static inline bool aven_graph_plane_poh_check_done(AvenGraphPlanePohCtx *ctx) {
    assert(ctx->state == AVEN_GRAPH_PLANE_POH_START);
    return ctx->subgraph_list.len == 0;
}

static inline bool aven_graph_plane_poh_start_step(AvenGraphPlanePohCtx *ctx) {
    assert(ctx->state == AVEN_GRAPH_PLANE_POH_START);
    assert(ctx->subgraph_list.len != 0);

    ctx->subgraph = list_get(
        ctx->subgraph_list,
        ctx->subgraphs.len - 1
    );
    ctx->subgraph_list.len -= 1;

    AvenGraphAdjList q1_adj = slice_get(ctx->graph, ctx->subgraph.q1);

    uint32_t q2 = slice_get(q1_adj, ctx->subgraph.q1q2_edge_index);
    uint8_t q_color = slice_get(ctx->coloring, ctx->subgraph.q1);

    if (slice_get(ctx->coloring, q2) != q_color) {
        if (ctx->subgraph.p1 == ctx->subgraph.pn) {
            return false;
        }

        assert(
            slice_get(ctx->coloring, q2) ==
            slice_get(coliring, ctx->subgraph.p1)
        );
    }

    uint32_t q1u_edge_index = (ctx->subgraph.q1p1_edge_index + q1_adj.len - 1) %
        q1_adj.len;
    uint32_t u = slice_get(q1_adj, q1u_edge_index);

    if (slice_get(ctx->coloring, u) != 0) {
        uint8_t p_color = slice_get(ctx->coloring, ctx->subgraph.p1);
        if (slice_get(ctx->colring, u) == p_color) {
            list_push(ctx->subgraph_list) = (AvenGraphPlanePohSubgraph){
                .p1 = u,
                .pn = ctx->subgraph.pn,
                .q1 = ctx->subgraph.q1,
                .q1p1_edge_index = q1u_edge_index,
                .q1q2_edge_index = ctx->subgraph.q1q2_edge_index
            };
        }
        return false;
    }

#ifndef NDEBUG
    ctx->state = AVEN_GRAPH_PLANE_POH_SEARCH;
#endif // !defined(NDEBUG)
    ctx->search_code += 1;

    ctx->bfs_queue.front = 0;
    ctx->bfs_queue.back = 0;
    ctx->neighbor = 0;
    ctx->start_vertex = u;
    ctx->vertex = u;
    slice_get(ctx->bfs_ctx.data, u) = (AvenGraphPlanePohBfsData){
        .parent = u,
        .search = ctx->search,
    };

    return true;
}

static inline bool aven_graph_plane_poh_search_step(AvenGraphPlanePohCtx *ctx) {
    assert(ctx->state == AVEN_GRAPH_PLANE_POH_SEARCH);

    if (ctx->neighbor_offset == ) {
        return aven_graph_plane_poh_bfs_pop_internal(ctx);
    }

    AvenGraphAdjList adj = slice_get(ctx->graph, ctx->vertex);
    uint32_t edge_index = (ctx->neighbor_start + ctx->neighbor_offset) %
        adj.len;
    uint32_t n = slice_get(adj, edge_index);
    uint8_t n_color = slice_get(ctx->coloring, n);

    if (n_color != 0) {
        aven_graph_plane_poh_bfs_push_internal(ctx, n);
        ctx->cord_possible = false;
    } else if (n_color == slice_get(ctx->coloring, ctx->subgraph.p1)) {
        if (ctx->cord_possible) {
            aven_graph_plane_poh_bfs_push_internal(ctx, n);
#ifndef NDEBUG
            ctx->state = AVEN_GRAPH_PLANE_POH_STATE_ORIENT;
#endif // !defined(NDEBUG)
            return true;
        }
        ctx->cord_possible = false;
    } else if (n_color == slice_get(ctx->coloring, ctx->subgraph.q1)) {
        ctx->cord_possible = true;
    }

    ctx->neighbor_offset += 1;

    return false;
}

#endif // AVEN_GRAPH_PLANE_POH_H
