#ifndef GRAPH_GEN_H
#define GRAPH_GEN_H

#include <aven.h>
#include <aven/arena.h>
#include "../graph.h"

static inline Graph graph_gen_complete(
    uint32_t size,
    AvenArena *arena
) {
    Graph graph = {
        .nb = { .len = size * (size - 1) },
        .adj = { .len = size },
    };
    graph.nb.ptr = aven_arena_create_array(uint32_t, arena, graph.nb.len);
    graph.adj.ptr = aven_arena_create_array(GraphAdj, arena, graph.adj.len);

    uint32_t i = 0;
    for (uint32_t v = 0; v < graph.adj.len; v += 1) {
        get(graph.adj, v).len = size - 1;
        get(graph.adj, v).index = i;

        for (uint32_t u = 0; u < graph.adj.len; u += 1) {
            if (u == v) {
                continue;
            }
            get(graph.nb, i) = u;
            i += 1;
        }
    }

    return graph;
}

static inline Graph graph_gen_grid(
    uint32_t width,
    uint32_t height,
    AvenArena *arena
) {
    Graph graph = {
        .nb = {
            .len = 4 * (width - 2) * (height - 2) +
                6 * (width - 2) +
                6 * (height - 2) +
                8,
        },
        .adj = { .len = width * height },
    };
    graph.nb.ptr = aven_arena_create_array(uint32_t, arena, graph.nb.len);
    graph.adj.ptr = aven_arena_create_array(GraphAdj, arena, graph.adj.len);

    uint32_t i = 0;
    for (uint32_t v = 0; v < graph.adj.len; v += 1) {
        GraphAdj *v_adj = &get(graph.adj, v);
        v_adj->index = i;

        uint32_t x = v % width;
        uint32_t y = v / width;

        if (x > 0) {
            get(graph.nb, i) = (x - 1) + y * width;
            i += 1;
        }
        if (x < width - 1) {
            get(graph.nb, i) = (x + 1) + y * width;
            i += 1;
        }
        if (y > 0) {
            get(graph.nb, i) = x + (y - 1) * width;
            i += 1;
        }
        if (y < height - 1) {
            get(graph.nb, i) = x + (y + 1) * width;
            i += 1;
        }

        v_adj->len = i - v_adj->index;
    }

    return graph;
}

#endif // GRAPH_GEN_H

