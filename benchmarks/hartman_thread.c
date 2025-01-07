#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
    #define _POSIX_C_SOURCE 200112L
#endif

#define AVEN_IMPLEMENTATION
#include <aven.h>
#include <aven/arena.h>
#include <aven/fs.h>
#include <aven/math.h>
#include <aven/graph.h>
#include <aven/graph/path_color.h>
#include <aven/graph/plane.h>
#include <aven/graph/plane/hartman.h>
#include <aven/graph/plane/hartman/thread.h>
#include <aven/graph/plane/gen.h>
#include <aven/path.h>
#include <aven/rng.h>
#include <aven/rng/pcg.h>
#include <aven/time.h>
#include <aven/thread_pool.h>

#include <float.h>
#include <stdio.h>
#include <stdlib.h>

#define ARENA_SIZE (4096UL * 1200000UL)

#define NGRAPHS 3
#define MAX_VERTICES 10000001

#define MAX_COLOR 127

#define NTHREADS 3

int main(void) {
    void *mem = malloc(ARENA_SIZE);
    if (mem == NULL) {
        fprintf(stderr, "ERROR: arena malloc failed\n");
        return 1;
    }
    AvenArena arena = aven_arena_init(mem, ARENA_SIZE);

    AvenRngPcg pcg_ctx = aven_rng_pcg_seed(0x12345679, 0xdef12345);
    AvenRng rng = aven_rng_pcg(&pcg_ctx);

    AvenThreadPool thread_pool = aven_thread_pool_init(
        NTHREADS - 1,
        NTHREADS - 1,
        &arena
    );
    aven_thread_pool_run(&thread_pool);

    Aff2 ident;
    aff2_identity(ident);

    for (uint32_t n = 10; n < MAX_VERTICES; n *= 10) {
        AvenArena temp_arena = arena;

        typedef struct {
            AvenGraph graph;
            AvenGraphPlaneHartmanListProp color_lists;
        } CaseData;

        Slice(CaseData) cases = { .len = NGRAPHS * (MAX_VERTICES / n) };
        cases.ptr = aven_arena_create_array(
            CaseData,
            &temp_arena,
            cases.len
        );

        for (uint32_t i = 0; i < cases.len; i += 1) {
            AvenGraph graph = aven_graph_plane_gen_tri_abs(
                n,
                rng,
                &temp_arena
            );
            get(cases, i).graph = graph;
            if (graph.len != n) {
                aven_panic("graph generation failed");
            }

            AvenGraphPlaneHartmanListProp *color_lists = &get(
                cases,
                i
            ).color_lists;

            color_lists->len = n;
            color_lists->ptr = aven_arena_create_array(
                AvenGraphPlaneHartmanList,
                &temp_arena,
                n
            );

            for (uint32_t j = 0; j < color_lists->len; j += 1) {
                AvenGraphPlaneHartmanList list = {
                    .len = 3,
                    .ptr = {
                        1 + aven_rng_rand_bounded(rng, MAX_COLOR),
                        1 + aven_rng_rand_bounded(rng, MAX_COLOR),
                        1 + aven_rng_rand_bounded(rng, MAX_COLOR),
                    },
                };

                while (get(list, 1) == get(list, 0)) {
                    get(list, 1) = 1 + aven_rng_rand_bounded(
                        rng,
                        MAX_COLOR
                    );
                }
                while (
                    get(list, 2) == get(list, 1) or
                    get(list, 2) == get(list, 0)
                ) {
                    get(list, 2) = 1 + aven_rng_rand_bounded(
                        rng,
                        MAX_COLOR
                    );
                }

                get(*color_lists, j) = list;
            }
        }

        uint32_t face_data[3] = { 0, 1, 2 };
        AvenGraphSubset face = slice_array(face_data);

        __asm volatile("" ::: "memory");
        AvenTimeInst start_inst = aven_time_now();
        __asm volatile("" ::: "memory");

        for (uint32_t i = 0; i < cases.len; i += 1) {
            aven_graph_plane_hartman_thread(
                get(cases, i).color_lists,
                get(cases, i).graph,
                face,
                &thread_pool,
                temp_arena
            );
        }

        __asm volatile("" ::: "memory");
        AvenTimeInst end_inst = aven_time_now();
        __asm volatile("" ::: "memory");

        int64_t elapsed_ns = aven_time_since(end_inst, start_inst);
        double ns_per_graph = (double)elapsed_ns / (double)cases.len;

        uint32_t nvalid = 0;
        for (uint32_t i = 0; i < cases.len; i += 1) {
            AvenArena color_arena = temp_arena;
            AvenGraphPropUint8 coloring = { .len = n };
            coloring.ptr = aven_arena_create_array(uint8_t, &color_arena, n);

            AvenGraphPlaneHartmanListProp color_lists = get(
                cases,
                i
            ).color_lists;

            bool valid = true;
            for (uint32_t v = 0; v < n; v += 1) {
                if (get(color_lists, v).len != 1) {
                    valid = false;
                    break;
                }
                get(coloring, v) = (uint8_t)get(get(color_lists, v), 0);
            }

            if (valid) {
                valid = aven_graph_path_color_verify(
                    get(cases, i).graph,
                    coloring,
                    color_arena
                );
            }

            if (valid) {
                nvalid += 1;
            }
        }

        printf(
            "path 3-choosing %lu graph(s) with %lu vertices:\n"
            "\tvalid 3-choosings: %lu\n"
            "\ttime per graph: %fns\n"
            "\ttime per half-edge: %fns\n",
            (unsigned long)cases.len,
            (unsigned long)n,
            (unsigned long)nvalid,
            ns_per_graph,
            ns_per_graph / (double)(6 * n - 12)
        );
    }

    aven_thread_pool_halt_and_destroy(&thread_pool);

    return 0;
}
