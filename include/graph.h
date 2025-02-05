#ifndef GRAPH_H
#define GRAPH_H

#include <aven.h>
#include <aven/arena.h>

typedef Slice(uint32_t) GraphAdjList;
typedef Slice(GraphAdjList) Graph;

typedef Slice(uint32_t) GraphSubset;

typedef Slice(uint64_t) GraphPropUint64;
typedef Slice(uint32_t) GraphPropUint32;
typedef Slice(uint16_t) GraphPropUint16;
typedef Slice(uint8_t) GraphPropUint8;

typedef struct{
    uint32_t vertex;
    uint32_t back_index;
} GraphAugAdjListNode;

typedef Slice(GraphAugAdjListNode) GraphAugAdjList;
typedef Slice(GraphAugAdjList) GraphAug;

static inline uint32_t graph_adj_neighbor_index(
    GraphAdjList adj,
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

static inline uint32_t graph_adj_next_neighbor_index(
    GraphAdjList adj,
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

static inline uint32_t graph_adj_next(
    GraphAdjList adj,
    uint32_t i
) {
    if (i + 1 == adj.len) {
        return 0;
    }

    return i + 1;
}

static inline uint32_t graph_adj_prev(
    GraphAdjList adj,
    uint32_t i
) {
    if (i == 0) {
        return (uint32_t)(adj.len - 1);
    }

    return i - 1;
}

static inline uint32_t graph_aug_adj_neighbor_index(
    GraphAugAdjList adj,
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

static inline uint32_t graph_aug_adj_next(
    GraphAugAdjList adj,
    uint32_t i
) {
    if (i + 1 == adj.len) {
        return 0;
    }

    return i + 1;
}

static inline uint32_t graph_aug_adj_prev(
    GraphAugAdjList adj,
    uint32_t i
) {
    if (i == 0) {
        return (uint32_t)(adj.len - 1);
    }

    return i - 1;
}

static inline GraphAug graph_aug(Graph graph, AvenArena *arena) {
    GraphAug aug_graph = { .len = graph.len };
    aug_graph.ptr = aven_arena_create_array(
        GraphAugAdjList,
        arena,
        aug_graph.len
    );

    for (uint32_t v = 0; v < graph.len; v += 1) {
        GraphAdjList v_adj = get(graph, v);

        GraphAugAdjListNode *v_aug_adj_nodes = aven_arena_create_array(
            GraphAugAdjListNode,
            arena,
            v_adj.len
        );

        GraphAugAdjList *v_aug_adj = &get(aug_graph, v);
        *v_aug_adj = (GraphAugAdjList){
            .len = v_adj.len,
            .ptr = v_aug_adj_nodes,
        };

        for (uint32_t i = 0; i < v_adj.len; i += 1) {
            get(*v_aug_adj, i).vertex = get(v_adj, i);
        }
    }

    AvenArena temp_arena = *arena;

    typedef List(GraphAugAdjListNode) GraphAugWorkList;

    Slice(GraphAugWorkList) work_lists = { .len = graph.len };
    work_lists.ptr = aven_arena_create_array(
        GraphAugWorkList,
        &temp_arena,
        work_lists.len
    );

    for (uint32_t v = 0; v < graph.len; v += 1) {
        GraphAdjList v_adj = get(graph, v);

        GraphAugAdjListNode *v_work_nodes = aven_arena_create_array(
            GraphAugAdjListNode,
            &temp_arena,
            v_adj.len
        );

        get(work_lists, v) = (GraphAugWorkList){
            .cap = v_adj.len,
            .ptr = v_work_nodes,
        };
    }

    for (uint32_t v = 0; v < graph.len; v += 1) {
        GraphAdjList v_adj = get(graph, v);
        for (uint32_t i = 0; i < v_adj.len; i += 1) {
            uint32_t u = get(v_adj, i);

            list_push(get(work_lists, u)) = (GraphAugAdjListNode){
                .vertex = v,
                .back_index = i,
            };
        }
    }

    for (uint32_t k = (uint32_t)graph.len; k > 0; k -= 1) {
        uint32_t v = k - 1;

        GraphAugWorkList *v_work_list = &get(work_lists, v);
        for (uint32_t i = 0; i < v_work_list->len; i += 1) {
            GraphAugAdjListNode v_work_node = get(*v_work_list, i);
            uint32_t u = v_work_node.vertex;

            GraphAugWorkList *u_work_list = &get(work_lists, u);
            GraphAugAdjListNode u_work_node = get(
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

    return aug_graph;
}

#endif // GRAPH_H

