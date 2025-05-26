#ifndef TEST_BFS_H
    #define TEST_BFS_H

    #include <aven.h>
    #include <aven/arena.h>
    #include <aven/fmt.h>
    #include <aven/test.h>

    #include <graph.h>
    #include <graph/bfs.h>
    #include <graph/gen.h>

    #include <stdio.h>

    typedef struct {
        uint32_t size;
        uint32_t start;
    } TestBfsCompleteArgs;

    static AvenTestResult test_bfs_complete(
        AvenArena *emsg_arena,
        AvenArena arena,
        void *opaque_args
    ) {
        TestBfsCompleteArgs *args = opaque_args;
        Graph g = graph_gen_complete(args->size, &arena);
        GraphBfsTree tree = graph_bfs(g, args->start, &arena);

        uint32_t root_dist = 0;
        size_t in_tree_valid = 0;
        size_t parents_valid = 0;
        size_t dists_valid = 0;
        for (uint32_t v = 0; v < g.adj.len; v += 1) {
            if (!graph_bfs_tree_contains(tree, v)) {
                continue;
            }

            in_tree_valid += 1;

            if (graph_bfs_tree_parent(tree, v) == args->start) {
                parents_valid += 1;
            }

            if (v == args->start) {
                root_dist = get(tree, v).dist;
            } else {
                if (get(tree, v).dist == 1) {
                    dists_valid += 1;
                }
            }
        }

        if (in_tree_valid != g.adj.len) {
            return (AvenTestResult){
                .error = 1,
                .message = aven_fmt(
                    emsg_arena,
                    "expected all {} vertices in BFS tree, found {}",
                    aven_fmt_uint(g.adj.len),
                    aven_fmt_uint(in_tree_valid)
                ),
            };
        }

        if (root_dist != 0) {
            return (AvenTestResult){
                .error = 1,
                .message = aven_fmt(
                    emsg_arena,
                    "expected root vertex to have dist 0, found {}",
                    aven_fmt_uint(root_dist)
                ),
            };
        }

        if (dists_valid != g.adj.len - 1) {
            return (AvenTestResult){
                .error = 1,
                .message = aven_fmt(
                    emsg_arena,
                    "expected all {} non-root vertices to have dist 1, found {}",
                    aven_fmt_uint(g.adj.len - 1),
                    aven_fmt_uint(dists_valid)
                ),
            };
        }

        if (parents_valid != g.adj.len) {
            return (AvenTestResult){
                .error = 1,
                .message = aven_fmt(
                    emsg_arena,
                    "expected all {} vertices to have root as parent, found {}",
                    aven_fmt_uint(g.adj.len),
                    aven_fmt_uint(parents_valid)
                ),
            };
        }

        return (AvenTestResult){ 0 };
    }

    typedef struct {
        uint32_t width;
        uint32_t height;
        uint32_t start;
    } TestBfsGridArgs;

    static AvenTestResult test_bfs_grid(
        AvenArena *emsg_arena,
        AvenArena arena,
        void *opaque_args
    ) {
        TestBfsGridArgs *args = opaque_args;
        Graph g = graph_gen_grid(args->width, args->height, &arena);
        GraphBfsTree tree = graph_bfs(g, args->start, &arena);

        size_t in_tree_valid = 0;
        size_t parents_valid = 0;
        size_t dists_valid = 0;

        uint32_t s_x = args->start % args->width;
        uint32_t s_y = args->start / args->width;

        for (uint32_t x = 0; x < args->width; x += 1) {
            for (uint32_t y = 0; y < args->height; y += 1) {
                uint32_t v = y * args->width + x;
                if (!graph_bfs_tree_contains(tree, v)) {
                    continue;
                }

                in_tree_valid += 1;

                if (v == args->start) {
                    if (graph_bfs_tree_parent(tree, v) == v) {
                        parents_valid += 1;
                    }
                    if (get(tree, v).dist == 0) {
                        dists_valid += 1;
                    }
                } else {
                    uint32_t u = graph_bfs_tree_parent(tree, v);
                    uint32_t u_x = u % args->width;
                    uint32_t u_y = u / args->width;

                    uint32_t vu_dist = ((u_x > x) ? (u_x - x) : (x - u_x)) +
                        ((u_y > y) ? (u_y - y) : (y - u_y));

                    if (vu_dist == 1) {
                        if (get(tree, v).dist == 1 and u == args->start) {
                            parents_valid += 1;
                        } else {
                            parents_valid += 1;
                        }
                    }

                    uint32_t vs_dist = ((s_x > x) ? (s_x - x) : (x - s_x)) +
                        ((s_y > y) ? (s_y - y) : (y - s_y));

                    if (get(tree, v).dist == vs_dist) {
                        dists_valid += 1;
                    }
                }
            }
        }

        if (in_tree_valid != g.adj.len) {
            return (AvenTestResult){
                .error = 1,
                .message = aven_fmt(
                    emsg_arena,
                    "expected all {} vertices in BFS tree, found {}",
                    aven_fmt_uint(g.adj.len),
                    aven_fmt_uint(in_tree_valid)
                ),
            };
        }

        if (dists_valid != g.adj.len) {
            return (AvenTestResult){
                .error = 1,
                .message = aven_fmt(
                    emsg_arena,
                    "expected all {} vertices grid dist to root, found {}",
                    aven_fmt_uint(g.adj.len),
                    aven_fmt_uint(dists_valid)
                ),
            };
        }

        if (parents_valid != g.adj.len) {
            return (AvenTestResult){
                .error = 1,
                .message = aven_fmt(
                    emsg_arena,
                    "expected all {} vertices to have grid adjacent parent, "
                    "found {}"
                    ,
                    aven_fmt_uint(g.adj.len),
                    aven_fmt_uint(parents_valid)
                ),
            };
        }

        return (AvenTestResult){ 0 };
    }

    static void test_bfs(AvenArena arena) {
        AvenTestCase tcase_data[] = {
            {
                .desc = aven_str("K_1 start 0"),
                .args = &(TestBfsCompleteArgs){ .size = 1, .start = 0 },
                .fn = test_bfs_complete,
            },
            {
                .desc = aven_str("K_2 start 0"),
                .args = &(TestBfsCompleteArgs){ .size = 2, .start = 0 },
                .fn = test_bfs_complete,
            },
            {
                .desc = aven_str("K_2 start 1"),
                .args = &(TestBfsCompleteArgs){ .size = 3, .start = 1 },
                .fn = test_bfs_complete,
            },
            {
                .desc = aven_str("K_7 start 6"),
                .args = &(TestBfsCompleteArgs){ .size = 7, .start = 6 },
                .fn = test_bfs_complete,
            },
            {
                .desc = aven_str("2x2 Grid start 0 (0, 0)"),
                .args = &(TestBfsGridArgs){
                    .width = 2,
                    .height = 2,
                    .start = 0,
                },
                .fn = test_bfs_grid,
            },
            {
                .desc = aven_str("2x2 Grid start 3 (1,1)"),
                .args = &(TestBfsGridArgs){
                    .width = 2,
                    .height = 2,
                    .start = 3,
                },
                .fn = test_bfs_grid,
            },
            {
                .desc = aven_str("4x4 Grid start 5 (1,1)"),
                .args = &(TestBfsGridArgs){
                    .width = 4,
                    .height = 4,
                    .start = 5,
                },
                .fn = test_bfs_grid,
            },
        };
        AvenTestCaseSlice tcases = slice_array(tcase_data);

        aven_test(tcases, arena);
    }

#endif // TEST_BFS_H
