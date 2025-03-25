#ifndef TEST_PLANE_H
#define TEST_PLANE_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/str.h>
#include <aven/test.h>

#include <graph.h>
#include <graph/plane.h>

#include "gen.h"

typedef struct {
    bool planar;
    uint32_t size;
    TestGenGraphType type;
} TestGraphPlaneArgs;

AvenTestResult test_graph_plane(
    AvenArena *emsg_arena,
    AvenArena arena,
    void *opaque_args
) {
    (void)emsg_arena;
    TestGraphPlaneArgs *args = opaque_args;

    Graph graph = test_gen_graph(args->size, args->type, &arena);

    if (graph_plane_validate(graph, arena) != args->planar) {
        return (AvenTestResult){
            .message = aven_str("generated graph not a plane triangulation"),
            .error = 1,
        };        
    }

    return (AvenTestResult){ 0 };
}

static void test_plane(AvenArena arena) {
    AvenTestCase tcase_data[] = {
        {
            .desc = aven_str("verify embedding K_1"),
            .args = &(TestGraphPlaneArgs){
                .size = 1,
                .type = TEST_GEN_GRAPH_TYPE_COMPLETE,
                .planar = true,
            },
            .fn = test_graph_plane,
        },
        {
            .desc = aven_str("verify embedding K_2"),
            .args = &(TestGraphPlaneArgs){
                .size = 2,
                .type = TEST_GEN_GRAPH_TYPE_COMPLETE,
                .planar = true,
            },
            .fn = test_graph_plane,
        },
        {
            .desc = aven_str("verify embedding K_3"),
            .args = &(TestGraphPlaneArgs){
                .size = 3,
                .type = TEST_GEN_GRAPH_TYPE_COMPLETE,
                .planar = true,
            },
            .fn = test_graph_plane,
        },
        {
            .desc = aven_str("verify embedding K_4"),
            .args = &(TestGraphPlaneArgs){
                .size = 4,
                .type = TEST_GEN_GRAPH_TYPE_COMPLETE,
                .planar = true,
            },
            .fn = test_graph_plane,
        },
        {
            .desc = aven_str("verify embedding K_5"),
            .args = &(TestGraphPlaneArgs){
                .size = 5,
                .type = TEST_GEN_GRAPH_TYPE_COMPLETE,
                .planar = false,
            },
            .fn = test_graph_plane,
        },
        {
            .desc = aven_str("verify embedding K_19"),
            .args = &(TestGraphPlaneArgs){
                .size = 19,
                .type = TEST_GEN_GRAPH_TYPE_COMPLETE,
                .planar = false,
            },
            .fn = test_graph_plane,
        },
        {
            .desc = aven_str("verify embedding 2x2 grid"),
            .args = &(TestGraphPlaneArgs){
                .size = 2,
                .type = TEST_GEN_GRAPH_TYPE_GRID,
                .planar = true,
            },
            .fn = test_graph_plane,
        },
        {
            .desc = aven_str("verify embedding 9x9 grid"),
            .args = &(TestGraphPlaneArgs){
                .size = 9,
                .type = TEST_GEN_GRAPH_TYPE_GRID,
                .planar = true,
            },
            .fn = test_graph_plane,
        },
        {
            .desc = aven_str("verify embedding pyramid A_3"),
            .args = &(TestGraphPlaneArgs){
                .size = 3,
                .type = TEST_GEN_GRAPH_TYPE_PYRAMID,
                .planar = true,
            },
            .fn = test_graph_plane,
        },
        {
            .desc = aven_str("verify embedding pyramid A_9"),
            .args = &(TestGraphPlaneArgs){
                .size = 9,
                .type = TEST_GEN_GRAPH_TYPE_PYRAMID,
                .planar = true,
            },
            .fn = test_graph_plane,
        },
        {
            .desc = aven_str("verify embedding order 9 triangulation"),
            .args = &(TestGraphPlaneArgs){
                .size = 9,
                .type = TEST_GEN_GRAPH_TYPE_TRIANGULATION,
                .planar = true,
            },
            .fn = test_graph_plane,
        },
        {
            .desc = aven_str("verify embedding order 21 triangulation"),
            .args = &(TestGraphPlaneArgs){
                .size = 21,
                .type = TEST_GEN_GRAPH_TYPE_TRIANGULATION,
                .planar = true,
            },
            .fn = test_graph_plane,
        },
    };
    AvenTestCaseSlice tcases = slice_array(tcase_data);

    aven_test(tcases, arena);
}

#endif // TEST_PLANE_H
