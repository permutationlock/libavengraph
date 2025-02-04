#include <aven.h>
#include <aven/arena.h>
#include <aven/fs.h>
#include <aven/path.h>
#include <aven/str.h>
#include <aven/test.h>

#include <graph.h>
#include <graph/bfs.h>
#include <graph/gen.h>

typedef struct {
    uint32_t size;
    uint32_t start;
    uint32_t end;
} TestBfsCompleteArgs;

AvenTestResult test_bfs_complete(AvenArena arena, void *opaque_args) {
    TestBfsCompleteArgs *args = opaque_args;
    Graph g = graph_gen_complete(args->size, &arena);
    GraphSubset path = graph_bfs(g, args->start, args->end, &arena);

    size_t expected_len = (args->start == args->end) ? 1 : 2;

    if (path.len != expected_len) {
        char fmt[] = "expected path.len = %lu, found path.len = %lu";
       
        char *buffer = aven_arena_alloc(
            &arena,
            sizeof(fmt) + 4,
            1,
            1
        );

        int len = sprintf(buffer, fmt, expected_len, path.len);
        assert(len > 0);

        return (AvenTestResult){
            .error = 1,
            .message = buffer,
        };
    }

    if (get(path, 0) != args->start) {
        char fmt[] = "expected first vertex %lu, found %lu";
       
        char *buffer = aven_arena_alloc(
            &arena,
            sizeof(fmt) + 8,
            1,
            1
        );

        int len = sprintf(buffer, fmt, args->start, get(path, 0));
        assert(len > 0);

        return (AvenTestResult){
            .error = 1,
            .message = buffer,
        };
    }

    if (get(path, expected_len - 1) != args->end) {
        char fmt[] = "expected last vertex %lu, found %lu";
       
        char *buffer = aven_arena_alloc(
            &arena,
            sizeof(fmt) + 8,
            1,
            1
        );

        int len = sprintf(
            buffer,
            fmt,
            args->end,
            get(path, expected_len - 1)
        );
        assert(len > 0);

        return (AvenTestResult){
            .error = 1,
            .message = buffer,
        };
    }

    return (AvenTestResult){ 0 };
}

typedef struct {
    uint32_t width;
    uint32_t height;
    uint32_t start;
    uint32_t end;
} TestBfsGridArgs;

AvenTestResult test_bfs_grid(AvenArena arena, void *opaque_args) {
    TestBfsGridArgs *args = opaque_args;
    Graph g = graph_gen_grid(args->width, args->height, &arena);
    GraphSubset path = graph_bfs(g, args->start, args->end, &arena);

    bool match = true;

    // verify start and end vertices
    if (
        get(path, 0) != args->start or
        get(path, path.len - 1) != args->end
    ) {
        match = false;
    }

    // verify shortest path
    if (match) {
        uint32_t startx = args->start % args->width;
        uint32_t starty = args->start / args->width;
        uint32_t endx = args->end % args->width;
        uint32_t endy = args->end / args->width;

        uint32_t distx = endx - startx;
        if (startx > endx) {
            distx = startx - endx;
        }

        uint32_t disty = endy - starty;
        if (starty > endy) {
            disty = starty - endy;
        }

        if (path.len != distx + disty + 1) {
            match = false;
        }
    }
    
    // verify path
    if (match) {
        for (size_t i = 0; i < path.len - 1; i += 1) {
            uint32_t v = get(path, i);
            uint32_t u = get(path, i + 1);
            
            uint32_t xv = v % args->width;
            uint32_t yv = v / args->width;
            uint32_t xu = u % args->width;
            uint32_t yu = u / args->width;

            if (xv != xu) {
                if (yv != yu or (xv - xu) * (xv - xu) != 1) {
                    match = false;
                    break;
                }
            }

            if (yv != yu) {
                if (xv != xu or (yv - yu) * (yv - yu) != 1) {
                    match = false;
                    break;
                }
            }
        }
    }

    if (!match) {
        AvenStr str1 = aven_str("path { ");
        AvenStr str2 = aven_str("} invalid");

        size_t buffer_len = str1.len +
            str1.len +
            str2.len +
            8 * path.len +
            1;
        char *buffer = aven_arena_alloc(
            &arena,
            buffer_len,
            1,
            1
        );

        size_t len = 0;
        len += (size_t)sprintf(buffer + len, "%s", str1.ptr);
        assert(len < buffer_len);

        for (size_t i = 0; i < path.len; i += 1) {
            len += (size_t)sprintf(
                buffer + len,
                "%u, ",
                get(path, i)
            );
            assert(len < buffer_len);
        }

        len += (size_t)sprintf(buffer + len, "%s", str2.ptr);
        assert(len < buffer_len);

        return (AvenTestResult){
            .error = 1,
            .message = buffer,
        };
    }

    return (AvenTestResult){ 0 };
}

static void test_bfs(AvenArena arena) {
    AvenTestCase tcase_data[] = {
        {
            .desc = "K_1 same start and target",
            .args = &(TestBfsCompleteArgs){
                .size = 1,
                .start = 0,
                .end = 0,
            },
            .fn = test_bfs_complete,
        },
        {
            .desc = "K_2 same start and target",
            .args = &(TestBfsCompleteArgs){
                .size = 2,
                .start = 0,
                .end = 0,
            },
            .fn = test_bfs_complete,
        },
        {
            .desc = "K_2 start 0, end 1",
            .args = &(TestBfsCompleteArgs){
                .size = 2,
                .start = 0,
                .end = 1,
            },
            .fn = test_bfs_complete,
        },
        {
            .desc = "K_7 start 6, end 0",
            .args = &(TestBfsCompleteArgs){
                .size = 7,
                .start = 6,
                .end = 0,
            },
            .fn = test_bfs_complete,
        },
        {
            .desc = "2x2 Grid start 0, end 3",
            .args = &(TestBfsGridArgs){
                .width = 2,
                .height = 2,
                .start = 0,
                .end = 3,
            },
            .fn = test_bfs_grid,
        },
        {
            .desc = "2x2 Grid start 0, end 2",
            .args = &(TestBfsGridArgs){
                .width = 2,
                .height = 2,
                .start = 0,
                .end = 2,
            },
            .fn = test_bfs_grid,
        },
        {
            .desc = "4x4 Grid start 0, end 15",
            .args = &(TestBfsGridArgs){
                .width = 4,
                .height = 4,
                .start = 5,
                .end = 10,
            },
            .fn = test_bfs_grid,
        },
    };
    AvenTestCaseSlice tcases = slice_array(tcase_data);

    aven_test(tcases, __FILE__, arena);
}

