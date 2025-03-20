#include <aven.h>
#include <aven/arena.h>
#include <aven/fs.h>
#include <aven/io.h>
#include <aven/math.h>
#include <aven/path.h>
#include <aven/rng.h>
#include <aven/rng/pcg.h>
#include <aven/str.h>
#include <aven/test.h>

#include <graph.h>
#include <graph/gen.h>
#include <graph/plane.h>
#include <graph/plane/gen.h>

typedef enum {
    TEST_PLANE_TYPE_COMPLETE,
    TEST_PLANE_TYPE_GRID,
    TEST_PLANE_TYPE_PYRAMID,
    TEST_PLANE_TYPE_TRIANGULATION,
} TestPlaneType;

typedef struct {
    bool planar;
    uint32_t size;
    TestPlaneType type;
} TestGraphPlaneArgs;

AvenTestResult test_graph_plane(
    AvenArena *emsg_arena,
    AvenArena arena,
    void *opaque_args
) {
    (void)emsg_arena;
    TestGraphPlaneArgs *args = opaque_args;

    AvenRngPcg pcg = aven_rng_pcg_seed(0xdead, 0xbeef);
    AvenRng rng = aven_rng_pcg(&pcg);

    Graph graph;
    switch (args->type) {
        case TEST_PLANE_TYPE_COMPLETE:
            graph = graph_gen_complete(args->size, &arena);
            break;
        case TEST_PLANE_TYPE_GRID:
            graph = graph_gen_grid(args->size, args->size, &arena);
            break;
        case TEST_PLANE_TYPE_PYRAMID:
            graph = graph_gen_pyramid(args->size, &arena);
            break;
        case TEST_PLANE_TYPE_TRIANGULATION:
            graph = graph_gen_triangulation(
                args->size,
                rng,
                (Vec2){ 0.33f, 0.33f },
                &arena
            );
            break;
        default:
            assert(false);
            break;
    }

    if (graph_plane_validate(graph, arena) != args->planar) {
        return (AvenTestResult){
            .message = "generated graph not a plane triangulation",
            .error = 1,
        };        
    }

    return (AvenTestResult){ 0 };
}

static void test_plane(AvenArena arena) {
    AvenTestCase tcase_data[] = {
        {
            .desc = "verify embedding K_1",
            .args = &(TestGraphPlaneArgs){
                .size = 1,
                .type = TEST_PLANE_TYPE_COMPLETE,
                .planar = true,
            },
            .fn = test_graph_plane,
        },
        {
            .desc = "verify embedding K_2",
            .args = &(TestGraphPlaneArgs){
                .size = 2,
                .type = TEST_PLANE_TYPE_COMPLETE,
                .planar = true,
            },
            .fn = test_graph_plane,
        },
        {
            .desc = "verify embedding K_3",
            .args = &(TestGraphPlaneArgs){
                .size = 3,
                .type = TEST_PLANE_TYPE_COMPLETE,
                .planar = true,
            },
            .fn = test_graph_plane,
        },
        {
            .desc = "verify embedding K_4",
            .args = &(TestGraphPlaneArgs){
                .size = 4,
                .type = TEST_PLANE_TYPE_COMPLETE,
                .planar = true,
            },
            .fn = test_graph_plane,
        },
        {
            .desc = "verify embedding K_5",
            .args = &(TestGraphPlaneArgs){
                .size = 5,
                .type = TEST_PLANE_TYPE_COMPLETE,
                .planar = false,
            },
            .fn = test_graph_plane,
        },
        {
            .desc = "verify embedding K_19",
            .args = &(TestGraphPlaneArgs){
                .size = 19,
                .type = TEST_PLANE_TYPE_COMPLETE,
                .planar = false,
            },
            .fn = test_graph_plane,
        },
        {
            .desc = "verify embedding 2x2 grid",
            .args = &(TestGraphPlaneArgs){
                .size = 2,
                .type = TEST_PLANE_TYPE_GRID,
                .planar = true,
            },
            .fn = test_graph_plane,
        },
        {
            .desc = "verify embedding 9x9 grid",
            .args = &(TestGraphPlaneArgs){
                .size = 9,
                .type = TEST_PLANE_TYPE_GRID,
                .planar = true,
            },
            .fn = test_graph_plane,
        },
        {
            .desc = "verify embedding pyramid A_3",
            .args = &(TestGraphPlaneArgs){
                .size = 3,
                .type = TEST_PLANE_TYPE_PYRAMID,
                .planar = true,
            },
            .fn = test_graph_plane,
        },
        {
            .desc = "verify embedding pyramid A_9",
            .args = &(TestGraphPlaneArgs){
                .size = 9,
                .type = TEST_PLANE_TYPE_PYRAMID,
                .planar = true,
            },
            .fn = test_graph_plane,
        },
        {
            .desc = "verify embedding order 9 triangulation",
            .args = &(TestGraphPlaneArgs){
                .size = 9,
                .type = TEST_PLANE_TYPE_TRIANGULATION,
                .planar = true,
            },
            .fn = test_graph_plane,
        },
        {
            .desc = "verify embedding order 21 triangulation",
            .args = &(TestGraphPlaneArgs){
                .size = 21,
                .type = TEST_PLANE_TYPE_TRIANGULATION,
                .planar = true,
            },
            .fn = test_graph_plane,
        },
    };
    AvenTestCaseSlice tcases = slice_array(tcase_data);

    aven_test(tcases, __FILE__, arena);
}

