#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
    #define _POSIX_C_SOURCE 200112L
#endif

#define AVEN_IMPLEMENTATION
#include <aven.h>
#include <aven/arena.h>
#include <aven/fs.h>
#include <aven/math.h>
#include <aven/graph.h>
#include <aven/graph/plane.h>
#include <aven/graph/bfs.h>
#include <aven/graph/plane/gen.h>
#include <aven/path.h>
#include <aven/rng.h>
#include <aven/rng/pcg.h>
#include <aven/time.h>

#include <float.h>
#include <stdio.h>
#include <stdlib.h>

#define ARENA_SIZE (4096UL * 3200000UL)

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
        AvenArena temp_arena = arena;

        typedef struct {
            AvenGraph graph;
            uint32_t root;
            uint32_t target;
            AvenGraphSubset path;
        } CaseData;

        Slice(CaseData) cases = { .len = NGRAPHS * (MAX_VERTICES / n) };
        cases.ptr = aven_arena_create_array(
            CaseData,
            &temp_arena,
            cases.len
        );

        for (uint32_t i = 0; i < cases.len; i += 1) {
            AvenGraphPlaneGenData data = aven_graph_plane_gen_tri_unrestricted(
                n,
                ident,
                false,
                rng,
                &temp_arena
            );
            slice_get(cases, i).graph = data.graph;
            if (data.graph.len != n) {
                abort();
            }
            slice_get(cases, i).root = aven_rng_rand_bounded(rng, n);
            do {
                slice_get(cases, i).target = aven_rng_rand_bounded(rng, n);
            } while (slice_get(cases, i).target == slice_get(cases, i).root);
        }

        AvenTimeInst start_inst = aven_time_now();

        for (uint32_t i = 0; i < cases.len; i += 1) {
            slice_get(cases, i).path = aven_graph_bfs(
                slice_get(cases, i).graph,
                slice_get(cases, i).root,
                slice_get(cases, i).target,
                &temp_arena
            );
        }

        AvenTimeInst end_inst = aven_time_now();
        int64_t elapsed_ns = aven_time_since(end_inst, start_inst);
        double ns_per_graph = (double)elapsed_ns / (double)cases.len;

        uint32_t nvalid = 0;
        for (uint32_t i = 0; i < cases.len; i += 1) {
            AvenGraphSubset path = slice_get(cases, i).path;
            bool valid = (slice_get(path, 0) == slice_get(cases, i).target) and
                (slice_get(path, path.len - 1) == slice_get(cases, i).root);
            if (valid) {
                nvalid += 1;
            }
        }

        printf(
            "bfs on %lu graph(s) with %lu vertices:\n"
            "\tvalid paths: %lu\n"
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
