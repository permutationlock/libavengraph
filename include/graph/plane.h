#ifndef GRAPH_PLANE_H
#define GRAPH_PLANE_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/math.h>

#include "../graph.h"

typedef Slice(Vec2) GraphPlaneEmbedding;

// Verify simple adjacency list graph represents a combinatorial embedding

static inline bool graph_plane_aug_validate(
    GraphAug graph,
    AvenArena temp_arena
) {
    if (graph.adj.len <= 1) {
        return true;
    }

    Slice(bool) visited = aven_arena_create_slice(
        bool,
        &temp_arena,
        graph.nb.len
    );
    for (uint32_t i = 0; i < visited.len; i += 1) {
        get(visited, i) = false;
    }

    uint32_t vertices = (uint32_t)graph.adj.len;
    uint32_t edges = (uint32_t)graph.nb.len / 2;

    if (edges > 3 * vertices - 6) {
        return false;
    }
    
    uint32_t faces = 0;

    for (uint32_t v = 0; v < graph.adj.len; v += 1) {
        GraphAdj v_adj = get(graph.adj, v);
        for (uint32_t i = 0; i < v_adj.len; i += 1) {
            uint32_t count = 0;
            uint32_t u = v;
            GraphAdj u_adj = v_adj;
            uint32_t uw_index = i;
            GraphAugNb uw = graph_aug_nb(graph.nb, u_adj, uw_index);

            while (
                count < graph.adj.len and
                !get(visited, u_adj.index + uw_index)
            ) {
                count += 1;
                get(visited, u_adj.index + uw_index) = true;

                if (uw.vertex == v) {
                    break;
                }

                u = uw.vertex;
                u_adj = get(graph.adj, u);
                uw_index = graph_adj_next(u_adj, uw.back_index);
                uw = graph_aug_nb(graph.nb, u_adj, uw_index);
            }

            if (count > 0) {
                if (uw.vertex != v) {
                    return false;
                }
                if (graph_adj_next(v_adj, uw.back_index) != i) {
                    return false;
                }

                faces += 1;
            }
        }
    }

    for (uint32_t i = 0; i < visited.len; i += 1) {
        if (!get(visited, i)) {
            return false;
        }
    }

    return (faces == (2 + edges - vertices));
}

static inline bool graph_plane_validate(Graph graph, AvenArena temp_arena) {
    GraphAug aug_graph = graph_aug(graph, &temp_arena);
    return graph_plane_aug_validate(aug_graph, temp_arena);
}

#endif // AVEN_PLANE_H

