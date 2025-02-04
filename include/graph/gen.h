#ifndef GRAPH_GEN_H
#define GRAPH_GEN_H

#include <aven.h>
#include <aven/arena.h>
#include "../graph.h"

static inline Graph graph_gen_complete(
    uint32_t size,
    AvenArena *arena
) {
    Graph g = {
        .len = size,
        .ptr = aven_arena_create_array(GraphAdjList, arena, size),
    };

    for (uint32_t v = 0; v < g.len; v += 1) {
        get(g, v).len = g.len - 1;
        get(g, v).ptr = aven_arena_create_array(
            uint32_t,
            arena,
            get(g, v).len
        );

        size_t i = 0;
        GraphAdjList vlist = get(g, v);
        for (uint32_t u = 0; u < g.len; u += 1) {
            if (v == u) {
                continue;
            }

            get(vlist, i) = u;
            i += 1;
        }
    }

    return g;
}

static inline Graph graph_gen_grid(
    uint32_t width,
    uint32_t height,
    AvenArena *arena
) {
    Graph g = { .len = width * height };
    g.ptr = aven_arena_create_array(GraphAdjList, arena, g.len);

    for (uint32_t v = 0; v < g.len; v += 1) {
        GraphAdjList *adj = &get(g, v);
        adj->len = 4;
        adj->ptr = aven_arena_create_array(
            uint32_t,
            arena,
            adj->len
        );

        uint32_t x = v % width;
        uint32_t y = v / width;

        uint32_t i = 0;
        if (x > 0) {
            get(*adj, i) = (x - 1) + y * width;
            i += 1;
        }
        if (x < width - 1) {
            get(*adj, i) = (x + 1) + y * width;
            i += 1;
        }
        if (y > 0) {
            get(*adj, i) = x + (y - 1) * width;
            i += 1;
        }
        if (y < height - 1) {
            get(*adj, i) = x + (y + 1) * width;
            i += 1;
        }
        
        adj->len = i;
    }

    return g;
}

#endif // GRAPH_GEN_H

