#ifndef AVEN_GRAPH_H
#define AVEN_GRAPH_H

#include <aven.h>

typedef Slice(uint32_t) AvenGraphAdjList;
typedef Slice(AvenGraphAdjList) AvenGraph;

typedef Slice(uint32_t) AvenGraphSubset;
typedef Slice(uint64_t) AvenGraphPropUint64;
typedef Slice(uint32_t) AvenGraphPropUint32;
typedef Slice(uint16_t) AvenGraphPropUint16;
typedef Slice(uint8_t) AvenGraphPropUint8;

typedef Slice(uint8_t) AvenGraphColorSlice;

typedef struct{
    uint32_t vertex;
    uint32_t back_index;
} AvenGraphAugAdjListNode;

typedef Slice(AvenGraphAugAdjListNode) AvenGraphAugAdjList;
typedef Slice(AvenGraphAugAdjList) AvenGraphAug;

static inline uint32_t aven_graph_neighbor_index(
    AvenGraph graph,
    uint32_t vertex,
    uint32_t neighbor
) {
    AvenGraphAdjList adj = slice_get(graph, vertex);
    for (uint32_t i = 0; i < adj.len; i += 1) {
        if (slice_get(adj, i) == neighbor) {
            return i;
        }
    }
    assert(false);
}

static inline uint32_t aven_graph_next_neighbor_index(
    AvenGraph graph,
    uint32_t vertex,
    uint32_t neighbor
) {
    AvenGraphAdjList adj = slice_get(graph, vertex);
    for (uint32_t i = 0; i + 1 < adj.len; i += 1) {
        if (slice_get(adj, i) == neighbor) {
            return i + 1;
        }
    }
    assert(slice_get(adj, adj.len - 1) == neighbor);

    return 0;
}

#endif // AVEN_GRAPH_H

