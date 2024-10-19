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

#endif // AVEN_GRAPH_H

