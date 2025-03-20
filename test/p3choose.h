#ifndef TEST_P3CHOOSE_H
#define TEST_P3CHOOSE_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/test.h>

#include <graph.h>
#include <graph/path_color.h>
#include <graph/plane/p3choose.h>

#include "gen.h"

typedef struct {
    uint32_t size;
    GraphSubset outer_face;
    GraphPlaneP3ChooseListProp list_assignment;
    TestGenGraphType type;
} TestP3ChooseArgs;

static AvenTestResult test_p3choose_graph(
    AvenArena *emsg_arena,
    AvenArena arena,
    void *opaque_args
) {
    (void)emsg_arena;
    TestP3ChooseArgs *args = opaque_args;

    Graph graph = test_gen_graph(args->size, args->type, &arena);
    GraphAug aug_graph = graph_aug(graph, &arena);

    assert(graph.adj.len == args->list_assignment.len);

    GraphPropUint8 coloring = graph_plane_p3choose(
        aug_graph,
        args->list_assignment,
        args->outer_face,
        &arena
    );

    if (
        !graph_plane_p3choose_verify_list_coloring(
            args->list_assignment,
            coloring
        )
    ) {
        return (AvenTestResult){
            .error = 1,
            .message = "invalid list coloring",
        };
    }

    if (!graph_path_color_verify(graph, coloring, arena)) {
        return (AvenTestResult){
            .error = 1,
            .message = "invalid path coloring",
        };
    }

    return (AvenTestResult){ 0 };
}

static void test_p3choose(AvenArena arena) {
    AvenTestCase tcase_data[] = {
        {
            .desc = "path choose K_3 w/BFS",
            .args = &(TestP3ChooseArgs){
                .size = 3,
                .type = TEST_GEN_GRAPH_TYPE_COMPLETE,
                .outer_face = slice_array((uint32_t[]){ 0, 1, 2 }),
                .list_assignment = slice_array(
                    (GraphPlaneP3ChooseList[]){
                        { .len = 2, .ptr = { 1, 2 } },
                        { .len = 2, .ptr = { 1, 2 } },
                        { .len = 2, .ptr = { 1, 2 } },
                    }
                ),
            },
            .fn = test_p3choose_graph,
        },
        {
            .desc = "path choose K_4 w/BFS",
            .args = &(TestP3ChooseArgs){
                .size = 4,
                .type = TEST_GEN_GRAPH_TYPE_COMPLETE,
                .outer_face = slice_array((uint32_t[]){ 0, 1, 3 }),
                .list_assignment = slice_array(
                    (GraphPlaneP3ChooseList[]){
                        { .len = 2, .ptr = { 1, 2 } },
                        { .len = 2, .ptr = { 1, 2 } },
                        { .len = 3, .ptr = { 1, 2, 3 } },
                        { .len = 2, .ptr = { 1, 2 } },
                    }
                ),
            },
            .fn = test_p3choose_graph,
        },
        {
            .desc = "path choose pyramid A_5 w/BFS",
            .args = &(TestP3ChooseArgs){
                .size = 5,
                .type = TEST_GEN_GRAPH_TYPE_PYRAMID,
                .outer_face = slice_array((uint32_t[]){ 0, 1, 2 }),
                .list_assignment = slice_array(
                    (GraphPlaneP3ChooseList[]){
                        { .len = 2, .ptr = { 1, 2 } },
                        { .len = 2, .ptr = { 2, 3 } },
                        { .len = 2, .ptr = { 3, 1 } },
                        { .len = 3, .ptr = { 1, 2, 4 } },
                        { .len = 3, .ptr = { 2, 4, 3 } },
                        { .len = 3, .ptr = { 1, 2, 4 } },
                        { .len = 3, .ptr = { 1, 4, 3 } },
                        { .len = 3, .ptr = { 1, 2, 4 } },
                        { .len = 3, .ptr = { 1, 4, 3 } },
                        { .len = 3, .ptr = { 1, 2, 4 } },
                        { .len = 3, .ptr = { 2, 4, 3 } },
                        { .len = 3, .ptr = { 1, 2, 4 } },
                        { .len = 3, .ptr = { 1, 4, 3 } },
                        { .len = 3, .ptr = { 1, 2, 4 } },
                        { .len = 3, .ptr = { 1, 4, 3 } },
                        { .len = 3, .ptr = { 1, 2, 4 } },
                        { .len = 3, .ptr = { 2, 4, 3 } },
                        { .len = 3, .ptr = { 1, 2, 4 } },
                    }
                ),
            },
            .fn = test_p3choose_graph,
        },
        {
            .desc = "path choose order 18 triangulation",
            .args = &(TestP3ChooseArgs){
                .size = 18,
                .type = TEST_GEN_GRAPH_TYPE_TRIANGULATION,
                .outer_face = slice_array((uint32_t[]){ 0, 1, 2 }),
                .list_assignment = slice_array(
                    (GraphPlaneP3ChooseList[]){
                        { .len = 2, .ptr = { 1, 2 } },
                        { .len = 2, .ptr = { 2, 3 } },
                        { .len = 2, .ptr = { 3, 1 } },
                        { .len = 3, .ptr = { 1, 2, 4 } },
                        { .len = 3, .ptr = { 2, 4, 3 } },
                        { .len = 3, .ptr = { 1, 2, 4 } },
                        { .len = 3, .ptr = { 1, 4, 3 } },
                        { .len = 3, .ptr = { 1, 2, 4 } },
                        { .len = 3, .ptr = { 1, 4, 3 } },
                        { .len = 3, .ptr = { 1, 2, 4 } },
                        { .len = 3, .ptr = { 2, 4, 3 } },
                        { .len = 3, .ptr = { 1, 2, 4 } },
                        { .len = 3, .ptr = { 1, 4, 3 } },
                        { .len = 3, .ptr = { 1, 2, 4 } },
                        { .len = 3, .ptr = { 1, 4, 3 } },
                        { .len = 3, .ptr = { 1, 2, 4 } },
                        { .len = 3, .ptr = { 2, 4, 3 } },
                        { .len = 3, .ptr = { 1, 2, 4 } },
                    }
                ),
            },
            .fn = test_p3choose_graph,
        },
    };
    AvenTestCaseSlice tcases = slice_array(tcase_data);

    aven_test(tcases, __FILE__, arena);
}

#endif // TEST_P3CHOOSE_H
