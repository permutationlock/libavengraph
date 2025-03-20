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
            .message = "failed to push graph to writer",
            .error = wr_error,
        };
    }

    AvenIoReader reader = aven_io_reader_init_bytes(space);
    GraphIoResult rd_res = graph_io_pop(&reader, &arena);
    if (rd_res.error != 0) {
        return (AvenTestResult){
            .message = "failed to pop graph from reader",
            .error = rd_res.error,
        };
    }

    Graph read_graph = rd_res.payload;

    if (!graph_io_validate(read_graph)) {
        return (AvenTestResult){
            .message = "poped graph invalid",
            .error = 1,
        };        
    }
    
    if (read_graph.adj.len != graph.adj.len) {
        char fmt[] = "expected %lu vertices, found %lu";

        char *buffer = aven_arena_alloc(
            emsg_arena,
            sizeof(fmt) + 16,
            1,
            1
        );

        int len = sprintf(
            buffer,
            fmt,
            (unsigned long)graph.adj.len,
            (unsigned long)read_graph.adj.len
        );
        assert(len > 0);

        return (AvenTestResult){
            .error = 1,
            .message = buffer,
        };
    }

    if (read_graph.nb.len != graph.nb.len) {
        char fmt[] = "expected %lu half-edges, found %lu";

        char *buffer = aven_arena_alloc(
            emsg_arena,
            sizeof(fmt) + 16,
            1,
            1
        );

        int len = sprintf(
            buffer,
            fmt,
            (unsigned long)graph.nb.len,
            (unsigned long)read_graph.nb.len
        );
        assert(len > 0);

        return (AvenTestResult){
            .error = 1,
            .message = buffer,
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
        char fmt[] = "graph adj lists differed from original in %lu places";

        char *buffer = aven_arena_alloc(
            emsg_arena,
            sizeof(fmt) + 8,
            1,
            1
        );

        int len = sprintf(
            buffer,
            fmt,
            (unsigned long)inv_adj
        );
        assert(len > 0);

        return (AvenTestResult){
            .error = 1,
            .message = buffer,
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
            .message = "failed to push graph to writer",
            .error = wr_error,
        };
    }

    AvenIoReader reader = aven_io_reader_init_bytes(space);
    GraphIoAugResult rd_res = graph_io_aug_pop(&reader, &arena);
    if (rd_res.error != 0) {
        return (AvenTestResult){
            .message = "failed to pop graph from reader",
            .error = rd_res.error,
        };
    }

    GraphAug read_graph = rd_res.payload;

    if (!graph_io_aug_validate(read_graph)) {
        return (AvenTestResult){
            .message = "poped graph invalid",
            .error = 1,
        };        
    }
    
    if (read_graph.adj.len != aug_graph.adj.len) {
        char fmt[] = "expected %lu vertices, found %lu";

        char *buffer = aven_arena_alloc(
            emsg_arena,
            sizeof(fmt) + 16,
            1,
            1
        );

        int len = sprintf(
            buffer,
            fmt,
            (unsigned long)aug_graph.adj.len,
            (unsigned long)read_graph.adj.len
        );
        assert(len > 0);

        return (AvenTestResult){
            .error = 1,
            .message = buffer,
        };
    }

    if (read_graph.nb.len != aug_graph.nb.len) {
        char fmt[] = "expected %lu half-edges, found %lu";

        char *buffer = aven_arena_alloc(
            emsg_arena,
            sizeof(fmt) + 16,
            1,
            1
        );

        int len = sprintf(
            buffer,
            fmt,
            (unsigned long)aug_graph.nb.len,
            (unsigned long)read_graph.nb.len
        );
        assert(len > 0);

        return (AvenTestResult){
            .error = 1,
            .message = buffer,
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
        char fmt[] = "graph adj lists differed from original in %lu places";

        char *buffer = aven_arena_alloc(
            emsg_arena,
            sizeof(fmt) + 8,
            1,
            1
        );

        int len = sprintf(
            buffer,
            fmt,
            (unsigned long)inv_adj
        );
        assert(len > 0);

        return (AvenTestResult){
            .error = 1,
            .message = buffer,
        };
    }

    return (AvenTestResult){ 0 };
}

static void test_io(AvenArena arena) {
    AvenTestCase tcase_data[] = {
        {
            .desc = "push pop K_1",
            .args = &(TestIoGraphArgs){
                .size = 1,
                .type = TEST_GEN_GRAPH_TYPE_COMPLETE,
            },
            .fn = test_io_graph,
        },
        {
            .desc = "push pop K_5",
            .args = &(TestIoGraphArgs){
                .size = 5,
                .type = TEST_GEN_GRAPH_TYPE_COMPLETE,
            },
            .fn = test_io_graph,
        },
        {
            .desc = "push pop K_19",
            .args = &(TestIoGraphArgs){
                .size = 19,
                .type = TEST_GEN_GRAPH_TYPE_COMPLETE,
            },
            .fn = test_io_graph,
        },
        {
            .desc = "push pop 2x2 grid",
            .args = &(TestIoGraphArgs){
                .size = 2,
                .type = TEST_GEN_GRAPH_TYPE_GRID,
            },
            .fn = test_io_graph,
        },
        {
            .desc = "push pop 9x9 grid",
            .args = &(TestIoGraphArgs){
                .size = 9,
                .type = TEST_GEN_GRAPH_TYPE_GRID,
            },
            .fn = test_io_graph,
        },
        {
            .desc = "push pop pyramid A_3",
            .args = &(TestIoGraphArgs){
                .size = 3,
                .type = TEST_GEN_GRAPH_TYPE_PYRAMID,
            },
            .fn = test_io_graph,
        },
        {
            .desc = "push pop pyramid A_9",
            .args = &(TestIoGraphArgs){
                .size = 9,
                .type = TEST_GEN_GRAPH_TYPE_PYRAMID,
            },
            .fn = test_io_graph,
        },
        {
            .desc = "push pop augmented K_1",
            .args = &(TestIoGraphArgs){
                .size = 1,
                .type = TEST_GEN_GRAPH_TYPE_COMPLETE,
            },
            .fn = test_io_graph_aug,
        },
        {
            .desc = "push pop augmented K_5",
            .args = &(TestIoGraphArgs){
                .size = 5,
                .type = TEST_GEN_GRAPH_TYPE_COMPLETE,
            },
            .fn = test_io_graph_aug,
        },
        {
            .desc = "push pop augmented K_19",
            .args = &(TestIoGraphArgs){
                .size = 19,
                .type = TEST_GEN_GRAPH_TYPE_COMPLETE,
            },
            .fn = test_io_graph_aug,
        },
        {
            .desc = "push pop augmented 2x2 grid",
            .args = &(TestIoGraphArgs){
                .size = 2,
                .type = TEST_GEN_GRAPH_TYPE_GRID,
            },
            .fn = test_io_graph_aug,
        },
        {
            .desc = "push pop augmented 9x9 grid",
            .args = &(TestIoGraphArgs){
                .size = 9,
                .type = TEST_GEN_GRAPH_TYPE_GRID,
            },
            .fn = test_io_graph_aug,
        },
        {
            .desc = "push pop augmented pyramid A_3",
            .args = &(TestIoGraphArgs){
                .size = 3,
                .type = TEST_GEN_GRAPH_TYPE_PYRAMID,
            },
            .fn = test_io_graph_aug,
        },
        {
            .desc = "push pop augmented pyramid A_9",
            .args = &(TestIoGraphArgs){
                .size = 9,
                .type = TEST_GEN_GRAPH_TYPE_PYRAMID,
            },
            .fn = test_io_graph_aug,
        },
    };
    AvenTestCaseSlice tcases = slice_array(tcase_data);

    aven_test(tcases, __FILE__, arena);
}

#endif // TEST_IO_H

