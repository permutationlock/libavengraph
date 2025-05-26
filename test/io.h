#ifndef TEST_IO_H
    #define TEST_IO_H

    #include <aven.h>
    #include <aven/arena.h>
    #include <aven/fs.h>
    #include <aven/io.h>
    #include <aven/path.h>
    #include <aven/str.h>
    #include <aven/test.h>

    #include <graph.h>
    #include <graph/io.h>

    #include <stdio.h>

    #include "gen.h"

    typedef struct {
        uint32_t size;
        TestGenGraphType type;
    } TestIoGraphArgs;

    static AvenTestResult test_io_graph(
        AvenArena *emsg_arena,
        AvenArena arena,
        void *opaque_args
    ) {
        TestIoGraphArgs *args = opaque_args;
        Graph graph = test_gen_graph(args->size, args->type, &arena);

        ByteSlice space = aven_arena_create_slice(
            unsigned char,
            &arena,
            graph_io_size(graph)
        );
        AvenIoWriter writer = aven_io_writer_init_bytes(space);
        int wr_error = graph_io_push(&writer, graph);
        if (wr_error != 0) {
            return (AvenTestResult){
                .message = aven_str("failed to push graph to writer"),
                .error = wr_error,
            };
        }

        AvenIoReader reader = aven_io_reader_init_bytes(space);
        GraphIoResult rd_res = graph_io_pop(&reader, &arena);
        if (rd_res.error != 0) {
            return (AvenTestResult){
                .message = aven_str("failed to pop graph from reader"),
                .error = rd_res.error,
            };
        }

        Graph read_graph = rd_res.payload;

        if (!graph_io_validate(read_graph)) {
            return (AvenTestResult){
                .message = aven_str("poped graph invalid"),
                .error = 1,
            };
        }

        if (read_graph.adj.len != graph.adj.len) {
            return (AvenTestResult){
                .error = 1,
                .message = aven_fmt(
                    emsg_arena,
                    "expected {} vertices, found {}",
                    aven_fmt_uint(graph.adj.len),
                    aven_fmt_uint(read_graph.adj.len)
                ),
            };
        }

        if (read_graph.nb.len != graph.nb.len) {
            return (AvenTestResult){
                .error = 1,
                .message = aven_fmt(
                    emsg_arena,
                    "expected {} half-edges, found {}",
                    aven_fmt_uint(graph.nb.len),
                    aven_fmt_uint(read_graph.nb.len)
                ),
            };
        }

        size_t inv_adj = 0;
        for (uint32_t v = 0; v < graph.adj.len; v += 1) {
            GraphAdj v_adj = get(graph.adj, v);
            GraphAdj rv_adj = get(read_graph.adj, v);
            if (v_adj.len != rv_adj.len) {
                inv_adj += 1;
                continue;
            }
            for (uint32_t i = 0; i < v_adj.len; i += 1) {
                uint32_t u = graph_nb(graph.nb, v_adj, i);
                uint32_t ru = graph_nb(read_graph.nb, rv_adj, i);
                if (u != ru) {
                    inv_adj += 1;
                    i = v_adj.len;
                    break;
                }
            }
        }

        if (inv_adj != 0) {
            return (AvenTestResult){
                .error = 1,
                .message = aven_fmt(
                    emsg_arena,
                    "read graph adj lists differed from original in {} places",
                    aven_fmt_uint(inv_adj)
                ),
            };
        }

        return (AvenTestResult){ 0 };
    }

    static AvenTestResult test_io_graph_aug(
        AvenArena *emsg_arena,
        AvenArena arena,
        void *opaque_args
    ) {
        TestIoGraphArgs *args = opaque_args;
        Graph graph = test_gen_graph(args->size, args->type, &arena);
        GraphAug aug_graph = graph_aug(graph, &arena);

        ByteSlice space = aven_arena_create_slice(
            unsigned char,
            &arena,
            graph_io_size(aug_graph)
        );
        AvenIoWriter writer = aven_io_writer_init_bytes(space);
        int wr_error = graph_io_aug_push(&writer, aug_graph);
        if (wr_error != 0) {
            return (AvenTestResult){
                .message = aven_str("failed to push graph to writer"),
                .error = wr_error,
            };
        }

        AvenIoReader reader = aven_io_reader_init_bytes(space);
        GraphIoAugResult rd_res = graph_io_aug_pop(&reader, &arena);
        if (rd_res.error != 0) {
            return (AvenTestResult){
                .message = aven_str("failed to pop graph from reader"),
                .error = rd_res.error,
            };
        }

        GraphAug read_graph = rd_res.payload;

        if (!graph_io_aug_validate(read_graph)) {
            return (AvenTestResult){
                .message = aven_str("poped graph invalid"),
                .error = 1,
            };
        }

        if (read_graph.adj.len != aug_graph.adj.len) {
            return (AvenTestResult){
                .error = 1,
                .message = aven_fmt(
                    emsg_arena,
                    "expected {} vertices, found {}",
                    aven_fmt_uint(aug_graph.adj.len),
                    aven_fmt_uint(read_graph.adj.len)
                ),
            };
        }

        if (read_graph.nb.len != aug_graph.nb.len) {
            return (AvenTestResult){
                .error = 1,
                .message = aven_fmt(
                    emsg_arena,
                    "expected {} half-edges, found {}",
                    aven_fmt_uint(aug_graph.nb.len),
                    aven_fmt_uint(read_graph.nb.len)
                ),
            };
        }

        size_t inv_adj = 0;
        for (uint32_t v = 0; v < aug_graph.adj.len; v += 1) {
            GraphAdj v_adj = get(aug_graph.adj, v);
            GraphAdj rv_adj = get(read_graph.adj, v);
            if (v_adj.len != rv_adj.len) {
                inv_adj += 1;
                continue;
            }
            for (uint32_t i = 0; i < v_adj.len; i += 1) {
                GraphAugNb vu = graph_aug_nb(aug_graph.nb, v_adj, i);
                GraphAugNb rvu = graph_aug_nb(read_graph.nb, rv_adj, i);
                if (vu.vertex != rvu.vertex) {
                    inv_adj += 1;
                    i = v_adj.len;
                    break;
                }
            }
        }

        if (inv_adj != 0) {
            return (AvenTestResult){
                .error = 1,
                .message = aven_fmt(
                    emsg_arena,
                    "read graph adj lists differed from original in {} places",
                    aven_fmt_uint(inv_adj)
                ),
            };
        }

        return (AvenTestResult){ 0 };
    }

    static void test_io(AvenArena arena) {
        AvenTestCase tcase_data[] = {
            {
                .desc = aven_str("push pop K_1"),
                .args = &(TestIoGraphArgs){
                    .size = 1,
                    .type = TEST_GEN_GRAPH_TYPE_COMPLETE,
                },
                .fn = test_io_graph,
            },
            {
                .desc = aven_str("push pop K_5"),
                .args = &(TestIoGraphArgs){
                    .size = 5,
                    .type = TEST_GEN_GRAPH_TYPE_COMPLETE,
                },
                .fn = test_io_graph,
            },
            {
                .desc = aven_str("push pop K_19"),
                .args = &(TestIoGraphArgs){
                    .size = 19,
                    .type = TEST_GEN_GRAPH_TYPE_COMPLETE,
                },
                .fn = test_io_graph,
            },
            {
                .desc = aven_str("push pop 2x2 grid"),
                .args = &(TestIoGraphArgs){
                    .size = 2,
                    .type = TEST_GEN_GRAPH_TYPE_GRID,
                },
                .fn = test_io_graph,
            },
            {
                .desc = aven_str("push pop 9x9 grid"),
                .args = &(TestIoGraphArgs){
                    .size = 9,
                    .type = TEST_GEN_GRAPH_TYPE_GRID,
                },
                .fn = test_io_graph,
            },
            {
                .desc = aven_str("push pop pyramid A_3"),
                .args = &(TestIoGraphArgs){
                    .size = 3,
                    .type = TEST_GEN_GRAPH_TYPE_PYRAMID,
                },
                .fn = test_io_graph,
            },
            {
                .desc = aven_str("push pop pyramid A_9"),
                .args = &(TestIoGraphArgs){
                    .size = 9,
                    .type = TEST_GEN_GRAPH_TYPE_PYRAMID,
                },
                .fn = test_io_graph,
            },
            {
                .desc = aven_str("push pop augmented K_1"),
                .args = &(TestIoGraphArgs){
                    .size = 1,
                    .type = TEST_GEN_GRAPH_TYPE_COMPLETE,
                },
                .fn = test_io_graph_aug,
            },
            {
                .desc = aven_str("push pop augmented K_5"),
                .args = &(TestIoGraphArgs){
                    .size = 5,
                    .type = TEST_GEN_GRAPH_TYPE_COMPLETE,
                },
                .fn = test_io_graph_aug,
            },
            {
                .desc = aven_str("push pop augmented K_19"),
                .args = &(TestIoGraphArgs){
                    .size = 19,
                    .type = TEST_GEN_GRAPH_TYPE_COMPLETE,
                },
                .fn = test_io_graph_aug,
            },
            {
                .desc = aven_str("push pop augmented 2x2 grid"),
                .args = &(TestIoGraphArgs){
                    .size = 2,
                    .type = TEST_GEN_GRAPH_TYPE_GRID,
                },
                .fn = test_io_graph_aug,
            },
            {
                .desc = aven_str("push pop augmented 9x9 grid"),
                .args = &(TestIoGraphArgs){
                    .size = 9,
                    .type = TEST_GEN_GRAPH_TYPE_GRID,
                },
                .fn = test_io_graph_aug,
            },
            {
                .desc = aven_str("push pop augmented pyramid A_3"),
                .args = &(TestIoGraphArgs){
                    .size = 3,
                    .type = TEST_GEN_GRAPH_TYPE_PYRAMID,
                },
                .fn = test_io_graph_aug,
            },
            {
                .desc = aven_str("push pop augmented pyramid A_9"),
                .args = &(TestIoGraphArgs){
                    .size = 9,
                    .type = TEST_GEN_GRAPH_TYPE_PYRAMID,
                },
                .fn = test_io_graph_aug,
            },
        };
        AvenTestCaseSlice tcases = slice_array(tcase_data);

        aven_test(tcases, arena);
    }

#endif // TEST_IO_H
