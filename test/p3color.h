#ifndef TEST_P3COLOR_H
    #define TEST_P3COLOR_H

    #include <aven.h>
    #include <aven/arena.h>
    #include <aven/test.h>

    #include <graph.h>
    #include <graph/path_color.h>
    #include <graph/plane/p3color.h>
    #include <graph/plane/p3color_bfs.h>

    #include "gen.h"

    typedef enum {
        TEST_P3COLOR_ALG_BFS,
        TEST_P3COLOR_ALG_TRACE,
    } TestP3ColorAlg;

    typedef struct {
        uint32_t size;
        TestGenGraphType type;
        TestP3ColorAlg alg;
    } TestP3ColorArgs;

    static AvenTestResult test_p3color_graph(
        AvenArena *emsg_arena,
        AvenArena arena,
        void *opaque_args
    ) {
        (void)emsg_arena;
        TestP3ColorArgs *args = opaque_args;

        GraphGenTriangulation tri = test_gen_triangulation(
            args->size,
            args->type,
            &arena
        );

        uint32_t p1_arr[] = { get(tri.outer_face, 0) };
        uint32_t p2_arr[] = { get(tri.outer_face, 2), get(tri.outer_face, 1) };
        GraphSubset p1 = slice_array(p1_arr);
        GraphSubset p2 = slice_array(p2_arr);

        GraphPropUint8 coloring;
        switch (args->alg) {
            case TEST_P3COLOR_ALG_BFS:
                coloring = graph_plane_p3color(tri.graph, p1, p2, &arena);
                break;
            case TEST_P3COLOR_ALG_TRACE:
                coloring = graph_plane_p3color_bfs(tri.graph, p1, p2, &arena);
                break;
        }

        if (!graph_path_color_verify(tri.graph, coloring, arena)) {
            return (AvenTestResult){
                .error = 1,
                .message = aven_str("invalid path coloring"),
            };
        }

        return (AvenTestResult){ 0 };
    }

    static void test_p3color(AvenArena arena) {
        AvenTestCase tcase_data[] = {
            {
                .desc = aven_str("path color K_3 w/BFS"),
                .args = &(TestP3ColorArgs){
                    .size = 3,
                    .type = TEST_GEN_GRAPH_TYPE_COMPLETE,
                    .alg = TEST_P3COLOR_ALG_BFS,
                },
                .fn = test_p3color_graph,
            },
            {
                .desc = aven_str("path color K_4 w/BFS"),
                .args = &(TestP3ColorArgs){
                    .size = 4,
                    .type = TEST_GEN_GRAPH_TYPE_COMPLETE,
                    .alg = TEST_P3COLOR_ALG_BFS,
                },
                .fn = test_p3color_graph,
            },
            {
                .desc = aven_str("path color pyramid A_5 w/BFS"),
                .args = &(TestP3ColorArgs){
                    .size = 5,
                    .type = TEST_GEN_GRAPH_TYPE_PYRAMID,
                    .alg = TEST_P3COLOR_ALG_BFS,
                },
                .fn = test_p3color_graph,
            },
            {
                .desc = aven_str("path color pyramid A_19 w/BFS"),
                .args = &(TestP3ColorArgs){
                    .size = 19,
                    .type = TEST_GEN_GRAPH_TYPE_PYRAMID,
                    .alg = TEST_P3COLOR_ALG_BFS,
                },
                .fn = test_p3color_graph,
            },
            {
                .desc = aven_str("path color order 9 triangulation w/BFS"),
                .args = &(TestP3ColorArgs){
                    .size = 9,
                    .type = TEST_GEN_GRAPH_TYPE_TRIANGULATION_OLD,
                    .alg = TEST_P3COLOR_ALG_BFS,
                },
                .fn = test_p3color_graph,
            },
            {
                .desc = aven_str("path color order 19 triangulation w/BFS"),
                .args = &(TestP3ColorArgs){
                    .size = 19,
                    .type = TEST_GEN_GRAPH_TYPE_TRIANGULATION_OLD,
                    .alg = TEST_P3COLOR_ALG_BFS,
                },
                .fn = test_p3color_graph,
            },
            {
                .desc = aven_str("path color order 119 triangulation w/BFS"),
                .args = &(TestP3ColorArgs){
                    .size = 119,
                    .type = TEST_GEN_GRAPH_TYPE_TRIANGULATION_OLD,
                    .alg = TEST_P3COLOR_ALG_BFS,
                },
                .fn = test_p3color_graph,
            },
            {
                .desc = aven_str("path color order 1119 triangulation w/BFS"),
                .args = &(TestP3ColorArgs){
                    .size = 1119,
                    .type = TEST_GEN_GRAPH_TYPE_TRIANGULATION_OLD,
                    .alg = TEST_P3COLOR_ALG_BFS,
                },
                .fn = test_p3color_graph,
            },
            {
                .desc = aven_str("path color K_3"),
                .args = &(TestP3ColorArgs){
                    .size = 3,
                    .type = TEST_GEN_GRAPH_TYPE_COMPLETE,
                    .alg = TEST_P3COLOR_ALG_TRACE,
                },
                .fn = test_p3color_graph,
            },
            {
                .desc = aven_str("path color K_4"),
                .args = &(TestP3ColorArgs){
                    .size = 4,
                    .type = TEST_GEN_GRAPH_TYPE_COMPLETE,
                    .alg = TEST_P3COLOR_ALG_TRACE,
                },
                .fn = test_p3color_graph,
            },
            {
                .desc = aven_str("path color pyramid A_5"),
                .args = &(TestP3ColorArgs){
                    .size = 5,
                    .type = TEST_GEN_GRAPH_TYPE_PYRAMID,
                    .alg = TEST_P3COLOR_ALG_TRACE,
                },
                .fn = test_p3color_graph,
            },
            {
                .desc = aven_str("path color pyramid A_19"),
                .args = &(TestP3ColorArgs){
                    .size = 19,
                    .type = TEST_GEN_GRAPH_TYPE_PYRAMID,
                    .alg = TEST_P3COLOR_ALG_TRACE,
                },
                .fn = test_p3color_graph,
            },
            {
                .desc = aven_str("path color order 9 triangulation"),
                .args = &(TestP3ColorArgs){
                    .size = 9,
                    .type = TEST_GEN_GRAPH_TYPE_TRIANGULATION,
                    .alg = TEST_P3COLOR_ALG_TRACE,
                },
                .fn = test_p3color_graph,
            },
            {
                .desc = aven_str("path color order 19 triangulation"),
                .args = &(TestP3ColorArgs){
                    .size = 19,
                    .type = TEST_GEN_GRAPH_TYPE_TRIANGULATION,
                    .alg = TEST_P3COLOR_ALG_TRACE,
                },
                .fn = test_p3color_graph,
            },
            {
                .desc = aven_str("path color order 119 triangulation"),
                .args = &(TestP3ColorArgs){
                    .size = 119,
                    .type = TEST_GEN_GRAPH_TYPE_TRIANGULATION,
                    .alg = TEST_P3COLOR_ALG_TRACE,
                },
                .fn = test_p3color_graph,
            },
            {
                .desc = aven_str("path color order 1119 triangulation"),
                .args = &(TestP3ColorArgs){
                    .size = 1119,
                    .type = TEST_GEN_GRAPH_TYPE_TRIANGULATION,
                    .alg = TEST_P3COLOR_ALG_TRACE,
                },
                .fn = test_p3color_graph,
            },
            {
                .desc = aven_str("path color order 9 triangulation (old)"),
                .args = &(TestP3ColorArgs){
                    .size = 9,
                    .type = TEST_GEN_GRAPH_TYPE_TRIANGULATION_OLD,
                    .alg = TEST_P3COLOR_ALG_TRACE,
                },
                .fn = test_p3color_graph,
            },
            {
                .desc = aven_str("path color order 19 triangulation (old)"),
                .args = &(TestP3ColorArgs){
                    .size = 19,
                    .type = TEST_GEN_GRAPH_TYPE_TRIANGULATION_OLD,
                    .alg = TEST_P3COLOR_ALG_TRACE,
                },
                .fn = test_p3color_graph,
            },
            {
                .desc = aven_str("path color order 119 triangulation (old)"),
                .args = &(TestP3ColorArgs){
                    .size = 119,
                    .type = TEST_GEN_GRAPH_TYPE_TRIANGULATION_OLD,
                    .alg = TEST_P3COLOR_ALG_TRACE,
                },
                .fn = test_p3color_graph,
            },
            {
                .desc = aven_str("path color order 1119 triangulation (old)"),
                .args = &(TestP3ColorArgs){
                    .size = 1119,
                    .type = TEST_GEN_GRAPH_TYPE_TRIANGULATION_OLD,
                    .alg = TEST_P3COLOR_ALG_TRACE,
                },
                .fn = test_p3color_graph,
            },
        };
        AvenTestCaseSlice tcases = slice_array(tcase_data);

        aven_test(tcases, arena);
    }

#endif // TEST_P3COLOR_H
