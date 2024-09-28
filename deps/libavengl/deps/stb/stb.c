#define AVEN_IMPLEMENTATION
#include <aven.h>
#include <aven/arena.h>

static inline void *aven_stb_malloc(void *ctx, size_t size) {
    return aven_arena_alloc(ctx, size, AVEN_ARENA_BIGGEST_ALIGNMENT, 1);
}

#define STBTT_malloc(x, u) aven_stb_malloc(u, x)
#define STBTT_free(x, u) ((void)0)

#define STBRP_STATIC
#define STB_RECT_PACK_IMPLEMENTATION
#include "include/stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "include/stb_truetype.h"

