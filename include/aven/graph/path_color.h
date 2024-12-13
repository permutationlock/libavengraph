#ifndef AVEN_GRAPH_PATH_COLOR_H
#define AVEN_GRAPH_PATH_COLOR_H

typedef struct {
    AvenGraph graph;
    AvenGraphPropUint8 coloring;
    AvenGraphPropUint8 visited;
    uint32_t next;
    uint32_t checked;
    Optional(uint32_t) maybe_v;
} AvenGraphPathColorVerifyCtx;

static inline AvenGraphPathColorVerifyCtx aven_graph_path_color_verify_init(
    AvenGraph graph,
    AvenGraphPropUint8 coloring,
    AvenArena *arena
) {
    AvenGraphPathColorVerifyCtx ctx = {
        .graph = graph,
        .coloring = coloring,
        .visited = { .len = graph.len },
    };

    ctx.visited.ptr = aven_arena_create_array(uint8_t, arena, ctx.visited.len);
    for (uint32_t i = 0; i < ctx.visited.len; i += 1) {
        get(ctx.visited, i) = 0;
    }

    return ctx;
}

static inline bool aven_graph_path_color_verify_step(
    AvenGraphPathColorVerifyCtx *ctx
) {
    while (!ctx->maybe_v.valid and ctx->next < ctx->graph.len) {
        uint32_t v;
        do {
            v = ctx->next;
            ctx->next += 1;
            if (v >= ctx->graph.len) {
                return true;
            }
        } while (get(ctx->visited, v) != 0);

        uint8_t color = get(ctx->coloring, v);
        uint32_t color_degree = 0;

        AvenGraphAdjList v_adj = get(ctx->graph, v);
        for (uint32_t i = 0; i < v_adj.len; i += 1) {
            uint32_t n = get(v_adj, i);
            if (get(ctx->coloring, n) == color) {
                color_degree += 1;
                if (color_degree > 1) {
                    break;
                }
            }
        }

        if (color_degree < 2) {
            ctx->maybe_v.value = v;
            ctx->maybe_v.valid = true;
        }
    }

    if (!ctx->maybe_v.valid) {
        return true;
    }

    uint32_t v = ctx->maybe_v.value;
    uint8_t color = get(ctx->coloring, v);

    get(ctx->visited, v) = 1;
    ctx->checked += 1;
    ctx->maybe_v.valid = false;

    AvenGraphAdjList v_adj = get(ctx->graph, v);
    for (uint32_t i = 0; i < v_adj.len; i += 1) {
        uint32_t n = get(v_adj, i);
        if (
            get(ctx->coloring, n) == color and
            get(ctx->visited, n) == 0
        ) {
            if (ctx->maybe_v.valid) {
                return true;
            }

            ctx->maybe_v.value = n;
            ctx->maybe_v.valid = true;
        }
    }

    return false;
}

static inline bool aven_graph_path_color_verify_result(
    AvenGraphPathColorVerifyCtx *ctx
) {
    return ctx->checked == ctx->graph.len;
}

static inline bool aven_graph_path_color_verify(
    AvenGraph graph,
    AvenGraphPropUint8 coloring,
    AvenArena arena
) {
    AvenGraphPathColorVerifyCtx ctx = aven_graph_path_color_verify_init(
        graph,
        coloring,
        &arena
    );

    while (!aven_graph_path_color_verify_step(&ctx)) {}

    return aven_graph_path_color_verify_result(&ctx);
}

#endif // AVEN_GRAPH_PATH_COLOR_H
