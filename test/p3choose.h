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

        GraphGenTriangulation tri = test_gen_triangulation(
            args->size,
            args->type,
            &arena
        );
        GraphAug aug_graph = graph_aug(tri.graph, &arena);

        assert(aug_graph.adj.len == args->list_assignment.len);

        GraphPropUint8 coloring = graph_plane_p3choose(
            aug_graph,
            args->list_assignment,
            tri.outer_face,
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
                .message = aven_str("invalid list coloring"),
            };
        }

        if (!graph_path_color_verify(tri.graph, coloring, arena)) {
            return (AvenTestResult){
                .error = 1,
                .message = aven_str("invalid path coloring"),
            };
        }

        return (AvenTestResult){ 0 };
    }

    static void test_p3choose(AvenArena arena) {
        AvenTestCase tcase_data[] = {
            {
                .desc = aven_str("path choose K_3"),
                .args = &(TestP3ChooseArgs){
                    .size = 3,
                    .type = TEST_GEN_GRAPH_TYPE_COMPLETE,
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
                .desc = aven_str("path choose K_4"),
                .args = &(TestP3ChooseArgs){
                    .size = 4,
                    .type = TEST_GEN_GRAPH_TYPE_COMPLETE,
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
                .desc = aven_str("path choose pyramid A_5"),
                .args = &(TestP3ChooseArgs){
                    .size = 5,
                    .type = TEST_GEN_GRAPH_TYPE_PYRAMID,
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
                .desc = aven_str("path choose order 18 triangulation"),
                .args = &(TestP3ChooseArgs){
                    .size = 18,
                    .type = TEST_GEN_GRAPH_TYPE_TRIANGULATION,
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
                .desc = aven_str("path choose order 18 triangulation (old)"),
                .args = &(TestP3ChooseArgs){
                    .size = 18,
                    .type = TEST_GEN_GRAPH_TYPE_TRIANGULATION_OLD,
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

        aven_test(tcases, arena);
    }

#endif // TEST_P3CHOOSE_H
