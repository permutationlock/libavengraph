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

static inline AvenGraphAug aven_graph_aug(AvenGraph graph, AvenArena *arena) {
    AvenGraphAug aug_graph = { .len = graph.len };
    aug_graph.ptr = aven_arena_create_array(
        AvenGraphAugAdjList,
        arena,
        aug_graph.len
    );

    for (uint32_t v = 0; v < graph.len; v += 1) {
        AvenGraphAdjList v_adj = slice_get(graph, v);

        AvenGraphAugAdjListNode *v_aug_adj_nodes = aven_arena_create_array(
            AvenGraphAugAdjListNode,
            arena,
            v_adj.len
        );

        slice_get(aug_graph, v) = (AvenGraphAugAdjList){
            .len = v_adj.len,
            .ptr = v_aug_adj_nodes,
        };
    }

    AvenArena temp_arena = *arena;

    typedef List(AvenGraphAugAdjListNode) AvenGraphAugWorkList;

    Slice(AvenGraphAugWorkList) work_lists = { .len = graph.len };
    work_lists.ptr = aven_arena_create_array(
        AvenGraphAugWorkList,
        &temp_arena,
        work_lists.len
    );

    for (uint32_t v = 0; v < graph.len; v += 1) {
        AvenGraphAdjList v_adj = slice_get(graph, v);

        AvenGraphAugAdjListNode *v_work_nodes = aven_arena_create_array(
            AvenGraphAugAdjListNode,
            &temp_arena,
            v_adj.len
        );

        slice_get(work_lists, v) = (AvenGraphAugWorkList){
            .cap = v_adj.len,
            .ptr = v_work_nodes,
        };
    }

    for (uint32_t v = 0; v < graph.len; v += 1) {
        AvenGraphAdjList v_adj = slice_get(graph, v);
        for (uint32_t i = 0; i < v_adj.len; i += 1) {
            uint32_t u = slice_get(v_adj, i);

            list_push(slice_get(work_lists, u)) = (AvenGraphAugAdjListNode){
                .vertex = v,
                .back_index = i,
            };
        }
    }

    for (uint32_t k = (uint32_t)graph.len; k > 0; k -= 1) {
        uint32_t v = k - 1;

        AvenGraphAugWorkList *v_work_list = &slice_get(work_lists, v);
        for (uint32_t i = 0; i < v_work_list->len; i += 1) {
            AvenGraphAugAdjListNode v_work_node = slice_get(*v_work_list, i);
            uint32_t u = v_work_node.vertex;

            AvenGraphAugWorkList *u_work_list = &slice_get(work_lists, u);
            AvenGraphAugAdjListNode u_work_node = slice_get(
                *u_work_list,
                u_work_list->len - 1
            );

            slice_get(
                slice_get(aug_graph, v),
                u_work_node.back_index
            ).back_index = v_work_node.back_index;
            slice_get(
                slice_get(aug_graph, u),
                v_work_node.back_index
            ).back_index = u_work_node.back_index;

            u_work_list->len -= 1;
        }

        v_work_list->len = 0;
    }

    for (uint32_t v = 0; v < graph.len; v += 1) {
        assert(slice_get(work_lists, v).len == 0);
    }

    return aug_graph;
}

static inline uint32_t aven_graph_aug_neighbor_index(
    AvenGraphAug graph,
    uint32_t vertex,
    uint32_t neighbor
) {
    AvenGraphAugAdjList adj = slice_get(graph, vertex);
    for (uint32_t i = 0; i < adj.len; i += 1) {
        if (slice_get(adj, i).vertex == neighbor) {
            return i;
        }
    }
    assert(false);
}

#endif // AVEN_GRAPH_H

