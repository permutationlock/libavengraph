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
    GraphSubset p1;
    GraphSubset p2;
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

    Graph graph = test_gen_graph(args->size, args->type, &arena);

    GraphPropUint8 coloring;
    switch (args->alg) {
        case TEST_P3COLOR_ALG_BFS:
            coloring = graph_plane_p3color(graph, args->p1, args->p2, &arena);
            break;
        case TEST_P3COLOR_ALG_TRACE:
            coloring = graph_plane_p3color_bfs(graph, args->p1, args->p2, &arena);
            break;
    }

    if (!graph_path_color_verify(graph, coloring, arena)) {
        return (AvenTestResult){
            .error = 1,
            .message = "invalid path coloring",
        };
    }

    return (AvenTestResult){ 0 };
}

static void test_p3color(AvenArena arena) {
    AvenTestCase tcase_data[] = {
        {
            .desc = "path color K_3 w/BFS",
            .args = &(TestP3ColorArgs){
                .size = 3,
                .type = TEST_GEN_GRAPH_TYPE_COMPLETE,
                .alg = TEST_P3COLOR_ALG_BFS,
                .p1 = slice_array((uint32_t[]){ 0 }),
                .p2 = slice_array((uint32_t[]){ 2, 1 }),
            },
            .fn = test_p3color_graph,
        },
        {
            .desc = "path color K_4 w/BFS",
            .args = &(TestP3ColorArgs){
                .size = 4,
                .type = TEST_GEN_GRAPH_TYPE_COMPLETE,
                .alg = TEST_P3COLOR_ALG_BFS,
                .p1 = slice_array((uint32_t[]){ 0 }),
                .p2 = slice_array((uint32_t[]){ 3, 1 }),
            },
            .fn = test_p3color_graph,
        },
        {
            .desc = "path color pyramid A_5 w/BFS",
            .args = &(TestP3ColorArgs){
                .size = 5,
                .type = TEST_GEN_GRAPH_TYPE_PYRAMID,
                .alg = TEST_P3COLOR_ALG_BFS,
                .p1 = slice_array((uint32_t[]){ 0 }),
                .p2 = slice_array((uint32_t[]){ 2, 1 }),
            },
            .fn = test_p3color_graph,
        },
        {
            .desc = "path color pyramid A_19 w/BFS",
            .args = &(TestP3ColorArgs){
                .size = 19,
                .type = TEST_GEN_GRAPH_TYPE_PYRAMID,
                .alg = TEST_P3COLOR_ALG_BFS,
                .p1 = slice_array((uint32_t[]){ 0 }),
                .p2 = slice_array((uint32_t[]){ 2, 1 }),
            },
            .fn = test_p3color_graph,
        },
        {
            .desc = "path color order 9 triangulation w/BFS",
            .args = &(TestP3ColorArgs){
                .size = 9,
                .type = TEST_GEN_GRAPH_TYPE_TRIANGULATION,
                .alg = TEST_P3COLOR_ALG_BFS,
                .p1 = slice_array((uint32_t[]){ 0 }),
                .p2 = slice_array((uint32_t[]){ 2, 1 }),
            },
            .fn = test_p3color_graph,
        },
        {
            .desc = "path color order 19 triangulation w/BFS",
            .args = &(TestP3ColorArgs){
                .size = 19,
                .type = TEST_GEN_GRAPH_TYPE_TRIANGULATION,
                .alg = TEST_P3COLOR_ALG_BFS,
                .p1 = slice_array((uint32_t[]){ 0 }),
                .p2 = slice_array((uint32_t[]){ 2, 1 }),
            },
            .fn = test_p3color_graph,
        },
        {
            .desc = "path color order 119 triangulation w/BFS",
            .args = &(TestP3ColorArgs){
                .size = 119,
                .type = TEST_GEN_GRAPH_TYPE_TRIANGULATION,
                .alg = TEST_P3COLOR_ALG_BFS,
                .p1 = slice_array((uint32_t[]){ 0 }),
                .p2 = slice_array((uint32_t[]){ 2, 1 }),
            },
            .fn = test_p3color_graph,
        },
        {
            .desc = "path color order 1119 triangulation w/BFS",
            .args = &(TestP3ColorArgs){
                .size = 1119,
                .type = TEST_GEN_GRAPH_TYPE_TRIANGULATION,
                .alg = TEST_P3COLOR_ALG_BFS,
                .p1 = slice_array((uint32_t[]){ 0 }),
                .p2 = slice_array((uint32_t[]){ 2, 1 }),
            },
            .fn = test_p3color_graph,
        },
        {
            .desc = "path color K_3 w/BFS",
            .args = &(TestP3ColorArgs){
                .size = 3,
                .type = TEST_GEN_GRAPH_TYPE_COMPLETE,
                .alg = TEST_P3COLOR_ALG_TRACE,
                .p1 = slice_array((uint32_t[]){ 0 }),
                .p2 = slice_array((uint32_t[]){ 2, 1 }),
            },
            .fn = test_p3color_graph,
        },
        {
            .desc = "path color K_4 w/BFS",
            .args = &(TestP3ColorArgs){
                .size = 4,
                .type = TEST_GEN_GRAPH_TYPE_COMPLETE,
                .alg = TEST_P3COLOR_ALG_TRACE,
                .p1 = slice_array((uint32_t[]){ 0 }),
                .p2 = slice_array((uint32_t[]){ 3, 1 }),
            },
            .fn = test_p3color_graph,
        },
        {
            .desc = "path color pyramid A_5 w/BFS",
            .args = &(TestP3ColorArgs){
                .size = 5,
                .type = TEST_GEN_GRAPH_TYPE_PYRAMID,
                .alg = TEST_P3COLOR_ALG_TRACE,
                .p1 = slice_array((uint32_t[]){ 0 }),
                .p2 = slice_array((uint32_t[]){ 2, 1 }),
            },
            .fn = test_p3color_graph,
        },
        {
            .desc = "path color pyramid A_19 w/BFS",
            .args = &(TestP3ColorArgs){
                .size = 19,
                .type = TEST_GEN_GRAPH_TYPE_PYRAMID,
                .alg = TEST_P3COLOR_ALG_TRACE,
                .p1 = slice_array((uint32_t[]){ 0 }),
                .p2 = slice_array((uint32_t[]){ 2, 1 }),
            },
            .fn = test_p3color_graph,
        },
        {
            .desc = "path color order 9 triangulation w/BFS",
            .args = &(TestP3ColorArgs){
                .size = 9,
                .type = TEST_GEN_GRAPH_TYPE_TRIANGULATION,
                .alg = TEST_P3COLOR_ALG_TRACE,
                .p1 = slice_array((uint32_t[]){ 0 }),
                .p2 = slice_array((uint32_t[]){ 2, 1 }),
            },
            .fn = test_p3color_graph,
        },
        {
            .desc = "path color order 19 triangulation w/BFS",
            .args = &(TestP3ColorArgs){
                .size = 19,
                .type = TEST_GEN_GRAPH_TYPE_TRIANGULATION,
                .alg = TEST_P3COLOR_ALG_TRACE,
                .p1 = slice_array((uint32_t[]){ 0 }),
                .p2 = slice_array((uint32_t[]){ 2, 1 }),
            },
            .fn = test_p3color_graph,
        },
        {
            .desc = "path color order 119 triangulation w/BFS",
            .args = &(TestP3ColorArgs){
                .size = 119,
                .type = TEST_GEN_GRAPH_TYPE_TRIANGULATION,
                .alg = TEST_P3COLOR_ALG_TRACE,
                .p1 = slice_array((uint32_t[]){ 0 }),
                .p2 = slice_array((uint32_t[]){ 2, 1 }),
            },
            .fn = test_p3color_graph,
        },
        {
            .desc = "path color order 1119 triangulation w/BFS",
            .args = &(TestP3ColorArgs){
                .size = 1119,
                .type = TEST_GEN_GRAPH_TYPE_TRIANGULATION,
                .alg = TEST_P3COLOR_ALG_TRACE,
                .p1 = slice_array((uint32_t[]){ 0 }),
                .p2 = slice_array((uint32_t[]){ 2, 1 }),
            },
            .fn = test_p3color_graph,
        },
    };
    AvenTestCaseSlice tcases = slice_array(tcase_data);

    aven_test(tcases, __FILE__, arena);
}

#endif // TEST_P3COLOR_H
