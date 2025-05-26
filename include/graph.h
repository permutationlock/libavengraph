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

    static inline uint32_t graph_nb(GraphNbSlice nb, GraphAdj v_adj, uint32_t i) {
        assert(i < v_adj.len);
        return get(nb, v_adj.index + i);
    }

    static inline uint32_t graph_adj_next(GraphAdj v_adj, uint32_t i) {
        assert(i < v_adj.len);
        if (i == v_adj.len - 1) {
            return 0;
        }
        return i + 1;
    }

    static inline uint32_t graph_adj_prev(GraphAdj v_adj, uint32_t i) {
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

    typedef struct {
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
            get(aug_graph.nb, i) = (GraphAugNb){ .vertex = get(graph.nb, i) };
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
                GraphAugNb u_work_node = get(*u_work_list, u_work_list->len - 1);

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

    typedef struct {
        uint32_t vertex;
        uint32_t back_nb;
        uint32_t next;
        uint32_t prev;
    } GraphDynNb;

    typedef PoolEntry(GraphDynNb) GraphDynNbEntry;
    typedef PoolExplicit(GraphDynNbEntry) GraphDynNbPool;

    typedef struct {
        uint32_t nb;
        uint32_t deg;
    } GraphDynAdj;

    typedef PoolEntry(GraphDynAdj) GraphDynAdjEntry;
    typedef PoolExplicit(GraphDynAdjEntry) GraphDynAdjPool;

    typedef struct {
        GraphDynNbPool nb;
        GraphDynAdjPool adj;
    } GraphDyn;

    static inline GraphDyn graph_dyn_init(
        uint32_t max_vertices,
        uint32_t max_edges,
        AvenArena *arena
    ) {
        GraphDyn graph = {
            .nb = { .cap = 2 * max_edges },
            .adj = { .cap = max_vertices },
        };
        graph.adj.ptr = aven_arena_create_array(
            GraphDynAdjEntry,
            arena,
            graph.adj.cap
        );
        graph.nb.ptr = aven_arena_create_array(
            GraphDynNbEntry,
            arena,
            graph.nb.cap
        );
        return graph;
    }

    static inline uint32_t graph_dyn_nb(GraphDyn graph, uint32_t v) {
        return pool_get(graph.adj, v).nb;
    }

    static inline uint32_t graph_dyn_nb_next(GraphDyn graph, uint32_t nb) {
        assert(nb != 0);
        return pool_get(graph.nb, nb).next;
    }

    static inline uint32_t graph_dyn_nb_prev(GraphDyn graph, uint32_t nb) {
        assert(nb != 0);
        return pool_get(graph.nb, nb - 1).prev;
    }

    static inline uint32_t graph_dyn_nb_vertex(GraphDyn graph, uint32_t nb) {
        assert(nb != 0);
        return pool_get(graph.nb, nb - 1).vertex;
    }

    static inline uint32_t graph_dyn_nb_back_nb(GraphDyn graph, uint32_t nb) {
        assert(nb != 0);
        return pool_get(graph.nb, nb - 1).back_nb;
    }

    static inline void graph_dyn_insert_edge(
        GraphDyn *graph,
        uint32_t v1,
        uint32_t nb1,
        uint32_t v2,
        uint32_t nb2
    ) {
        uint32_t next_nb1 = (uint32_t)pool_create(graph->nb);
        uint32_t next_nb2 = (uint32_t)pool_create(graph->nb);
        if (pool_get(graph->adj, v1).deg == 0) {
            assert(nb1 == 0);
            pool_get(graph->nb, next_nb1) = (GraphDynNb){
                .vertex = v2,
                .next = next_nb1,
                .prev = next_nb1,
                .back_nb = next_nb2,
            };
            pool_get(graph->adj, v1).nb = next_nb1 + 1;
        } else {
            assert(nb1 != 0);
            nb1 -= 1;
            pool_get(graph->nb, next_nb1) = (GraphDynNb){
                .vertex = v2,
                .next = pool_get(graph->nb, nb1).next,
                .prev = nb1 + 1,
                .back_nb = next_nb2 + 1,
            };
            pool_get(graph->nb, pool_get(graph->nb, nb1).next).prev = next_nb1 +
                1;
            pool_get(graph->nb, nb1).next = next_nb1 + 1;
        }
        if (pool_get(graph->adj, v2).deg == 0) {
            assert(nb2 == 0);
            pool_get(graph->nb, next_nb2) = (GraphDynNb){
                .vertex = v1,
                .next = next_nb2 + 1,
                .prev = next_nb2 + 1,
                .back_nb = next_nb1 + 1,
            };
            pool_get(graph->adj, v2).nb = next_nb2 + 1;
        } else {
            assert(nb2 != 0);
            nb2 -= 1;
            pool_get(graph->nb, next_nb2) = (GraphDynNb){
                .vertex = v1,
                .next = pool_get(graph->nb, nb2).next,
                .prev = nb2 + 1,
                .back_nb = next_nb1 + 1,
            };
            pool_get(graph->nb, pool_get(graph->nb, nb2).next).prev = next_nb2 +
                1;
            pool_get(graph->nb, nb2).next = next_nb2 + 1;
        }
        pool_get(graph->adj, v1).deg += 1;
        pool_get(graph->adj, v2).deg += 1;
    }

    static inline uint32_t graph_dyn_delete_edge(GraphDyn *graph, uint32_t nb1) {
        assert(nb1 != 0);
        GraphDynNb nb1_entry = pool_get(graph->nb, nb1 - 1);
        uint32_t v2 = nb1_entry.vertex;
        uint32_t nb2 = nb1_entry.back_nb;
        GraphDynNb nb2_entry = pool_get(graph->nb, nb2 - 1);
        uint32_t v1 = nb2_entry.vertex;
        if (pool_get(graph->adj, v1).deg == 1) {
            pool_delete(graph->nb, nb1 - 1);
            pool_get(graph->adj, v1).nb = 0;
        } else {
            pool_get(graph->nb, nb1_entry.next - 1).prev = nb1_entry.prev;
            pool_get(graph->nb, nb1_entry.prev - 1).next = nb1_entry.next;
            pool_get(graph->nb, nb2_entry.next - 1).prev = nb2_entry.prev;
            pool_get(graph->nb, nb2_entry.prev - 1).next = nb2_entry.next;
            pool_delete(graph->nb, nb1 - 1);
            pool_delete(graph->nb, nb2 - 1);
        }
        pool_get(graph->adj, v1).deg -= 1;
        pool_get(graph->adj, v2).deg -= 1;
        if (pool_get(graph->adj, v1).deg > 0) {
            return nb1_entry.next;
        }
        return 0;
    }

    static inline void graph_dyn_delete_vertex(GraphDyn *graph, uint32_t v) {
        uint32_t nb = graph_dyn_nb(*graph, v);
        while (nb != 0) {
            nb = graph_dyn_delete_edge(graph, nb);
        }
        pool_delete(graph->adj, v);
    }
    // static inline Graph graph_dyn_as_graph(GraphDyn graph, AvenArena *arena) {
    //     Graph fixed_graph = {
    //         .adj = aven_arena_create_slice(
    //             GraphAdj,
    //             arena,
    //             graph.adj.used
    //         ),
    //         .nb = aven_arena_create_slice(
    //             uint32_t,
    //             arena,
    //             graph.nb.used
    //         ),
    //     };

    //     AvenArena temp_arena = *arena;
    //     Slice(uint32_t) labels = aven_arena_create_slice(
    //         uint32_t,
    //         &temp_arena,
    //         graph.adj.len
    //     );
    //     Slice(uint32_t) vertices = aven_arena_create_slice(
    //         uint32_t,
    //         &temp_arena,
    //         graph.adj.used
    //     );
    //     for (uint32_t v = 0; v < labels.len; v += 1) {
    //         get(labels, v) = 0;
    //     }
    //     size_t free = graph.adj.free;
    //     while (free != 0) {
    //         get(labels, free - 1) = 0xffffffff;
    //         free = get(graph.adj, free).parent;
    //     }

    //     uint32_t count = 0;
    //     for (uint32_t v = 0; v < labels.len; v += 1) {
    //         if (get(labels, v) == 0) {
    //             get(vertices, count) = v;
    //             get(labels, v) = count++;
    //         }
    //     }
    //     assert(count == vertices.len);

    //     for (uint32_t i = 0; i < vertices.len; i += 1) {
    //         uint32_t v = get(vertices, i);

    //     }

    //     return fixed_graph;
    // }

#endif // GRAPH_H

