/*
 * A program to find complete, short algorithm examples and generate a tikz
 * drawing of each step for use in a latex paper.
 *
 * Randomly generate triangulations and 3-list-assignments, then apply the path
 * 3-list-coloring algorithm. Keep going until we find a setup that hits a
 * sufficient number of cases of the algorithm.
 */

#define AVEN_IMPLEMENTATION

#include <aven.h>
#include <aven/arena.h>
#include <aven/graph.h>
#include <aven/graph/plane.h>
#include <aven/graph/plane/gen.h>
#include <aven/graph/plane/hartman.h>
#include <aven/graph/plane/hartman/tikz.h>
#include <aven/math.h>
#include <aven/rng.h>
#include <aven/rng/pcg.h>

#include <stdio.h>
#include <stdlib.h>

#define MAX_COLOR 5
#define VERTICES 9
#define MIN_AREA 0.045f
#define MIN_STEP 0.0001f

#define ARENA_SIZE (4096 * 100)

typedef struct {
    AvenGraphAug graph;
    AvenGraphPlaneEmbedding embedding;
    AvenGraphPlaneHartmanCtx ctx;
} TestCase;

static TestCase gen_test_case(
    uint32_t vertices,
    float min_area,
    float min_step,
    uint32_t max_color,
    AvenRng rng,
    AvenArena *arena
) {
    TestCase test_case = { 0 };

    Aff2 ident;
    aff2_identity(ident);
    AvenGraphPlaneGenData data = aven_graph_plane_gen_tri(
        vertices,
        ident,
        min_area,
        min_step,
        true,
        rng,
        arena
    );

    test_case.graph = aven_graph_aug(data.graph, arena);
    test_case.embedding = data.embedding;

    uint32_t outer_face_data[4] = { 0, 1, 2, 3, };
    AvenGraphSubset outer_face = slice_array(outer_face_data);

    AvenGraphPlaneHartmanListProp color_lists = aven_arena_create_slice(
        AvenGraphPlaneHartmanList,
        arena,
        test_case.graph.len
    );

    for (uint32_t i = 0; i < color_lists.len; i += 1) {
        AvenGraphPlaneHartmanList list = {
            .len = 3,
            .ptr = {
                1 + aven_rng_rand_bounded(rng, max_color),
                1 + aven_rng_rand_bounded(rng, max_color),
                1 + aven_rng_rand_bounded(rng, max_color),
            },
        };

        while (get(list, 1) == get(list, 0)) {
            get(list, 1) = 1 + aven_rng_rand_bounded(
                rng,
                max_color
            );
        }
        while (
            get(list, 2) == get(list, 1) or
            get(list, 2) == get(list, 0)
        ) {
            get(list, 2) = 1 + aven_rng_rand_bounded(
                rng,
                max_color
            );
        }

        get(color_lists, i) = list;
    }
    
    test_case.ctx = aven_graph_plane_hartman_init(
        color_lists,
        test_case.graph,
        outer_face,
        arena
    );

    return test_case;
}

int main(void) {
    void *mem = malloc(ARENA_SIZE);
    if (mem == NULL) {
        fprintf(stderr, "ERROR: arena malloc failed\n");
        return 1;
    }
    AvenArena arena = aven_arena_init(mem, ARENA_SIZE);

    AvenRngPcg pcg_ctx = aven_rng_pcg_seed(0x1234ff79, 0xdef12345);
    AvenRng rng = aven_rng_pcg(&pcg_ctx);

    Aff2 ident;
    aff2_identity(ident);

    AvenRngPcg last_state;

    int satisfied = 0;
    int steps = 0;
    while (satisfied < 10 or steps > 16) {
        last_state = pcg_ctx;
        satisfied = 0;
        steps = 0;
        AvenArena temp_arena = arena;

        TestCase test_case = gen_test_case(
            VERTICES,
            MIN_AREA,
            MIN_STEP,
            MAX_COLOR,
            rng,
            &temp_arena
        );

        bool cases[12] = { 0 };

        AvenGraphPlaneHartmanFrameOptional frame =
            aven_graph_plane_hartman_next_frame(&test_case.ctx);

        do {
            do {
                cases[
                    aven_graph_plane_hartman_frame_case(
                        &test_case.ctx,
                        &frame.value
                    )
                ] = true;
                steps += 1;
            } while (
                !aven_graph_plane_hartman_frame_step(
                    &test_case.ctx,
                    &frame.value
                )
            );

            frame = aven_graph_plane_hartman_next_frame(&test_case.ctx);
        } while (frame.valid);

        for (size_t i = 0; i < countof(cases); i += 1) {
            if (cases[i]) {
                satisfied += 1;
            }
        }
    }


    pcg_ctx = last_state;

    TestCase test_case = gen_test_case(
        VERTICES,
        MIN_AREA,
        MIN_STEP,
        MAX_COLOR,
        rng,
        &arena
    );

    AvenGraphPlaneHartmanFrameOptional frame =
        aven_graph_plane_hartman_next_frame(&test_case.ctx);

    int count = 0;
    do {
        do {
            aven_graph_plane_hartman_tikz(
                test_case.embedding,
                &test_case.ctx,
                &frame.value,
                (Vec2){ 4.75f, 4.0f },
                arena
            );
            count += 1;
            if (count % 2 == 0) {
                printf("\n");
            } else {
                printf("$\\qquad$\n");
            }
        } while (
            !aven_graph_plane_hartman_frame_step(&test_case.ctx, &frame.value)
        );

        frame = aven_graph_plane_hartman_next_frame(&test_case.ctx);
    } while (frame.valid);

    return 0;
}
