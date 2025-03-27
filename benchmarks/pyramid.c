#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
    #define _POSIX_C_SOURCE 200112L
#endif

#define AVEN_IMPLEMENTATION
#include <aven.h>
#include <aven/arena.h>
#include <aven/fs.h>
#include <aven/math.h>
#include <aven/path.h>
#include <aven/rng.h>
#include <aven/rng/pcg.h>
#include <aven/time.h>

#include <graph.h>
#include <graph/path_color.h>
#include <graph/bfs.h>
#include <graph/plane/p3color_bfs.h>
#include <graph/plane/p3color.h>
#include <graph/gen.h>

#include <stdio.h>
#include <stdlib.h>

#define ARENA_SIZE ((size_t)4096UL * (size_t)3000000UL)

#define NGRAPHS 1
#define FULL_RUNS 1
#define MAX_VERTICES 10000001
#define START_VERTICES 10000

#define MAX_COLOR 6

#define NTHREADS 4

#define NBENCHES 5

#ifdef __GNUC__
    #define BENCHMARK_COMPILER_BARRIER __asm volatile( "" ::: "memory" );
#else
    #define BENCHMARK_COMPILER_BARRIER
#endif

int main(void) {
    void *mem = malloc(ARENA_SIZE);
    if (mem == NULL) {
        fprintf(stderr, "ERROR: arena malloc failed\n");
        return 1;
    }
    AvenArena arena = aven_arena_init(mem, ARENA_SIZE);

    const char *bench_names[] = {
        "BFS",
        "Path 3-Color w/ BFS",
        "Path 3-Color w/ N(P)",
        "Path 3-Color w/ BFS (flipped)",
        "Path 3-Color w/ N(P) (flipped)",
    };

    if (countof(bench_names) != NBENCHES) {
        aven_panic("invalid benchmark count");
    }

    typedef Slice(double) DoubleSlice;
    Slice(DoubleSlice) bench_times = aven_arena_create_slice(
        DoubleSlice,
        &arena,
        NBENCHES
    );

    {
        size_t n_count = 0;
        for (uint32_t n = START_VERTICES; n < MAX_VERTICES; n *= 10) {
            n_count += 1;
        }

        for (size_t i = 0; i < bench_times.len; i += 1) {
            get(bench_times, i).len = n_count;
            get(bench_times, i).ptr = aven_arena_create_array(
                double,
                &arena,
                n_count
            );

            for (size_t j = 0; j < get(bench_times, i).len; j += 1) {
                get(get(bench_times, i), j) = 0.0;
            }
        }
    }

    AvenRngPcg pcg_ctx = aven_rng_pcg_seed(0x3241ef25, 0xe837910f);
    AvenRng rng = aven_rng_pcg(&pcg_ctx);

    uint32_t p_data[] = { 0 };
    uint32_t q_data[] = { 2, 1 };
    GraphSubset p = slice_array(p_data);
    GraphSubset q = slice_array(q_data);

    uint32_t p_data_flipped[] = { 1, 2 };
    uint32_t q_data_flipped[] = { 0 };
    GraphSubset p_flipped = slice_array(p_data_flipped);
    GraphSubset q_flipped = slice_array(q_data_flipped);

    Aff2 ident;
    aff2_identity(ident);

    for (size_t r = 0; r < FULL_RUNS; r += 1) {
        size_t n_count = 0;
        for (uint32_t na = START_VERTICES; na < MAX_VERTICES; na *= 10) {
            AvenArena loop_arena = arena;

            size_t bench_index = 0;

            uint32_t ka = (uint32_t)sqrtf(((float)na - 3.0f) * 2.0f);
            uint32_t n = (ka * (ka + 1)) / 2 + 3;

            typedef struct {
                Graph graph;
                GraphPropUint8 coloring;
                GraphBfsTree tree;
                uint32_t root;
            } CaseData;

            size_t nruns = 1;

            Slice(CaseData) cases = { .len = NGRAPHS * max(MAX_VERTICES / n, 1) };
            cases.ptr = aven_arena_create_array(
                CaseData,
                &loop_arena,
                cases.len
            );

            for (uint32_t i = 0; i < cases.len; i += 1) {
                Graph graph = graph_gen_pyramid(
                    ka,
                    &loop_arena
                );
                get(cases, i).graph = graph;
                if (graph.adj.len != n) {
                    aven_panic("graph generation failed");
                }

                for (uint32_t j = 0; j < graph.adj.len; j += 1) {
                    get(cases, i).root = 3;
                }
            }
            {
                AvenArena temp_arena = loop_arena;

                BENCHMARK_COMPILER_BARRIER
                AvenTimeInst start_inst = aven_time_now();
                BENCHMARK_COMPILER_BARRIER

                for (size_t k = 0; k < nruns; k += 1) {
                    BENCHMARK_COMPILER_BARRIER
                    temp_arena = loop_arena;
                    for (uint32_t i = 0; i < cases.len; i += 1) {
                        get(cases, i).tree = graph_bfs(
                            get(cases, i).graph,
                            get(cases, i).root,
                            &temp_arena
                        );
                    }
                    BENCHMARK_COMPILER_BARRIER
                }

                BENCHMARK_COMPILER_BARRIER
                AvenTimeInst end_inst = aven_time_now();
                BENCHMARK_COMPILER_BARRIER

                int64_t elapsed_ns = aven_time_since(end_inst, start_inst);
                double ns_per_graph = (double)elapsed_ns / (double)(cases.len * nruns);

                uint32_t nvalid = 0;
                for (uint32_t i = 0; i < cases.len; i += 1) {
                    GraphBfsTree tree = get(cases, i).tree;
                    bool valid = true;
                    for (uint32_t v = 0; v < tree.len; v += 1) {
                        if (!graph_bfs_tree_contains(tree, v)) {
                            valid = false;
                            break;
                        }
                        if (v == get(cases, i).root) {
                            if (get(tree, v).dist != 0) {
                                valid = false;
                                break;
                            }
                            if (graph_bfs_tree_parent(tree, v) != v) {
                                valid = false;
                                break;
                            }
                        } else {
                            uint32_t u = graph_bfs_tree_parent(tree, v);
                            if (get(tree, v).dist - get(tree, u).dist != 1) {
                                valid = false;
                                break;
                            }
                        }
                    }
                    if (valid) {
                        nvalid += 1;
                    }
                }

                if (nvalid < cases.len) {
                    aven_panic("invalid bfs path");
                }

                printf(
                    "bfs on %lu graph(s) with %lu vertices:\n"
                    "\ttime per graph: %fns\n"
                    "\ttime per half-edge: %fns\n",
                    (unsigned long)cases.len,
                    (unsigned long)n,
                    ns_per_graph,
                    ns_per_graph / (double)(6 * n - 12)
                );

                get(get(bench_times, bench_index), n_count) += ns_per_graph;
                bench_index += 1;
            }
            {
                AvenArena temp_arena = loop_arena;

                BENCHMARK_COMPILER_BARRIER
                AvenTimeInst start_inst = aven_time_now();
                BENCHMARK_COMPILER_BARRIER

                size_t ncases = max(cases.len / 10, 1);

                for (size_t k = 0; k < nruns; k += 1) {
                    BENCHMARK_COMPILER_BARRIER
                    temp_arena = loop_arena;
                    for (uint32_t i = 0; i < ncases; i += 1) {
                        get(cases, i).coloring = graph_plane_p3color_bfs(
                            get(cases, i).graph,
                            p,
                            q,
                            &temp_arena
                        );
                    }
                    BENCHMARK_COMPILER_BARRIER
                }

                BENCHMARK_COMPILER_BARRIER
                AvenTimeInst end_inst = aven_time_now();
                BENCHMARK_COMPILER_BARRIER

                int64_t elapsed_ns = aven_time_since(end_inst, start_inst);
                double ns_per_graph = (double)elapsed_ns / (double)(ncases * nruns);

                uint32_t nvalid = 0;
                for (uint32_t i = 0; i < ncases; i += 1) {
                    bool valid = graph_path_color_verify(
                        get(cases, i).graph,
                        get(cases, i).coloring,
                        temp_arena
                    );
                    if (valid) {
                        nvalid += 1;
                    }
                }

                if (nvalid < ncases) {
                    aven_panic("invalid 3-coloring (bfs)");
                }

                printf(
                    "path 3-coloring (bfs) %lu graph(s) with %lu vertices:\n"
                    "\ttime per graph: %fns\n"
                    "\ttime per half-edge: %fns\n",
                    (unsigned long)ncases,
                    (unsigned long)n,
                    ns_per_graph,
                    ns_per_graph / (double)(6 * n - 12)
                );

                get(get(bench_times, bench_index), n_count) += ns_per_graph;
                bench_index += 1;
            }
            {
                AvenArena temp_arena = loop_arena;

                BENCHMARK_COMPILER_BARRIER
                AvenTimeInst start_inst = aven_time_now();
                BENCHMARK_COMPILER_BARRIER

                for (size_t k = 0; k < nruns; k += 1) {
                    BENCHMARK_COMPILER_BARRIER
                    temp_arena = loop_arena;
                    for (uint32_t i = 0; i < cases.len; i += 1) {
                        get(cases, i).coloring = graph_plane_p3color(
                            get(cases, i).graph,
                            p,
                            q,
                            &temp_arena
                        );
                    }
                    BENCHMARK_COMPILER_BARRIER
                }

                BENCHMARK_COMPILER_BARRIER
                AvenTimeInst end_inst = aven_time_now();
                BENCHMARK_COMPILER_BARRIER

                int64_t elapsed_ns = aven_time_since(end_inst, start_inst);
                double ns_per_graph = (double)elapsed_ns / (double)(cases.len * nruns);

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

                if (nvalid < cases.len) {
                    aven_panic("invalid 3-coloring");
                }

                printf(
                    "path 3-coloring %lu graph(s) with %lu vertices:\n"
                    "\ttime per graph: %fns\n"
                    "\ttime per half-edge: %fns\n",
                    (unsigned long)cases.len,
                    (unsigned long)n,
                    ns_per_graph,
                    ns_per_graph / (double)(6 * n - 12)
                );

                get(get(bench_times, bench_index), n_count) += ns_per_graph;
                bench_index += 1;
            }
            {
                AvenArena temp_arena = loop_arena;

                BENCHMARK_COMPILER_BARRIER
                AvenTimeInst start_inst = aven_time_now();
                BENCHMARK_COMPILER_BARRIER

                size_t ncases = max(cases.len / 10, 1);

                for (size_t k = 0; k < nruns; k += 1) {
                    BENCHMARK_COMPILER_BARRIER
                    temp_arena = loop_arena;
                    for (uint32_t i = 0; i < ncases; i += 1) {
                        get(cases, i).coloring = graph_plane_p3color_bfs(
                            get(cases, i).graph,
                            p_flipped,
                            q_flipped,
                            &temp_arena
                        );
                    }
                    BENCHMARK_COMPILER_BARRIER
                }

                BENCHMARK_COMPILER_BARRIER
                AvenTimeInst end_inst = aven_time_now();
                BENCHMARK_COMPILER_BARRIER

                int64_t elapsed_ns = aven_time_since(end_inst, start_inst);
                double ns_per_graph = (double)elapsed_ns / (double)(ncases * nruns);

                uint32_t nvalid = 0;
                for (uint32_t i = 0; i < ncases; i += 1) {
                    bool valid = graph_path_color_verify(
                        get(cases, i).graph,
                        get(cases, i).coloring,
                        temp_arena
                    );
                    if (valid) {
                        nvalid += 1;
                    }
                }

                if (nvalid < ncases) {
                    aven_panic("invalid 3-coloring (bfs)");
                }

                printf(
                    "path 3-coloring (bfs) (flipped) %lu graph(s) with %lu vertices:\n"
                    "\ttime per graph: %fns\n"
                    "\ttime per half-edge: %fns\n",
                    (unsigned long)ncases,
                    (unsigned long)n,
                    ns_per_graph,
                    ns_per_graph / (double)(6 * n - 12)
                );

                get(get(bench_times, bench_index), n_count) += ns_per_graph;
                bench_index += 1;
            }
            {
                AvenArena temp_arena = loop_arena;

                BENCHMARK_COMPILER_BARRIER
                AvenTimeInst start_inst = aven_time_now();
                BENCHMARK_COMPILER_BARRIER

                for (size_t k = 0; k < nruns; k += 1) {
                    BENCHMARK_COMPILER_BARRIER
                    temp_arena = loop_arena;
                    for (uint32_t i = 0; i < cases.len; i += 1) {
                        get(cases, i).coloring = graph_plane_p3color(
                            get(cases, i).graph,
                            p_flipped,
                            q_flipped,
                            &temp_arena
                        );
                    }
                    BENCHMARK_COMPILER_BARRIER
                }

                BENCHMARK_COMPILER_BARRIER
                AvenTimeInst end_inst = aven_time_now();
                BENCHMARK_COMPILER_BARRIER

                int64_t elapsed_ns = aven_time_since(end_inst, start_inst);
                double ns_per_graph = (double)elapsed_ns / (double)(cases.len * nruns);

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

                if (nvalid < cases.len) {
                    aven_panic("invalid 3-coloring");
                }

                printf(
                    "path 3-coloring (flipped) %lu graph(s) with %lu vertices:\n"
                    "\ttime per graph: %fns\n"
                    "\ttime per half-edge: %fns\n",
                    (unsigned long)cases.len,
                    (unsigned long)n,
                    ns_per_graph,
                    ns_per_graph / (double)(6 * n - 12)
                );

                get(get(bench_times, bench_index), n_count) += ns_per_graph;
                bench_index += 1;
            }

            n_count += 1;
        }
    }

    for (size_t i = 0; i < bench_times.len; i += 1) {
        DoubleSlice i_times = get(bench_times, i);
        printf("%s: ", bench_names[i]);
        for (size_t j = 0; j < i_times.len; j += 1) {
            double avg_time = get(i_times, j) / (double)FULL_RUNS;
            printf("%f, ", avg_time);
        }
        printf("\n");
    }

    return 0;
}
