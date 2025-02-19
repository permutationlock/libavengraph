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
#include <graph.h>
#include <graph/plane.h>
#include <graph/plane/gen.h>
#include <graph/plane/p3choose.h>
#include <graph/plane/p3choose/tikz.h>
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
    GraphAug graph;
    GraphPlaneEmbedding embedding;
    GraphPlaneP3ChooseCtx ctx;
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
    GraphPlaneGenData data = graph_plane_gen_tri(
        vertices,
        ident,
        min_area,
        min_step,
        true,
        rng,
        arena
    );

    test_case.graph = graph_aug(data.graph, arena);
    test_case.embedding = data.embedding;

    uint32_t outer_face_data[4] = { 0, 1, 2, 3, };
    GraphSubset outer_face = slice_array(outer_face_data);

    GraphPlaneP3ChooseListProp color_lists = aven_arena_create_slice(
        GraphPlaneP3ChooseList,
        arena,
        test_case.graph.len
    );

    for (uint32_t i = 0; i < color_lists.len; i += 1) {
        GraphPlaneP3ChooseList list = { .len = 3 };

        for (uint8_t j = 0; j < list.len; j += 1) {
            get(list, j) = (uint8_t)(1 + aven_rng_rand_bounded(rng, max_color));
        }

        while (get(list, 1) == get(list, 0)) {
            get(list, 1) = (uint8_t)(1 + aven_rng_rand_bounded(
                rng,
                max_color
            ));
        }
        while (
            get(list, 2) == get(list, 1) or
            get(list, 2) == get(list, 0)
        ) {
            get(list, 2) = (uint8_t)(1 + aven_rng_rand_bounded(
                rng,
                max_color
            ));
        }

        get(color_lists, i) = list;
    }

    test_case.ctx = graph_plane_p3choose_init(
        test_case.graph,
        color_lists,
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

        GraphPlaneP3ChooseFrameOptional frame =
            graph_plane_p3choose_next_frame(&test_case.ctx);

        do {
            do {
                cases[
                    graph_plane_p3choose_frame_case(
                        &test_case.ctx,
                        &frame.value
                    )
                ] = true;
                steps += 1;
            } while (
                !graph_plane_p3choose_frame_step(
                    &test_case.ctx,
                    &frame.value
                )
            );

            frame = graph_plane_p3choose_next_frame(&test_case.ctx);
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

    GraphPlaneP3ChooseFrameOptional frame =
        graph_plane_p3choose_next_frame(&test_case.ctx);

    int count = 0;
    do {
        do {
            graph_plane_p3choose_tikz(
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
            !graph_plane_p3choose_frame_step(&test_case.ctx, &frame.value)
        );

        frame = graph_plane_p3choose_next_frame(&test_case.ctx);
    } while (frame.valid);

    return 0;
}
