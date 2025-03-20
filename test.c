#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
    #define _POSIX_C_SOURCE 200112L
#endif

#define AVEN_IMPLEMENTATION

#include <aven.h>
#include <aven/arena.h>
#include <aven/fs.h>
#include <aven/path.h>
#include <aven/str.h>
#include <aven/test.h>

#include <stdlib.h>

#include "test/bfs.h"
#include "test/io.h"
#include "test/plane.h"

#define ARENA_SIZE (4096 * 16)

int main(void) {
    aven_fs_utf8_mode();
    void *mem = malloc(ARENA_SIZE);
    AvenArena test_arena = aven_arena_init(mem, ARENA_SIZE);

    test_bfs(test_arena);
    test_io(test_arena);
    test_plane(test_arena);

    return 0;
}
