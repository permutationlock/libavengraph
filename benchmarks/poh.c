#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
    #define _POSIX_C_SOURCE 200112L
#endif

#define AVEN_IMPLEMENTATION
#include <aven.h>
#include <aven/arena.h>
#include <aven/fs.h>
#include <aven/math.h>
#include <graph.h>
#include <graph/path_color.h>
#include <graph/plane.h>
#include <graph/plane/p3color.h>
#include <graph/plane/gen.h>
#include <aven/path.h>
#include <aven/rng.h>
#include <aven/rng/pcg.h>
#include <aven/time.h>

#include <float.h>
#include <stdio.h>
#include <stdlib.h>

#define ARENA_SIZE (4096UL * 1200000UL)

#define NGRAPHS 1
#define MAX_VERTICES 10000001

int main(void) {
    void *mem = malloc(ARENA_SIZE);
    if (mem == NULL) {
        fprintf(stderr, "ERROR: arena malloc failed\n");
        return 1;
    }
    AvenArena arena = aven_arena_init(mem, ARENA_SIZE);

    AvenRngPcg pcg_ctx = aven_rng_pcg_seed(0x12345679, 0xdef12345);
    AvenRng rng = aven_rng_pcg(&pcg_ctx);

    Aff2 ident;
    aff2_identity(ident);

    for (uint32_t n = 1000; n < MAX_VERTICES; n *= 10) {
        AvenArena temp_arena = arena;

        typedef struct {
            Graph graph;
            GraphPropUint8 coloring;
        } CaseData;

        Slice(CaseData) cases = { .len = NGRAPHS * (MAX_VERTICES / n) };
        cases.ptr = aven_arena_create_array(
            CaseData,
            &temp_arena,
            cases.len
        );

        for (uint32_t i = 0; i < cases.len; i += 1) {
            Graph graph = graph_plane_gen_tri_abs(
                n,
                rng,
                &temp_arena
            );
            get(cases, i).graph = graph;
            if (graph.len != n) {
                aven_panic("graph generation failed");
            }
        }

        uint32_t p_data[] = { 1, 2 };
        uint32_t q_data[] = { 0 };
        GraphSubset p = slice_array(p_data);
        GraphSubset q = slice_array(q_data);

        __asm volatile("" ::: "memory");
        AvenTimeInst start_inst = aven_time_now();
        __asm volatile("" ::: "memory");

        for (uint32_t i = 0; i < cases.len; i += 1) {
            get(cases, i).coloring = graph_plane_p3color(
                get(cases, i).graph,
                p,
                q,
                &temp_arena
            );
        }

        __asm volatile("" ::: "memory");
        AvenTimeInst end_inst = aven_time_now();
        __asm volatile("" ::: "memory");

        int64_t elapsed_ns = aven_time_since(end_inst, start_inst);
        double ns_per_graph = (double)elapsed_ns / (double)cases.len;

        uint32_t nvalid = 0;
        for (uint32_t i = 0; i < cases.len; i += 1) {
            bool valid = graph_path_color_verify(
                get(cases, i).graph,
                get(cases, i).coloring,
                temp_arena
            );
            if (valid) {
                nvalid += 1;
            }
        }

        printf(
            "path 3-coloring %lu graph(s) with %lu vertices:\n"
            "\tvalid 3-colorings: %lu\n"
            "\ttime per graph: %fns\n"
            "\ttime per half-edge: %fns\n",
            (unsigned long)cases.len,
            (unsigned long)n,
            (unsigned long)nvalid,
            ns_per_graph,
            ns_per_graph / (double)(6 * n - 12)
        );
    }

    return 0;
}
