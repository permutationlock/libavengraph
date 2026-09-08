#ifndef TEST_GRAPH_DYN_H
    #define TEST_GRAPH_DYN_H

    #include <aven.h>
    #include <aven/arena.h>
    #include <aven/fmt.h>
    #include <aven/test.h>

    #include <graph.h>
    #include <graph/bfs.h>
    #include <graph/gen.h>

    #include <stdio.h>

    typedef struct {
        uint32_t u;
        uint32_t v;
    } TestGraphDynEdge;
    typedef Slice(TestGraphDynEdge) TestGraphDynEdgeSlice;

    typedef enum {
        TEST_GRAPH_DYN_OP_TYPE_INS_VERT,
        TEST_GRAPH_DYN_OP_TYPE_DEL_VERT,
        TEST_GRAPH_DYN_OP_TYPE_INS_EDGE,
        TEST_GRAPH_DYN_OP_TYPE_DEL_EDGE,
    } TestGraphDynOpType;

    typedef struct {
        TestGraphDynOpType type;
        uint32_t idx;
    } TestGraphDynOp;
    typedef Slice(TestGraphDynOp) TestGraphDynOpSlice;

    typedef struct {
        uint32_t size;
        TestGraphDynEdgeSlice edges;
        TestGraphDynOpSlice ops;
    } TestGraphDynArgs;

    static AvenTestResult test_graph_dyn_ops(
        AvenArena *emsg_arena,
        AvenArena arena,
        void *opaque_args
    ) {
        TestGraphDynArgs *args = opaque_args;

        GraphDynSubset vert_labels = aven_arena_create_slice(
            Idx,
            &arena,
            args->size
        );
        for (size_t i = 0; i < vert_labels.len; i += 1) {
            get(vert_labels, i) = (Idx){ 0 };
        }

        Slice(GraphDynEdge) edge_nbs = aven_arena_create_slice(
            GraphDynEdge,
            &arena,
            args->edges.len
        );
        for (size_t i = 0; i < edge_nbs.len; i += 1) {
            get(edge_nbs, i) = (GraphDynEdge){ 0 };
        }

        GraphDyn dgraph = graph_dyn_init(
            args->size,
            (uint32_t)(args->edges.len * 2),
            &arena
        );

        for (size_t i = 0; i < args->ops.len; i += 1) {
            TestGraphDynOp op = get(args->ops, i);
            switch (op.type) {
                case TEST_GRAPH_DYN_OP_TYPE_INS_VERT: {
                    get(vert_labels, op.idx) = graph_dyn_insert_vertex(&dgraph);
                    break;
                }
                case TEST_GRAPH_DYN_OP_TYPE_DEL_VERT: {
                    graph_dyn_delete_vertex(&dgraph, get(vert_labels, op.idx));
                    get(vert_labels, op.idx) = (Idx){ 0 };
                    break;
                }
                case TEST_GRAPH_DYN_OP_TYPE_INS_EDGE: {
                    TestGraphDynEdge edge = get(args->edges, op.idx);
                    Idx u_label = get(vert_labels, edge.u);
                    Idx v_label = get(vert_labels, edge.v);
                    get(edge_nbs, op.idx) = graph_dyn_insert_edge(
                        &dgraph,
                        u_label,
                        graph_dyn_nb_last(dgraph, u_label),
                        v_label,
                        graph_dyn_nb_last(dgraph, v_label)
                    );
                    break;
                }
                case TEST_GRAPH_DYN_OP_TYPE_DEL_EDGE: {
                    GraphDynEdge dedge = get(edge_nbs, op.idx);
                    graph_dyn_delete_edge(&dgraph, dedge.nb1);
                    break;
                }
            }
        }

        Graph graph = graph_from_dyn_init(
            dgraph.adj.used,
            dgraph.nb.used / 2,
            &arena
        );
        graph_from_dyn(graph, dgraph, arena);
        GraphDynSubset labels = graph_from_dyn_labels(dgraph, &arena);

        if (labels.len != dgraph.adj.used) {
            return (AvenTestResult){
                .error = 1,
                .message = aven_fmt(
                    emsg_arena,
                    "expected {} labels, found {}",
                    aven_fmt_uint(dgraph.adj.used),
                    aven_fmt_uint(labels.len)
                ),
            };
        }

        for (uint32_t v = 0; v < labels.len; v += 1) {
            uint32_t v_deg = (uint32_t)get(graph.adj, v).len;
            Idx dv = get(labels, v);
            uint32_t dv_deg = graph_dyn_deg(dgraph, dv);

            if (v_deg != dv_deg) {
                return (AvenTestResult){
                    .error = 1,
                    .message = aven_fmt(
                        emsg_arena,
                        "expected vertex {} to have deg {}, found {}",
                        aven_fmt_uint(idx_unwrap(dv)),
                        aven_fmt_uint(dv_deg),
                        aven_fmt_uint(v_deg)
                    ),
                };
            }
        }

        for (uint32_t v = 0; v < labels.len; v += 1) {
            GraphAdj v_adj = get(graph.adj, v);

            Idx dv = get(labels, v);
            Idx nb = graph_dyn_nb(dgraph, dv);

            for (uint32_t i = 0; i < v_adj.len; i += 1) {
                uint32_t vn = graph_nb(graph.nb, v_adj, i);
                Idx vn_label = get(labels, vn);
                Idx dv_n = graph_dyn_nb_vertex(dgraph, nb);

                if (!idx_valid(vn_label)) {
                    return (AvenTestResult){
                        .error = 1,
                        .message = aven_fmt(
                            emsg_arena,
                            "expected vertex {} neighbor {} label invalid",
                            aven_fmt_uint(idx_unwrap(dv)),
                            aven_fmt_uint(i)
                        ),
                    };
                }

                if (!idx_valid(dv_n)) {
                    return (AvenTestResult){
                        .error = 1,
                        .message = aven_fmt(
                            emsg_arena,
                            "expected vertex {} neighbor {} invalid in dgraph",
                            aven_fmt_uint(idx_unwrap(dv)),
                            aven_fmt_uint(i)
                        ),
                    };
                }

                if (idx_unwrap(vn_label) != idx_unwrap(dv_n)) {
                    return (AvenTestResult){
                        .error = 1,
                        .message = aven_fmt(
                            emsg_arena,
                            "expected vertex {} neighbor {} to be {}, found {}",
                            aven_fmt_uint(idx_unwrap(dv)),
                            aven_fmt_uint(i),
                            aven_fmt_uint(idx_unwrap(dv_n)),
                            aven_fmt_uint(idx_unwrap(vn_label))
                        ),
                    };
                }

                nb = graph_dyn_nb_next(dgraph, nb);
            }
        }

        return (AvenTestResult){ 0 };
    }

    static void test_graph_dyn(AvenArena arena) {
        AvenTestCase tcase_data[] = {
            {
                .desc = aven_str("K_1"),
                .args = &(TestGraphDynArgs){
                    .size = 1,
                    .ops = slice_array(
                        (TestGraphDynOp[]){
                            {
                                .type = TEST_GRAPH_DYN_OP_TYPE_INS_VERT,
                                .idx = 0,
                            },
                        }
                    ),
                },
                .fn = test_graph_dyn_ops,
            },
            {
                .desc = aven_str("K_2"),
                .args = &(TestGraphDynArgs){
                    .size = 2,
                    .edges = slice_array((TestGraphDynEdge[]){ { 0, 1 } }),
                    .ops = slice_array(
                        (TestGraphDynOp[]){
                            {
                                .type = TEST_GRAPH_DYN_OP_TYPE_INS_VERT,
                                .idx = 0,
                            },
                            {
                                .type = TEST_GRAPH_DYN_OP_TYPE_INS_VERT,
                                .idx = 1,
                            },
                            {
                                .type = TEST_GRAPH_DYN_OP_TYPE_INS_EDGE,
                                .idx = 0,
                            },
                        }
                    ),
                },
                .fn = test_graph_dyn_ops,
            },
            {
                .desc = aven_str("K_3"),
                .args = &(TestGraphDynArgs){
                    .size = 3,
                    .edges = slice_array(
                        (TestGraphDynEdge[]){ { 0, 1 }, { 0, 2 }, { 1, 2 } }
                    ),
                    .ops = slice_array(
                        (TestGraphDynOp[]){
                            {
                                .type = TEST_GRAPH_DYN_OP_TYPE_INS_VERT,
                                .idx = 0,
                            },
                            {
                                .type = TEST_GRAPH_DYN_OP_TYPE_INS_VERT,
                                .idx = 1,
                            },
                            {
                                .type = TEST_GRAPH_DYN_OP_TYPE_INS_EDGE,
                                .idx = 0,
                            },
                            {
                                .type = TEST_GRAPH_DYN_OP_TYPE_INS_VERT,
                                .idx = 2,
                            },
                            {
                                .type = TEST_GRAPH_DYN_OP_TYPE_INS_EDGE,
                                .idx = 1,
                            },
                            {
                                .type = TEST_GRAPH_DYN_OP_TYPE_INS_EDGE,
                                .idx = 2,
                            },
                        }
                    ),
                },
                .fn = test_graph_dyn_ops,
            },
            {
                .desc = aven_str("K_2 to K_3 to K_2"),
                .args = &(TestGraphDynArgs){
                    .size = 3,
                    .edges = slice_array(
                        (TestGraphDynEdge[]){ { 0, 1 }, { 0, 2 }, { 1, 2 } }
                    ),
                    .ops = slice_array(
                        (TestGraphDynOp[]){
                            {
                                .type = TEST_GRAPH_DYN_OP_TYPE_INS_VERT,
                                .idx = 0,
                            },
                            {
                                .type = TEST_GRAPH_DYN_OP_TYPE_INS_VERT,
                                .idx = 1,
                            },
                            {
                                .type = TEST_GRAPH_DYN_OP_TYPE_INS_EDGE,
                                .idx = 0,
                            },
                            {
                                .type = TEST_GRAPH_DYN_OP_TYPE_INS_VERT,
                                .idx = 2,
                            },
                            {
                                .type = TEST_GRAPH_DYN_OP_TYPE_INS_EDGE,
                                .idx = 1,
                            },
                            {
                                .type = TEST_GRAPH_DYN_OP_TYPE_INS_EDGE,
                                .idx = 2,
                            },
                            {
                                .type = TEST_GRAPH_DYN_OP_TYPE_DEL_VERT,
                                .idx = 0,
                            },
                        }
                    ),
                },
                .fn = test_graph_dyn_ops,
            },
        };
        AvenTestCaseSlice tcases = slice_array(tcase_data);

        aven_test(tcases, arena);
    }

#endif // TEST_GRAPH_DYN_H
