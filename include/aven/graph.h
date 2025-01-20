#ifndef AVEN_GRAPH_H
#define AVEN_GRAPH_H

#include <aven.h>
#include <aven/arena.h>

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

static inline uint32_t aven_graph_adj_neighbor_index(
    AvenGraphAdjList adj,
    uint32_t neighbor
) {
    for (uint32_t i = 0; i < adj.len; i += 1) {
        if (get(adj, i) == neighbor) {
            return i;
        }
    }
    assert(false);
    return 0xffffffff;
}

static inline uint32_t aven_graph_adj_next_neighbor_index(
    AvenGraphAdjList adj,
    uint32_t neighbor
) {
    for (uint32_t i = 0; i + 1 < adj.len; i += 1) {
        if (get(adj, i) == neighbor) {
            return i + 1;
        }
    }
    assert(get(adj, adj.len - 1) == neighbor);

    return 0;
}

static inline uint32_t aven_graph_adj_next(
    AvenGraphAdjList adj,
    uint32_t i
) {
    if (i + 1 == adj.len) {
        return 0;
    }

    return i + 1;
}

static inline uint32_t aven_graph_adj_prev(
    AvenGraphAdjList adj,
    uint32_t i
) {
    if (i == 0) {
        return (uint32_t)(adj.len - 1);
    }

    return i - 1;
}

static inline uint32_t aven_graph_aug_adj_neighbor_index(
    AvenGraphAugAdjList adj,
    uint32_t neighbor
) {
    for (uint32_t i = 0; i < adj.len; i += 1) {
        if (get(adj, i).vertex == neighbor) {
            return i;
        }
    }
    assert(false);
    return 0xffffffff;
}

static inline uint32_t aven_graph_aug_adj_next(
    AvenGraphAugAdjList adj,
    uint32_t i
) {
    if (i + 1 == adj.len) {
        return 0;
    }

    return i + 1;
}

static inline uint32_t aven_graph_aug_adj_prev(
    AvenGraphAugAdjList adj,
    uint32_t i
) {
    if (i == 0) {
        return (uint32_t)(adj.len - 1);
    }

    return i - 1;
}

static inline AvenGraphAug aven_graph_aug(AvenGraph graph, AvenArena *arena) {
    AvenGraphAug aug_graph = { .len = graph.len };
    aug_graph.ptr = aven_arena_create_array(
        AvenGraphAugAdjList,
        arena,
        aug_graph.len
    );

    for (uint32_t v = 0; v < graph.len; v += 1) {
        AvenGraphAdjList v_adj = get(graph, v);

        AvenGraphAugAdjListNode *v_aug_adj_nodes = aven_arena_create_array(
            AvenGraphAugAdjListNode,
            arena,
            v_adj.len
        );

        AvenGraphAugAdjList *v_aug_adj = &get(aug_graph, v);
        *v_aug_adj = (AvenGraphAugAdjList){
            .len = v_adj.len,
            .ptr = v_aug_adj_nodes,
        };

        for (uint32_t i = 0; i < v_adj.len; i += 1) {
            get(*v_aug_adj, i).vertex = get(v_adj, i);
        }
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
        AvenGraphAdjList v_adj = get(graph, v);

        AvenGraphAugAdjListNode *v_work_nodes = aven_arena_create_array(
            AvenGraphAugAdjListNode,
            &temp_arena,
            v_adj.len
        );

        get(work_lists, v) = (AvenGraphAugWorkList){
            .cap = v_adj.len,
            .ptr = v_work_nodes,
        };
    }

    for (uint32_t v = 0; v < graph.len; v += 1) {
        AvenGraphAdjList v_adj = get(graph, v);
        for (uint32_t i = 0; i < v_adj.len; i += 1) {
            uint32_t u = get(v_adj, i);

            list_push(get(work_lists, u)) = (AvenGraphAugAdjListNode){
                .vertex = v,
                .back_index = i,
            };
        }
    }

    for (uint32_t k = (uint32_t)graph.len; k > 0; k -= 1) {
        uint32_t v = k - 1;

        AvenGraphAugWorkList *v_work_list = &get(work_lists, v);
        for (uint32_t i = 0; i < v_work_list->len; i += 1) {
            AvenGraphAugAdjListNode v_work_node = get(*v_work_list, i);
            uint32_t u = v_work_node.vertex;

            AvenGraphAugWorkList *u_work_list = &get(work_lists, u);
            AvenGraphAugAdjListNode u_work_node = get(
                *u_work_list,
                u_work_list->len - 1
            );

            get(
                get(aug_graph, v),
                u_work_node.back_index
            ).back_index = v_work_node.back_index;
            get(
                get(aug_graph, u),
                v_work_node.back_index
            ).back_index = u_work_node.back_index;

            u_work_list->len -= 1;
        }

        v_work_list->len = 0;
    }

    for (uint32_t v = 0; v < graph.len; v += 1) {
        assert(get(work_lists, v).len == 0);
    }

    return aug_graph;
}

#endif // AVEN_GRAPH_H

