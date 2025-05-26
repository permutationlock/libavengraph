#ifndef TEST_DFS_H
    #define TEST_DFS_H

    #include <aven.h>
    #include <aven/arena.h>
    #include <aven/fmt.h>
    #include <aven/test.h>

    #include <graph.h>
    #include <graph/dfs.h>
    #include <graph/gen.h>

    #include <stdio.h>

    typedef struct {
        uint32_t size;
        uint32_t start;
    } TestDfsCompleteArgs;

    static AvenTestResult test_dfs_complete(
        AvenArena *emsg_arena,
        AvenArena arena,
        void *opaque_args
    ) {
        TestDfsCompleteArgs *args = opaque_args;
        Graph g = graph_gen_complete(args->size, &arena);
        GraphDfsData data = graph_dfs(g, args->start, &arena);

        size_t in_tree_valid = 0;
        for (uint32_t v = 0; v < g.adj.len; v += 1) {
            if (!graph_dfs_tree_contains(data.tree, v)) {
                continue;
            }

            in_tree_valid += 1;
        }

        if (in_tree_valid != g.adj.len) {
            return (AvenTestResult){
                .error = 1,
                .message = aven_fmt(
                    emsg_arena,
                    "expected all {} vertices in DFS tree, found {}",
                    aven_fmt_uint(g.adj.len),
                    aven_fmt_uint(in_tree_valid)
                ),
            };
        }

        if (data.numbering.len != g.adj.len) {
            return (AvenTestResult){
                .error = 1,
                .message = aven_fmt(
                    emsg_arena,
                    "expected all {} vertices to have DFS number, found {}",
                    aven_fmt_uint(g.adj.len),
                    aven_fmt_uint(in_tree_valid)
                ),
            };
        }

        uint32_t root_number = get(data.tree, args->start).number;
        if (root_number != 0) {
            return (AvenTestResult){
                .error = 1,
                .message = aven_fmt(
                    emsg_arena,
                    "expected root vertex to have dfs number 0, found {}",
                    aven_fmt_uint(root_number)
                ),
            };
        }

        size_t parents_valid = 0;
        for (uint32_t n = 0; n < data.numbering.len; n += 1) {
            uint32_t v = get(data.numbering, n);
            uint32_t p = graph_dfs_tree_parent(data.tree, v);
            uint32_t pn = get(data.tree, p).number;
            if (n == 0 and p == v and pn == n) {
                parents_valid += 1;
            } else if (p != v and pn == n - 1) {
                parents_valid += 1;
            }
        }

        if (parents_valid != g.adj.len) {
            return (AvenTestResult){
                .error = 1,
                .message = aven_fmt(
                    emsg_arena,
                    "expected all {} vertices to have parent "
                    "with prior DFS number, found {}"
                    ,
                    aven_fmt_uint(g.adj.len),
                    aven_fmt_uint(parents_valid)
                ),
            };
        }

        if (g.adj.len > 2) {
            size_t valid_lowpoints = 0;
            for (uint32_t n = 0; n < data.numbering.len; n += 1) {
                uint32_t v = get(data.numbering, n);
                if (get(data.tree, v).lowpoint == 0) {
                    valid_lowpoints += 1;
                }
            }
            if (valid_lowpoints != g.adj.len) {
                return (AvenTestResult){
                    .error = 1,
                    .message = aven_fmt(
                        emsg_arena,
                        "expected all {} vertices to have lowpoint 0, found {}",
                        aven_fmt_uint(g.adj.len),
                        aven_fmt_uint(parents_valid)
                    ),
                };
            }
        } else {
            // In K_2 we have root lowpoint 0, and other vertex lowpoint 1
            if (get(data.tree, args->start).lowpoint != 0) {
                return (AvenTestResult){
                    .error = 1,
                    .message = aven_str("expected root to have lowpoint 0"),
                };
            }
        }

        return (AvenTestResult){ 0 };
    }

    typedef struct {
        uint32_t size;
        uint32_t start;
    } TestDfsPathArgs;

    static AvenTestResult test_dfs_path(
        AvenArena *emsg_arena,
        AvenArena arena,
        void *opaque_args
    ) {
        TestDfsPathArgs *args = opaque_args;
        Graph g = graph_gen_path(args->size, &arena);
        GraphDfsData data = graph_dfs(g, args->start, &arena);

        size_t in_tree_valid = 0;
        for (uint32_t v = 0; v < g.adj.len; v += 1) {
            if (!graph_dfs_tree_contains(data.tree, v)) {
                continue;
            }

            in_tree_valid += 1;
        }

        if (in_tree_valid != g.adj.len) {
            return (AvenTestResult){
                .error = 1,
                .message = aven_fmt(
                    emsg_arena,
                    "expected all {} vertices in DFS tree, found {}",
                    aven_fmt_uint(g.adj.len),
                    aven_fmt_uint(in_tree_valid)
                ),
            };
        }

        if (data.numbering.len != g.adj.len) {
            return (AvenTestResult){
                .error = 1,
                .message = aven_fmt(
                    emsg_arena,
                    "expected all {} vertices to have DFS number, found {}",
                    aven_fmt_uint(g.adj.len),
                    aven_fmt_uint(in_tree_valid)
                ),
            };
        }

        uint32_t root_number = get(data.tree, args->start).number;
        if (root_number != 0) {
            return (AvenTestResult){
                .error = 1,
                .message = aven_fmt(
                    emsg_arena,
                    "expected root vertex to have dfs number 0, found {}",
                    aven_fmt_uint(root_number)
                ),
            };
        }

        size_t parents_valid = 0;
        for (uint32_t n = 0; n < data.numbering.len; n += 1) {
            uint32_t v = get(data.numbering, n);
            uint32_t p = graph_dfs_tree_parent(data.tree, v);
            uint32_t pn = get(data.tree, p).number;
            if (n == 0 and p == v and pn == n) {
                parents_valid += 1;
            } else if (p != v and pn < n) {
                parents_valid += 1;
            }
        }

        if (parents_valid != g.adj.len) {
            return (AvenTestResult){
                .error = 1,
                .message = aven_fmt(
                    emsg_arena,
                    "expected all {} vertices to have parent "
                    "with prior DFS number, found {}"
                    ,
                    aven_fmt_uint(g.adj.len),
                    aven_fmt_uint(parents_valid)
                ),
            };
        }

        size_t valid_lowpoints = 0;
        for (uint32_t n = 0; n < data.numbering.len; n += 1) {
            uint32_t v = get(data.numbering, n);
            if (get(data.tree, v).lowpoint == n) {
                valid_lowpoints += 1;
            }
        }

        if (valid_lowpoints != g.adj.len) {
            return (AvenTestResult){
                .error = 1,
                .message = aven_fmt(
                    emsg_arena,
                    "expected all {} vertices to have lowpoint equal to DFS "
                    "number, found {}"
                    ,
                    aven_fmt_uint(g.adj.len),
                    aven_fmt_uint(parents_valid)
                ),
            };
        }

        return (AvenTestResult){ 0 };
    }

    static void test_dfs(AvenArena arena) {
        AvenTestCase tcase_data[] = {
            {
                .desc = aven_str("K_1 start 0"),
                .args = &(TestDfsCompleteArgs){ .size = 1, .start = 0 },
                .fn = test_dfs_complete,
            },
            {
                .desc = aven_str("K_2 start 0"),
                .args = &(TestDfsCompleteArgs){ .size = 2, .start = 0 },
                .fn = test_dfs_complete,
            },
            {
                .desc = aven_str("K_3 start 1"),
                .args = &(TestDfsCompleteArgs){ .size = 3, .start = 1 },
                .fn = test_dfs_complete,
            },
            {
                .desc = aven_str("K_7 start 6"),
                .args = &(TestDfsCompleteArgs){ .size = 7, .start = 6 },
                .fn = test_dfs_complete,
            },
            {
                .desc = aven_str("P_1 start 0"),
                .args = &(TestDfsPathArgs){ .size = 1, .start = 0 },
                .fn = test_dfs_path,
            },
            {
                .desc = aven_str("P_2 start 0"),
                .args = &(TestDfsPathArgs){ .size = 2, .start = 0 },
                .fn = test_dfs_path,
            },
            {
                .desc = aven_str("P_2 start 1"),
                .args = &(TestDfsPathArgs){ .size = 2, .start = 0 },
                .fn = test_dfs_path,
            },
            {
                .desc = aven_str("P_3 start 1"),
                .args = &(TestDfsPathArgs){ .size = 3, .start = 1 },
                .fn = test_dfs_path,
            },
            {
                .desc = aven_str("P_7 start 4"),
                .args = &(TestDfsPathArgs){ .size = 7, .start = 4 },
                .fn = test_dfs_path,
            },
        };
        AvenTestCaseSlice tcases = slice_array(tcase_data);

        aven_test(tcases, arena);
    }

#endif // TEST_DFS_H
