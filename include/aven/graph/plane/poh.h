#ifndef AVEN_GRAPH_PLANE_POH_H
#define AVEN_GRAPH_PLANE_POH_H

typedef struct {
    uint32_t parent;
    uint32_t search;
} AvenGraphPlanePohBfsData;

typedef struct {
    uint32_t p_start;
    uint32_t q_start;
    uint32_t qp_edge_index;
    uint32_t qn_edge_index;
} AvenGraphPlanePohFrame;

typedef enum {
    AVEN_GRAPH_PLANE_POH_STATE_FIND_START,
    AVEN_GRAPH_PLANE_POH_STATE_FIND_PATH,
} AvenGraphPlanePohState;

typedef union {
    struct {
        uint32_t neighbor_index;
    } find_start;
    struct {
        uint32_t u;
        uint32_t v;
        uint32_t neighbor_index;
    } find_path;
} AvenGraphPlanePohData;

typedef struct {
    AvenGraph graph;
    AvenGraphPropUint8 coloring;
    Slice(AvenGraphPlanePohBfsData) bfs_data;
    uint32_t p_start;
    uint32_t q_start;
    uint32_t q_path_neighbor_index;
    uint32_t search;
    uint32_t colors[3];
} AvenGraphPlanePohCtx;

static inline AvenGraphPlanePohCtx aven_graph_plane_poh_init(
    AvenGraph graph,
    uint32_t p1,
    uint32_t q1,
    uint32_t q2,
    uint8_t c1,
    uint8_t c2,
    uint8_t c3,
    AvenArena *arena
) {
    assert(c1 > 0 and c2 > 0 and c3 > 0);
    assert(p1 != q1);

    AvenGraphPohCtx ctx = {
        .graph = graph,
        .coloring = { .len = graph.len },
        .bfs_data = { .len = graph.len },
        .p_start = p1,
        .q_start = q1,
        .p_end = q2,
        .colors = { c1, c2, c3 },
    };

    AvenGraphAdjList q_adj = slice_get(ctx.graph, ctx.q);
    uint32_t i = 0;
    for (; i < .len; i += 1) {
        uint32_t n = slice_get(q_adj, i);
        if (n == ctx.p) {
            break;
        }
    }
    assert(slice_get(q_adj, i) == p);

    ctx.q_start_edge = (i + 1) % q_adj.len;

    ctx.coloring.ptr = aven_arena_create_array(
        uint8_t,
        arena,
        ctx.coloring.len
    );

    ctx.bfs_data.ptr = aven_arena_create_array(
        AvenGraphPlanePohBfsData,
        arena,
        ctx.bfs_data.len
    );

    return ctx;
}

static inline bool aven_graph_plane_poh_step(AvenGraphPlanePohCtx *ctx) {
    if (!ctx->maybe_u.valid) {
        AvenGraphAdjList q_adj = slice_get(ctx->graph, ctx->q);
        uint32_t last_n = slice_get(q_adj, ctx->start_edge);
        for (uint32_t i = 1; i < q_adj.len; i += 1) {
            uint32_t neighbor_index = (ctx->q_start_edge + i) % q_adj.len;
            uint32_t n = slice_get(q_adj, neighbor_index);

            if (n == ctx->p) {
                ctx->maybe_u.value = last_n;
                ctx->maybe_u.valid = true;
                break;
            }

            last_n = n;
        }

        if !(
    }
}

#endif // AVEN_GRAPH_PLANE_POH_H
