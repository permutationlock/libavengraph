#ifndef GRAPH_H
#define GRAPH_H

#include <aven.h>
#include <aven/arena.h>

typedef struct {
    uint32_t index;
    uint32_t len;
} GraphAdj;
typedef Slice(GraphAdj) GraphAdjSlice;

typedef Slice(uint32_t) GraphNbSlice;

typedef struct {
    GraphNbSlice nb;
    GraphAdjSlice adj;
} Graph;

typedef Slice(uint32_t) GraphSubset;

static inline uint32_t graph_nb(
    GraphNbSlice nb,
    GraphAdj v_adj,
    uint32_t i
) {
    assert(i < v_adj.len);
    return get(nb, v_adj.index + i);
}

static inline uint32_t graph_adj_next(
    GraphAdj v_adj,
    uint32_t i
) {
    assert(i < v_adj.len);
    if (i == v_adj.len - 1) {
        return 0;
    }
    return i + 1;
}

static inline uint32_t graph_adj_prev(
    GraphAdj v_adj,
    uint32_t i
) {
    assert(i < v_adj.len);
    if (i == 0) {
        return v_adj.len - 1;
    }
    return i - 1;
}

static inline uint32_t graph_nb_index(
    GraphNbSlice nb,
    GraphAdj v_adj,
    uint32_t u
) {
    for (uint32_t i = 0; i < v_adj.len; i += 1) {
        if (graph_nb(nb, v_adj, i) == u) {
            return i;
        }
    }

    assert(false);
    return 0xffffffff;
}

typedef Slice(uint64_t) GraphPropUint64;
typedef Slice(uint32_t) GraphPropUint32;
typedef Slice(uint16_t) GraphPropUint16;
typedef Slice(uint8_t) GraphPropUint8;

typedef struct{
    uint32_t vertex;
    uint32_t back_index;
} GraphAugNb;
typedef Slice(GraphAugNb) GraphAugNbSlice;

typedef struct {
    GraphAugNbSlice nb;
    GraphAdjSlice adj;
} GraphAug;

static inline GraphAugNb graph_aug_nb(
    GraphAugNbSlice nb,
    GraphAdj v_adj,
    uint32_t i
) {
    assert(i < v_adj.len);
    return get(nb, v_adj.index + i);
}

static inline uint32_t graph_aug_nb_index(
    GraphAugNbSlice nb,
    GraphAdj v_adj,
    uint32_t u
) {
    for (uint32_t i = 0; i < v_adj.len; i += 1) {
        if (graph_aug_nb(nb, v_adj, i).vertex == u) {
            return i;
        }
    }

    assert(false);
    return 0xffffffff;
}

static inline uint32_t graph_aug_deg(GraphAug graph, uint32_t v) {
    return get(graph.adj, v).len;
}

static inline GraphAug graph_aug(Graph graph, AvenArena *arena) {
    GraphAug aug_graph = {
        .nb = { .len = graph.nb.len },
        .adj = { .len = graph.adj.len },
    };
    aug_graph.nb.ptr = aven_arena_create_array(
        GraphAugNb,
        arena,
        aug_graph.nb.len
    );
    aug_graph.adj.ptr = aven_arena_create_array(
        GraphAdj,
        arena,
        aug_graph.adj.len
    );

    for (uint32_t v = 0; v < graph.adj.len; v += 1) {
        get(aug_graph.adj, v) = get(graph.adj, v);
    }

    for (uint32_t i = 0; i < graph.nb.len; i += 1) {
        get(aug_graph.nb, i) = (GraphAugNb){
            .vertex = get(graph.nb, i),
        };
    }

    AvenArena temp_arena = *arena;

    typedef List(GraphAugNb) GraphAugWorkList;

    Slice(GraphAugWorkList) work_lists = { .len = graph.adj.len };
    work_lists.ptr = aven_arena_create_array(
        GraphAugWorkList,
        &temp_arena,
        work_lists.len
    );

    for (uint32_t v = 0; v < graph.adj.len; v += 1) {
        uint32_t v_deg = get(graph.adj, v).len;

        GraphAugNb *v_work_nodes = aven_arena_create_array(
            GraphAugNb,
            &temp_arena,
            v_deg
        );

        get(work_lists, v) = (GraphAugWorkList){
            .cap = v_deg,
            .ptr = v_work_nodes,
        };
    }

    for (uint32_t v = 0; v < graph.adj.len; v += 1) {
        GraphAdj v_adj = get(graph.adj, v);
        for (uint32_t i = 0; i < v_adj.len; i += 1) {
            uint32_t u = graph_nb(graph.nb, v_adj, i);

            list_push(get(work_lists, u)) = (GraphAugNb){
                .vertex = v,
                .back_index = i,
            };
        }
    }

    for (uint32_t k = (uint32_t)graph.adj.len; k > 0; k -= 1) {
        uint32_t v = k - 1;

        GraphAugWorkList *v_work_list = &get(work_lists, v);
        for (uint32_t i = 0; i < v_work_list->len; i += 1) {
            GraphAugNb v_work_node = get(*v_work_list, i);
            uint32_t u = v_work_node.vertex;

            GraphAugWorkList *u_work_list = &get(work_lists, u);
            GraphAugNb u_work_node = get(
                *u_work_list,
                u_work_list->len - 1
            );

            get(
                aug_graph.nb,
                get(aug_graph.adj, v).index + u_work_node.back_index
            ).back_index = v_work_node.back_index;
            get(
                aug_graph.nb,
                get(aug_graph.adj, u).index + v_work_node.back_index
            ).back_index = u_work_node.back_index;

            u_work_list->len -= 1;
        }

        v_work_list->len = 0;
    }

    return aug_graph;
}

#endif // GRAPH_H

