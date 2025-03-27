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
#include <graph/plane/p3choose.h>
#include <graph/gen.h>

#ifdef BENCHMARK_THREADED
    #include <aven/thread/pool.h>
    #include <graph/plane/p3color/thread.h>
    #include <graph/plane/p3choose/thread.h>
#endif

#include <stdio.h>
#include <stdlib.h>

#define ARENA_SIZE ((size_t)4096UL * (size_t)800000UL)

#define FULL_RUNS 100
#define NGRAPHS 1
#define MAX_VERTICES 10000001
#define START_VERTICES 10000

#define MAX_COLOR 6

#define NTHREADS 4

#ifdef BENCHMARK_THREADED
    #define NBENCHES 11
#else
    #define NBENCHES 5
#endif

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

    const char *bench_names[NBENCHES] = {
        "BFS",
        "Augment Adjacency Lists",
        "Path 3-Color w/ BFS",
        "Path 3-Color w/ N(P)",
#ifdef BENCHMARK_THREADED
        "Path 3-Color w/ N(P) (2 threads)",
        "Path 3-Color w/ N(P) (3 threads)",
        "Path 3-Color w/ N(P) (4 threads)",
#endif
        "Path 3-Choose",
#ifdef BENCHMARK_THREADED
        "Path 3-Choose (2 threads)",
        "Path 3-Choose (3 threads)",
        "Path 3-Choose (4 threads)",
#endif
    };

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

#ifdef BENCHMARK_THREADED
    AvenThreadPool thread_pool = aven_thread_pool_init(
        NTHREADS - 1,
        NTHREADS - 1,
        &arena
    );
    aven_thread_pool_run(&thread_pool);
#endif

    uint32_t p_data[] = { 1, 2 };
    uint32_t q_data[] = { 0 };
    GraphSubset p = slice_array(p_data);
    GraphSubset q = slice_array(q_data);

    uint32_t face_data[3] = { 0, 1, 2 };
    GraphSubset face = slice_array(face_data);

    Aff2 ident;
    aff2_identity(ident);

    for (size_t r = 0; r < FULL_RUNS; r += 1) {
        size_t n_count = 0;
        for (uint32_t n = START_VERTICES; n < MAX_VERTICES; n *= 10) {
            AvenArena loop_arena = arena;

            size_t bench_index = 0;

            typedef struct {
                Graph graph;
                GraphAug aug_graph;
                GraphPlaneP3ChooseListProp color_lists;
                GraphPropUint8 coloring;
                GraphBfsTree tree;
                uint32_t root;
            } CaseData;

            size_t nruns = max(1, MAX_VERTICES / n);

            Slice(CaseData) cases = { .len = NGRAPHS };
            cases.ptr = aven_arena_create_array(
                CaseData,
                &loop_arena,
                cases.len
            );

            for (uint32_t i = 0; i < cases.len; i += 1) {
                Graph graph = graph_gen_triangulation(
                    n,
                    rng,
                    (Vec2){ 0.0833f, 0.1666f },
                    &loop_arena
                );
                get(cases, i).graph = graph;
                if (graph.adj.len != n) {
                    aven_panic("graph generation failed");
                }

                get(cases, i).root = aven_rng_rand_bounded(rng, n);

                GraphPlaneP3ChooseListProp *color_lists = &get(
                    cases,
                    i
                ).color_lists;

                color_lists->len = n;
                color_lists->ptr = aven_arena_create_array(
                    GraphPlaneP3ChooseList,
                    &loop_arena,
                    n
                );

                for (uint32_t j = 0; j < color_lists->len; j += 1) {
                    GraphPlaneP3ChooseList list = { .len = 3 };
                    get(list, 0) = (uint8_t)(
                        1 + aven_rng_rand_bounded(rng, MAX_COLOR)
                    );
                    get(list, 1) = (uint8_t)(
                        1 + aven_rng_rand_bounded(rng, MAX_COLOR)
                    );
                    get(list, 2) = (uint8_t)(
                        1 + aven_rng_rand_bounded(rng, MAX_COLOR)
                    );

                    while (get(list, 1) == get(list, 0)) {
                        get(list, 1) = (uint8_t)(
                            1 + aven_rng_rand_bounded(rng, MAX_COLOR)
                        );
                    }
                    while (
                        get(list, 2) == get(list, 1) or
                        get(list, 2) == get(list, 0)
                    ) {
                        get(list, 2) = (uint8_t)(
                            1 + aven_rng_rand_bounded(rng, MAX_COLOR)
                        );
                    }

                    get(*color_lists, j) = list;
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
                    aven_panic("invalid bfs tree");
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

                for (size_t k = 0; k < nruns; k += 1) {
                    BENCHMARK_COMPILER_BARRIER
                    temp_arena = loop_arena;
                    for (uint32_t i = 0; i < cases.len; i += 1) {
                        get(cases, i).aug_graph = graph_aug(
                            get(cases, i).graph,
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

                loop_arena = temp_arena;

                uint32_t nvalid = 0;
                for (uint32_t i = 0; i < cases.len; i += 1) {
                    Graph graph = get(cases, i).graph;
                    GraphAug aug_graph = get(cases, i).aug_graph;

                    if (graph.adj.len != aug_graph.adj.len) {
                        continue;
                    }

                    bool valid = true;
                    for (uint32_t v = 0; v < graph.adj.len; v += 1) {
                        GraphAdj v_adj = get(graph.adj, v);
                        GraphAdj v_aug_adj = get(aug_graph.adj, v);

                        if (v_adj.len != v_aug_adj.len) {
                            valid = false;
                        }

                        for (uint32_t j = 0; j < v_adj.len; j += 1) {
                            uint32_t u = graph_nb(graph.nb, v_adj, j);
                            GraphAugNb u_node = graph_aug_nb(
                                aug_graph.nb,
                                v_aug_adj,
                                j
                            );

                            if (u != u_node.vertex) {
                                valid = false;
                                break;
                            }

                            GraphAdj u_aug_adj = get(aug_graph.adj, u);
                            uint32_t w = graph_aug_nb(
                                aug_graph.nb,
                                u_aug_adj,
                                u_node.back_index
                            ).vertex;
                            if (v != w) {
                                valid = false;
                                break;
                            }
                        }

                        if (!valid) {
                            break;
                        }
                    }

                    if (valid) {
                        nvalid += 1;
                    }
                }

                if (nvalid < cases.len) {
                    aven_panic("invalid augmentation");
                }

                printf(
                    "augmenting %lu graph(s) with %lu vertices:\n"
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

                size_t bfs_nruns = max(nruns / 10, 1);

                BENCHMARK_COMPILER_BARRIER
                AvenTimeInst start_inst = aven_time_now();
                BENCHMARK_COMPILER_BARRIER

                for (size_t k = 0; k < bfs_nruns; k += 1) {
                    BENCHMARK_COMPILER_BARRIER
                    temp_arena = loop_arena;
                    for (uint32_t i = 0; i < cases.len; i += 1) {
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
                double ns_per_graph = (double)elapsed_ns /
                    (double)(cases.len * bfs_nruns);

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
                    aven_panic("invalid 3-coloring (bfs)");
                }

                printf(
                    "path 3-coloring (bfs) %lu graph(s) with %lu vertices:\n"
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
#ifdef BENCHMARK_THREADED
            for (size_t nthreads = 2; nthreads <= NTHREADS; nthreads += 1){
                AvenArena temp_arena = loop_arena;

                BENCHMARK_COMPILER_BARRIER
                AvenTimeInst start_inst = aven_time_now();
                BENCHMARK_COMPILER_BARRIER

                for (size_t k = 0; k < nruns; k += 1) {
                    BENCHMARK_COMPILER_BARRIER
                    temp_arena = loop_arena;
                    for (uint32_t i = 0; i < cases.len; i += 1) {
                        get(cases, i).coloring = graph_plane_p3color_thread(
                            get(cases, i).graph,
                            p,
                            q,
                            &thread_pool,
                            nthreads,
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
                    aven_panic("invalid 3-coloring (threaded)");
                }

                printf(
                    "path 3-coloring (%lu threads) %lu graph(s) with %lu vertices:\n"
                    "\ttime per graph: %fns\n"
                    "\ttime per half-edge: %fns\n",
                    (unsigned long)nthreads,
                    (unsigned long)cases.len,
                    (unsigned long)n,
                    ns_per_graph,
                    ns_per_graph / (double)(6 * n - 12)
                );

                get(get(bench_times, bench_index), n_count) += ns_per_graph;
                bench_index += 1;
            }
#endif
            {
                AvenArena temp_arena = loop_arena;

                BENCHMARK_COMPILER_BARRIER
                AvenTimeInst start_inst = aven_time_now();
                BENCHMARK_COMPILER_BARRIER

                for (size_t k = 0; k < nruns; k += 1) {
                    BENCHMARK_COMPILER_BARRIER
                    temp_arena = loop_arena;
                    for (uint32_t i = 0; i < cases.len; i += 1) {
                        get(cases, i).coloring = graph_plane_p3choose(
                            get(cases, i).aug_graph,
                            get(cases, i).color_lists,
                            face,
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
                    bool valid = true;

                    Graph graph = get(cases, i).graph;

                    GraphPlaneP3ChooseListProp color_lists = get(
                        cases,
                        i
                    ).color_lists;
                    GraphPropUint8 coloring = get(cases, i).coloring;

                    // verify coloring is a list-coloring
                    for (uint32_t v = 0; v < graph.adj.len; v += 1) {
                        uint8_t v_color = get(coloring, v);
                        GraphPlaneP3ChooseList v_colors = get(color_lists, v);

                        bool found = false;
                        for (uint32_t j = 0; j < v_colors.len; j += 1) {
                            if (get(v_colors, j) == v_color) {
                                found = true;
                            }
                        }

                        if (!found) {
                            valid = false;
                            break;
                        }
                    }

                    // verify coloring is a path coloring
                    if (valid) {
                        valid = graph_path_color_verify(
                            graph,
                            coloring,
                            temp_arena
                        );
                    }

                    if (valid) {
                        nvalid += 1;
                    }
                }

                if (nvalid < cases.len) {
                    aven_panic("invalid 3-choosing");
                }

                printf(
                    "path 3-choosing %lu graph(s) with %lu vertices:\n"
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
#ifdef BENCHMARK_THREADED
            for (size_t nthreads = 2; nthreads <= NTHREADS; nthreads += 1){
                AvenArena temp_arena = loop_arena;

                BENCHMARK_COMPILER_BARRIER
                AvenTimeInst start_inst = aven_time_now();
                BENCHMARK_COMPILER_BARRIER

                for (size_t k = 0; k < nruns; k += 1) {
                    BENCHMARK_COMPILER_BARRIER
                    temp_arena = loop_arena;
                    for (uint32_t i = 0; i < cases.len; i += 1) {
                        get(cases, i).coloring = graph_plane_p3choose_thread(
                            get(cases, i).aug_graph,
                            get(cases, i).color_lists,
                            face,
                            &thread_pool,
                            nthreads,
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
                    bool valid = true;

                    GraphPlaneP3ChooseListProp color_lists = get(
                        cases,
                        i
                    ).color_lists;
                    GraphPropUint8 coloring = get(cases, i).coloring;

                    // verify coloring is a list-coloring
                    for (uint32_t v = 0; v < get(cases, i).graph.adj.len; v += 1) {
                        uint8_t v_color = get(coloring, v);
                        GraphPlaneP3ChooseList v_colors = get(color_lists, v);

                        bool found = false;
                        for (uint32_t j = 0; j < v_colors.len; j += 1) {
                            if (get(v_colors, j) == v_color) {
                                found = true;
                            }
                        }

                        if (!found) {
                            valid = false;
                            break;
                        }
                    }

                    // verify coloring is a path coloring
                    if (valid) {
                        valid = graph_path_color_verify(
                            get(cases, i).graph,
                            get(cases, i).coloring,
                            temp_arena
                        );
                    }

                    if (valid) {
                        nvalid += 1;
                    }
                }

                if (nvalid < cases.len) {
                    aven_panic("invalid 3-choosing (threaded)");
                }

                printf(
                    "path 3-choosing (%lu threads) %lu graph(s) with %lu vertices:\n"
                    "\ttime per graph: %fns\n"
                    "\ttime per half-edge: %fns\n",
                    (unsigned long)nthreads,
                    (unsigned long)cases.len,
                    (unsigned long)n,
                    ns_per_graph,
                    ns_per_graph / (double)(6 * n - 12)
                );

                get(get(bench_times, bench_index), n_count) += ns_per_graph;
                bench_index += 1;
            }
#endif

            n_count += 1;
        }
    }

#ifdef BENCHMARK_THREADED
    aven_thread_pool_halt_and_destroy(&thread_pool);
#endif

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
