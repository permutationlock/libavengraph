/*
 * A program to find complete, short algorithm examples and generate a tikz
 * drawing of each step for use in a latex paper.
 */

#define AVEN_IMPLEMENTATION

#include <aven.h>
#include <aven/arena.h>
#include <graph.h>
#include <graph/plane.h>
#include <graph/plane/gen.h>
#include <graph/plane/poh.h>
#include <graph/plane/poh_bfs.h>
#include <graph/plane/poh/tikz.h>
#include <aven/math.h>
#include <aven/rng.h>
#include <aven/rng/pcg.h>

#include <stdio.h>
#include <stdlib.h>

#define VERTICES 9
#define MIN_AREA 0.045f
#define MIN_STEP 0.0001f

#define ARENA_SIZE (4096 * 100)

typedef struct {
    Graph graph;
    GraphPlaneEmbedding embedding;
    GraphPlanePohCtx ctx;
    GraphPropUint8 bfs_coloring;
} TestCase;

static TestCase gen_test_case(
    uint32_t vertices,
    float min_area,
    float min_step,
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

    test_case.graph = data.graph;
    test_case.embedding = data.embedding;

    uint32_t p_data[] = { 0, };
    uint32_t q_data[] = { 3, 2, 1 };
    GraphSubset p = slice_array(p_data);
    GraphSubset q = slice_array(q_data);
    
    test_case.ctx = graph_plane_poh_init(
        test_case.graph,
        p,
        q,
        arena
    );

    test_case.bfs_coloring = graph_plane_poh_bfs(
        test_case.graph,
        p,
        q,
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

    AvenRngPcg pcg_ctx = aven_rng_pcg_seed(0x55343279, 0xdef123ff);
    AvenRng rng = aven_rng_pcg(&pcg_ctx);

    Aff2 ident;
    aff2_identity(ident);

    AvenRngPcg last_state;

    int satisfied = 0;
    int steps = 0;
    bool same_as_bfs = true;
    while (satisfied < 11 or steps > 23 or same_as_bfs) {
        last_state = pcg_ctx;
        satisfied = 0;
        steps = 0;
        AvenArena temp_arena = arena;

        TestCase test_case = gen_test_case(
            VERTICES,
            MIN_AREA,
            MIN_STEP,
            rng,
            &temp_arena
        );

        bool cases[AVEN_GRAPH_PLANE_POH_CASE_MAX] = { 0 };

        GraphPlanePohFrameOptional frame =
            graph_plane_poh_next_frame(&test_case.ctx);
        GraphPlanePohFrame last_frame = { .u = (uint32_t)(-1) };

        do {
            do {
                cases[
                    graph_plane_poh_frame_case(
                        &test_case.ctx,
                        &frame.value
                    )
                ] = true;
                if (
                    frame.value.u != last_frame.u or
                    frame.value.x != last_frame.x or
                    frame.value.y != last_frame.y or
                    frame.value.z != last_frame.z or
                    frame.value.edge_index == get(
                        test_case.ctx.vertex_info,
                        frame.value.u
                    ).len
                ) {
                    steps += 1;
                }
                last_frame = frame.value;
            } while (
                !graph_plane_poh_frame_step(
                    &test_case.ctx,
                    &frame.value
                )
            );

            frame = graph_plane_poh_next_frame(&test_case.ctx);
        } while (frame.valid);

        for (size_t i = 0; i < countof(cases); i += 1) {
            if (cases[i]) {
                satisfied += 1;
            }
        }

        same_as_bfs = true;
        for (uint32_t v = 0; v < test_case.bfs_coloring.len; v += 1) {
            int32_t bfs_color = (int32_t)get(test_case.bfs_coloring, v);
            same_as_bfs = get(test_case.ctx.vertex_info, v).mark == bfs_color;
            if (!same_as_bfs) {
                break;
            }
        }
    }

    pcg_ctx = last_state;

    TestCase test_case = gen_test_case(
        VERTICES,
        MIN_AREA,
        MIN_STEP,
        rng,
        &arena
    );

    GraphPlanePohFrameOptional frame =
        graph_plane_poh_next_frame(&test_case.ctx);
    GraphPlanePohFrame last_frame = { .u = (uint32_t)(-1) };

    int count = 0;
    do {
        do {
            if (
                frame.value.u != last_frame.u or
                frame.value.x != last_frame.x or
                frame.value.y != last_frame.y or
                frame.value.z != last_frame.z or
                frame.value.edge_index == get(
                    test_case.ctx.vertex_info,
                    frame.value.u
                ).len
            ) {
                graph_plane_poh_tikz(
                    test_case.embedding,
                    &test_case.ctx,
                    &frame.value,
                    (Vec2){ 3.75f, 4.0f },
                    arena
                );
                count += 1;
                if (count % 3 == 0) {
                    printf("\n");
                } else {
                    //printf("$\\qquad$\n");
                }
            }
            last_frame = frame.value;
        } while (
            !graph_plane_poh_frame_step(&test_case.ctx, &frame.value)
        );

        frame = graph_plane_poh_next_frame(&test_case.ctx);
    } while (frame.valid);

    return 0;
}
