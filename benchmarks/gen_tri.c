#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
    #define _POSIX_C_SOURCE 200112L
#endif

#define AVEN_IMPLEMENTATION
#include <aven.h>
#include <aven/arena.h>
#include <aven/fs.h>
#include <aven/math.h>
#include <aven/graph.h>
#include <aven/graph/plane/gen.h>
#include <aven/path.h>
#include <aven/rng.h>
#include <aven/rng/pcg.h>
#include <aven/time.h>

#include <float.h>
#include <stdio.h>
#include <stdlib.h>

#define ARENA_SIZE (4096UL * 400000UL)

#define NGRAPHS 3
#define MAX_VERTICES 10000001

int main(void) {
    void *mem = malloc(ARENA_SIZE);
    if (mem == NULL) {
        fprintf(stderr, "ERROR: arena malloc failed\n");
        return 1;
    }
    AvenArena arena = aven_arena_init(mem, ARENA_SIZE);

    AvenRngPcg pcg_ctx = aven_rng_pcg_seed(0xdeadf00d, 0xbeefea11);
    AvenRng rng = aven_rng_pcg(&pcg_ctx);

    Aff2 ident;
    aff2_identity(ident);

    for (uint32_t n = 10; n < MAX_VERTICES; n *= 10) {
        uint32_t ngraphs = NGRAPHS * (MAX_VERTICES / n);
        double avg_deg_sum = 0;
        size_t overall_max_deg = 0;
        size_t overall_min_deg = MAX_VERTICES;
        size_t max_deg_sum = 0;
        size_t min_deg_sum = 0;

        AvenTimeInst start_inst = aven_time_now();

        for (uint32_t i = 0; i < ngraphs; i += 1) {
            AvenArena temp_arena = arena;
            
            size_t deg_sum = 0;
            size_t max_deg = 0;
            size_t min_deg = MAX_VERTICES;

            AvenGraphPlaneGenData data = aven_graph_plane_gen_tri_unrestricted(
                n,
                ident,
                false,
                rng,
                &temp_arena
            );
            assert(data.graph.len == n);

            for (uint32_t v = 0; v < data.graph.len; v += 1) {
                AvenGraphAdjList v_adj = get(data.graph, v);
                deg_sum += v_adj.len;
                max_deg = max(max_deg, v_adj.len);
                min_deg = min(min_deg, v_adj.len);
            }

            double avg_deg = (double)deg_sum / (double)n;
            avg_deg_sum += avg_deg;
            max_deg_sum += max_deg;
            min_deg_sum += min_deg;

            overall_max_deg = max(max_deg, overall_max_deg);
            overall_min_deg = min(min_deg, overall_min_deg);
        }

        AvenTimeInst end_inst = aven_time_now();

        int64_t elapsed_ns = aven_time_since(end_inst, start_inst);

        double ns_per_graph = (double)elapsed_ns / (double)ngraphs;

        double avg_deg = avg_deg_sum / (double)ngraphs;
        double avg_max_deg = (double)max_deg_sum / (double)ngraphs;
        double avg_min_deg = (double)min_deg_sum / (double)ngraphs;

        printf(
            "generating %lu graph(s) with %lu vertices:\n"
            "\ttime per graph: %fns\n"
            "\taverage degree: %f\n"
            "\tmin degree: %lu\n"
            "\taverage min degree: %f\n"
            "\tmax degree: %lu\n"
            "\taverage max degree: %f\n",
            (unsigned long)ngraphs,
            (unsigned long)n,
            ns_per_graph,
            avg_deg,
            (unsigned long)overall_min_deg,
            avg_min_deg,
            (unsigned long)overall_max_deg,
            avg_max_deg
        );
    }
    
    return 0;
}
