#ifndef GRAPH_PATH_COLOR_H
#define GRAPH_PATH_COLOR_H

#include "../graph.h"

typedef struct {
    Graph graph;
    GraphPropUint8 coloring;
    GraphPropUint8 visited;
    uint32_t next;
    uint32_t checked;
    Optional(uint32_t) maybe_v;
} GraphPathColorVerifyCtx;

static inline GraphPathColorVerifyCtx graph_path_color_verify_init(
    Graph graph,
    GraphPropUint8 coloring,
    AvenArena *arena
) {
    GraphPathColorVerifyCtx ctx = {
        .graph = graph,
        .coloring = coloring,
        .visited = { .len = graph.adj.len },
    };

    ctx.visited.ptr = aven_arena_create_array(uint8_t, arena, ctx.visited.len);
    for (uint32_t i = 0; i < ctx.visited.len; i += 1) {
        get(ctx.visited, i) = 0;
    }

    return ctx;
}

static inline bool graph_path_color_verify_step(
    GraphPathColorVerifyCtx *ctx
) {
    while (!ctx->maybe_v.valid and ctx->next < ctx->graph.adj.len) {
        uint32_t v;
        do {
            v = ctx->next;
            ctx->next += 1;
            if (v >= ctx->graph.adj.len) {
                return true;
            }
        } while (get(ctx->visited, v) != 0);

        uint8_t color = get(ctx->coloring, v);
        uint32_t color_degree = 0;

        GraphAdj v_adj = get(ctx->graph.adj, v);
        for (uint32_t i = 0; i < v_adj.len; i += 1) {
            uint32_t n = graph_nb(ctx->graph.nb, v_adj, i);
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

    GraphAdj v_adj = get(ctx->graph.adj, v);
    for (uint32_t i = 0; i < v_adj.len; i += 1) {
        uint32_t n = graph_nb(ctx->graph.nb, v_adj, i);
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

static inline bool graph_path_color_verify_result(
    GraphPathColorVerifyCtx *ctx
) {
    return ctx->checked == ctx->graph.adj.len;
}

static inline bool graph_path_color_verify(
    Graph graph,
    GraphPropUint8 coloring,
    AvenArena arena
) {
    GraphPathColorVerifyCtx ctx = graph_path_color_verify_init(
        graph,
        coloring,
        &arena
    );

    while (!graph_path_color_verify_step(&ctx)) {}

    return graph_path_color_verify_result(&ctx);
}

#endif // GRAPH_PATH_COLOR_H
