#ifndef LIBAVENGRAPH_BUILD_H
#define LIBAVENGRAPH_BUILD_H

static inline AvenStr libavengraph_build_include_path(
    AvenStr root_path,
    AvenArena *arena
) {
    return aven_path(arena, root_path.ptr, "include", NULL);
}

#endif // LIBAVENGRAPH_BUILD_H
