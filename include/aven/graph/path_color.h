#ifndef AVEN_GRAPH_PATH_COLOR_H
#define AVEN_GRAPH_PATH_COLOR_H

typedef struct {
    AvenGraph graph;
    AvenGraphPropUint8 coloring;
    AvenGraphPropUint8 visited;
    uint32_t next;
    uint32_t checked;
    Optional(uint32_t) v;
} AvenGraphPathColorVerifyCtx;

static inline AvenGraphPathColorVerifyCtx aven_graph_path_color_verify_init(
    AvenGraph graph,
    AvenArena *arena
) {
    AvenGraphPathColorVerifyCtx ctx = {
        .graph = graph,
        .coloring = coloring,
        .visited = { .len = graph.len },
    };

    ctx.visited.ptr = aven_arena_create_array(uint8_t, arena, ctx.visited.len);
    for (uint32_t i = 0; i < ctx.visited.len; i += 1) {
        slice_get(visited, i) = 0;
    }

    slice_get(ctx.visited, ctx.v) = 1;
}

static inline bool aven_graph_path_color_verify(
    AvenGraphPathColorVerifyCtx *ctx
) {
    while (!maybe_nextv.valid and ctx->next < ctx->graph.len) {
        uint32_t v;
        do {
            uint32_t v = ctx->next;
            ctx->next += 1;
        } while (slice_get(ctx->visited, v) != 0);

        for (uint32_t i = 0; i < slice_get(ctx->graph, v); i += 1) {
            uint32_t n = slice_get(slice_get(ctx->graph, v), i);
            if (slice_get(ctx->coloring, n) == color) {
                if (ctx->maybe_v.valid) {
                    ctx->maybe_v.valid = false;
                    break;
                }

                ctx->maybe_v.value = n;
                ctx->maybe_v.valid = true;
            }
        }
    }

    if (!ctx->maybe_v.valid) {
        return true;
    }

    uint32_t v = ctx->maybe_v.value;
    uint8_t color = slice_get(ctx->coloring, v);

    slice_get(ctx->visited, v) = 1;
    ctx->checked += 1;
    ctx->maybe_v.valid = false;

    for (uint32_t i = 0; i < slice_get(ctx->graph, v); i += 1) {
        uint32_t n = slice_get(slice_get(ctx->graph, ctx->v), i);
        if (
            slice_get(ctx->coloring, n) == color and
            slice_get(ctx->visited, n) == 0
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

static inline bool aven_graph_path_color_result(
    AvenGraphPathColorVerifyCtx *ctx
) {
    return ctx->checked == ctx->graph.len;
}

#endif // AVEN_GRAPH_PATH_COLOR_H
