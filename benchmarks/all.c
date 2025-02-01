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
#include <aven/graph/bfs.h>
#include <aven/graph/plane/poh_bfs.h>
#include <aven/graph/plane/poh.h>
#include <aven/graph/plane/poh/thread.h>
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

#define ARENA_SIZE (4096UL * 4000000UL)

#define FULL_RUNS 10
#define NGRAPHS 10
#define MAX_VERTICES 1000001
#define START_VERTICES 1000

#define MAX_COLOR 8

#define NTHREADS 4

#define NBENCHES 11

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
        "Path 3-Color w/ N(P) (2 threads)",
        "Path 3-Color w/ N(P) (3 threads)",
        "Path 3-Color w/ N(P) (4 threads)",
        "Path 3-Choose",
        "Path 3-Choose (2 threads)",
        "Path 3-Choose (3 threads)",
        "Path 3-Choose (4 threads)",
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

    AvenThreadPool thread_pool = aven_thread_pool_init(
        NTHREADS - 1,
        NTHREADS - 1,
        &arena
    );
    aven_thread_pool_run(&thread_pool);
    
    uint32_t p_data[] = { 1, 2 };
    uint32_t q_data[] = { 0 };
    AvenGraphSubset p = slice_array(p_data);
    AvenGraphSubset q = slice_array(q_data);

    uint32_t face_data[3] = { 0, 1, 2 };
    AvenGraphSubset face = slice_array(face_data);

    Aff2 ident;
    aff2_identity(ident);

    for (size_t r = 0; r < FULL_RUNS; r += 1) {
        size_t n_count = 0;
        for (uint32_t n = START_VERTICES; n < MAX_VERTICES; n *= 10) {
            AvenArena loop_arena = arena;

            size_t bench_index = 0;

            typedef struct {
                AvenGraph graph;
                AvenGraphAug aug_graph;
                AvenGraphPlaneHartmanListProp color_lists;
                AvenGraphPropUint8 coloring;
                AvenGraphSubset path;
                uint32_t root;
                uint32_t target;
            } CaseData;

            size_t nruns = MAX_VERTICES / n;

            Slice(CaseData) cases = { .len = NGRAPHS };
            cases.ptr = aven_arena_create_array(
                CaseData,
                &loop_arena,
                cases.len
            );

            for (uint32_t i = 0; i < cases.len; i += 1) {
                AvenGraph graph = aven_graph_plane_gen_tri_abs(
                    n,
                    rng,
                    &loop_arena
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
                    &loop_arena,
                    n
                );

                for (uint32_t j = 0; j < color_lists->len; j += 1) {
                    AvenGraphPlaneHartmanList list = { .len = 3 };
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

                    get(cases, i).root = aven_rng_rand_bounded(rng, n);
                    do {
                        get(cases, i).target = aven_rng_rand_bounded(rng, n);
                    } while (get(cases, i).target == get(cases, i).root);
                }
            }
            {
                AvenArena temp_arena = loop_arena;

                __asm volatile("" ::: "memory");
                AvenTimeInst start_inst = aven_time_now();
                __asm volatile("" ::: "memory");

                for (size_t k = 0; k < nruns; k += 1) {
                    __asm volatile("" ::: "memory");
                    temp_arena = loop_arena;
                    for (uint32_t i = 0; i < cases.len; i += 1) {
                        get(cases, i).path = aven_graph_bfs(
                            get(cases, i).graph,
                            get(cases, i).root,
                            get(cases, i).target,
                            &temp_arena
                        );
                    }
                    __asm volatile("" ::: "memory");
                }

                __asm volatile("" ::: "memory");
                AvenTimeInst end_inst = aven_time_now();
                __asm volatile("" ::: "memory");

                int64_t elapsed_ns = aven_time_since(end_inst, start_inst);
                double ns_per_graph = (double)elapsed_ns / (double)(cases.len * nruns);

                uint32_t nvalid = 0;
                for (uint32_t i = 0; i < cases.len; i += 1) {
                    AvenGraphSubset path = get(cases, i).path;
                    bool valid = (get(path, 0) == get(cases, i).target) and
                        (get(path, path.len - 1) == get(cases, i).root);
                    if (valid) {
                        nvalid += 1;
                    }
                }

                if (nvalid < cases.len) {
                    aven_panic("invalid bfs path");
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

                get(get(bench_times, bench_index), n_count) += ns_per_graph;
                bench_index += 1;
            }
            {
                AvenArena temp_arena = loop_arena;
                __asm volatile("" ::: "memory");
                AvenTimeInst start_inst = aven_time_now();
                __asm volatile("" ::: "memory");

                for (size_t k = 0; k < nruns; k += 1) {
                    __asm volatile("" ::: "memory");
                    temp_arena = loop_arena;
                    for (uint32_t i = 0; i < cases.len; i += 1) {
                        get(cases, i).aug_graph = aven_graph_aug(
                            get(cases, i).graph,
                            &temp_arena
                        );
                    }
                    __asm volatile("" ::: "memory");
                }

                __asm volatile("" ::: "memory");
                AvenTimeInst end_inst = aven_time_now();
                __asm volatile("" ::: "memory");

                int64_t elapsed_ns = aven_time_since(end_inst, start_inst);
                double ns_per_graph = (double)elapsed_ns / (double)(cases.len * nruns);

                loop_arena = temp_arena;

                uint32_t nvalid = 0;
                for (uint32_t i = 0; i < cases.len; i += 1) {
                    AvenGraph graph = get(cases, i).graph;
                    AvenGraphAug aug_graph = get(cases, i).aug_graph;

                    if (graph.len != aug_graph.len) {
                        continue;
                    }

                    bool valid = true;
                    for (uint32_t v = 0; v < graph.len; v += 1) {
                        AvenGraphAdjList v_adj = get(graph, v);
                        AvenGraphAugAdjList v_aug_adj = get(aug_graph, v);

                        if (v_adj.len != v_aug_adj.len) {
                            valid = false;
                        }

                        for (uint32_t j = 0; j < v_adj.len; j += 1) {
                            uint32_t u = get(v_adj, j);
                            AvenGraphAugAdjListNode u_node = get(v_aug_adj, j);

                            if (u != u_node.vertex) {
                                valid = false;
                                break;
                            }

                            AvenGraphAugAdjList u_aug_adj = get(aug_graph, u);
                            if (v != get(u_aug_adj, u_node.back_index).vertex) {
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
                    "\tvalid augmented graphs: %lu\n"
                    "\ttime per graph: %fns\n"
                    "\ttime per half-edge: %fns\n",
                    (unsigned long)cases.len,
                    (unsigned long)n,
                    (unsigned long)nvalid,
                    ns_per_graph,
                    ns_per_graph / (double)(6 * n - 12)
                );

                get(get(bench_times, bench_index), n_count) += ns_per_graph;
                bench_index += 1;
            }
            {
                AvenArena temp_arena = loop_arena;
        
                __asm volatile("" ::: "memory");
                AvenTimeInst start_inst = aven_time_now();
                __asm volatile("" ::: "memory");

                for (size_t k = 0; k < nruns; k += 1) {
                    __asm volatile("" ::: "memory");
                    temp_arena = loop_arena;
                    for (uint32_t i = 0; i < cases.len; i += 1) {
                        get(cases, i).coloring = aven_graph_plane_poh_bfs(
                            get(cases, i).graph,
                            p,
                            q,
                            &temp_arena
                        );
                    }
                    __asm volatile("" ::: "memory");
                }

                __asm volatile("" ::: "memory");
                AvenTimeInst end_inst = aven_time_now();
                __asm volatile("" ::: "memory");

                int64_t elapsed_ns = aven_time_since(end_inst, start_inst);
                double ns_per_graph = (double)elapsed_ns / (double)(cases.len * nruns);

                uint32_t nvalid = 0;
                for (uint32_t i = 0; i < cases.len; i += 1) {
                    bool valid = aven_graph_path_color_verify(
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
                    "\tvalid 3-colorings: %lu\n"
                    "\ttime per graph: %fns\n"
                    "\ttime per half-edge: %fns\n",
                    (unsigned long)cases.len,
                    (unsigned long)n,
                    (unsigned long)nvalid,
                    ns_per_graph,
                    ns_per_graph / (double)(6 * n - 12)
                );

                get(get(bench_times, bench_index), n_count) += ns_per_graph;
                bench_index += 1;
            }
            {
                AvenArena temp_arena = loop_arena;

                __asm volatile("" ::: "memory");
                AvenTimeInst start_inst = aven_time_now();
                __asm volatile("" ::: "memory");

                for (size_t k = 0; k < nruns; k += 1) {
                    __asm volatile("" ::: "memory");
                    temp_arena = loop_arena;
                    for (uint32_t i = 0; i < cases.len; i += 1) {
                        get(cases, i).coloring = aven_graph_plane_poh(
                            get(cases, i).graph,
                            p,
                            q,
                            &temp_arena
                        );
                    }
                    __asm volatile("" ::: "memory");
                }

                __asm volatile("" ::: "memory");
                AvenTimeInst end_inst = aven_time_now();
                __asm volatile("" ::: "memory");

                int64_t elapsed_ns = aven_time_since(end_inst, start_inst);
                double ns_per_graph = (double)elapsed_ns / (double)(cases.len * nruns);

                uint32_t nvalid = 0;
                for (uint32_t i = 0; i < cases.len; i += 1) {
                    bool valid = aven_graph_path_color_verify(
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
                    "\tvalid 3-colorings: %lu\n"
                    "\ttime per graph: %fns\n"
                    "\ttime per half-edge: %fns\n",
                    (unsigned long)cases.len,
                    (unsigned long)n,
                    (unsigned long)nvalid,
                    ns_per_graph,
                    ns_per_graph / (double)(6 * n - 12)
                );

                get(get(bench_times, bench_index), n_count) += ns_per_graph;
                bench_index += 1;
            }
            for (size_t nthreads = 2; nthreads <= NTHREADS; nthreads += 1){
                AvenArena temp_arena = loop_arena;

                __asm volatile("" ::: "memory");
                AvenTimeInst start_inst = aven_time_now();
                __asm volatile("" ::: "memory");

                for (size_t k = 0; k < nruns; k += 1) {
                    __asm volatile("" ::: "memory");
                    temp_arena = loop_arena;
                    for (uint32_t i = 0; i < cases.len; i += 1) {
                        get(cases, i).coloring = aven_graph_plane_poh_thread(
                            get(cases, i).graph,
                            p,
                            q,
                            &thread_pool,
                            nthreads,
                            &temp_arena
                        );
                    }
                    __asm volatile("" ::: "memory");
                }

                __asm volatile("" ::: "memory");
                AvenTimeInst end_inst = aven_time_now();
                __asm volatile("" ::: "memory");

                int64_t elapsed_ns = aven_time_since(end_inst, start_inst);
                double ns_per_graph = (double)elapsed_ns / (double)(cases.len * nruns);

                uint32_t nvalid = 0;
                for (uint32_t i = 0; i < cases.len; i += 1) {
                    bool valid = aven_graph_path_color_verify(
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
                    "\tvalid 3-colorings: %lu\n"
                    "\ttime per graph: %fns\n"
                    "\ttime per half-edge: %fns\n",
                    (unsigned long)nthreads,
                    (unsigned long)cases.len,
                    (unsigned long)n,
                    (unsigned long)nvalid,
                    ns_per_graph,
                    ns_per_graph / (double)(6 * n - 12)
                );

                get(get(bench_times, bench_index), n_count) += ns_per_graph;
                bench_index += 1;
            }
            {
                AvenArena temp_arena = loop_arena;

                __asm volatile("" ::: "memory");
                AvenTimeInst start_inst = aven_time_now();
                __asm volatile("" ::: "memory");

                for (size_t k = 0; k < nruns; k += 1) {
                    __asm volatile("" ::: "memory");
                    temp_arena = loop_arena;
                    for (uint32_t i = 0; i < cases.len; i += 1) {
                        get(cases, i).coloring = aven_graph_plane_hartman(
                            get(cases, i).aug_graph,
                            get(cases, i).color_lists,
                            face,
                            &temp_arena
                        );
                    }
                    __asm volatile("" ::: "memory");
                }

                __asm volatile("" ::: "memory");
                AvenTimeInst end_inst = aven_time_now();
                __asm volatile("" ::: "memory");

                int64_t elapsed_ns = aven_time_since(end_inst, start_inst);
                double ns_per_graph = (double)elapsed_ns / (double)(cases.len * nruns);

                uint32_t nvalid = 0;
                for (uint32_t i = 0; i < cases.len; i += 1) {
                    bool valid = true;
    
                    AvenGraphPlaneHartmanListProp color_lists = get(
                        cases,
                        i
                    ).color_lists;
                    AvenGraphPropUint8 coloring = get(cases, i).coloring;

                    // verify coloring is a list-coloring
                    for (uint32_t v = 0; v < get(cases, i).graph.len; v += 1) {
                        uint8_t v_color = get(coloring, v);
                        AvenGraphPlaneHartmanList v_colors = get(color_lists, v);

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
                        valid = aven_graph_path_color_verify(
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
                    aven_panic("invalid 3-choosing");
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

                get(get(bench_times, bench_index), n_count) += ns_per_graph;
                bench_index += 1;
            }
            for (size_t nthreads = 2; nthreads <= NTHREADS; nthreads += 1){
                AvenArena temp_arena = loop_arena;

                __asm volatile("" ::: "memory");
                AvenTimeInst start_inst = aven_time_now();
                __asm volatile("" ::: "memory");

                for (size_t k = 0; k < nruns; k += 1) {
                    __asm volatile("" ::: "memory");
                    temp_arena = loop_arena;
                    for (uint32_t i = 0; i < cases.len; i += 1) {
                        get(cases, i).coloring = aven_graph_plane_hartman_thread(
                            get(cases, i).aug_graph,
                            get(cases, i).color_lists,
                            face,
                            &thread_pool,
                            nthreads,
                            &temp_arena
                        );
                    }
                    __asm volatile("" ::: "memory");
                }

                __asm volatile("" ::: "memory");
                AvenTimeInst end_inst = aven_time_now();
                __asm volatile("" ::: "memory");

                int64_t elapsed_ns = aven_time_since(end_inst, start_inst);
                double ns_per_graph = (double)elapsed_ns / (double)(cases.len * nruns);

                uint32_t nvalid = 0;
                for (uint32_t i = 0; i < cases.len; i += 1) {
                    bool valid = true;
    
                    AvenGraphPlaneHartmanListProp color_lists = get(
                        cases,
                        i
                    ).color_lists;
                    AvenGraphPropUint8 coloring = get(cases, i).coloring;

                    // verify coloring is a list-coloring
                    for (uint32_t v = 0; v < get(cases, i).graph.len; v += 1) {
                        uint8_t v_color = get(coloring, v);
                        AvenGraphPlaneHartmanList v_colors = get(color_lists, v);

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
                        valid = aven_graph_path_color_verify(
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
                    "\tvalid 3-choosings: %lu\n"
                    "\ttime per graph: %fns\n"
                    "\ttime per half-edge: %fns\n",
                    (unsigned long)nthreads,
                    (unsigned long)cases.len,
                    (unsigned long)n,
                    (unsigned long)nvalid,
                    ns_per_graph,
                    ns_per_graph / (double)(6 * n - 12)
                );

                get(get(bench_times, bench_index), n_count) += ns_per_graph;
                bench_index += 1;
            }

            n_count += 1;
        }
    }

    aven_thread_pool_halt_and_destroy(&thread_pool);

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
