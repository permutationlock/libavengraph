#ifndef LIBAVENGL_BUILD_H
#define LIBAVENGL_BUILD_H

AvenArg libavengl_build_arg_data[] = {
    {
        .name = "-glfw-ccflags",
        .description = "C compiler flags for GLFW",
        .type = AVEN_ARG_TYPE_STRING,
#if defined(LIBAVENGL_DEFAULT_GLFW_CCFLAGS)
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
            .data = { .arg_str = LIBAVENGL_DEFAULT_GLFW_CCFLAGS },
        },
#else
        .optional = true,
#endif
    },
    {
        .name = "-stb-ccflags",
        .description = "C compiler flags for STB",
        .type = AVEN_ARG_TYPE_STRING,
#if defined(LIBAVENGL_DEFAULT_STB_CCFLAGS)
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
            .data = { .arg_str = LIBAVENGL_DEFAULT_STB_CCFLAGS },
        },
#else
        .optional = true,
#endif
    },
    {
        .name = "-no-glfw",
        .description = "Don't build GLFW locally",
        .type = AVEN_ARG_TYPE_BOOL,
    },
    {
        .name = "-syslibs",
        .description = "System libraries to link",
        .type = AVEN_ARG_TYPE_STRING,
        .value = {
            .type = AVEN_ARG_TYPE_STRING,
#if defined(BUILD_DEFAULT_SYSLIBS)
            .data = { .arg_str = BUILD_DEFAULT_SYSLIBS },
#elif defined(_WIN32)
    #if defined(_MSC_VER) and !defined(__clang__)
            .data = {
                .arg_str = "kernel32.lib user32.lib gdi32.lib shell32.lib"
            },
    #else
            .data = { .arg_str = "kernel32 user32 gdi32 shell32" },
    #endif
#else
            .data = { .arg_str = "m dl" },
#endif
        },
    },
};

static inline AvenArgSlice libavengl_build_args(void) {
    AvenArgSlice args = slice_array(libavengl_build_arg_data);
    return args;
}

typedef struct {
    Optional(AvenStrSlice) ccflags;
    bool external;
} LibAvenGlBuildGLFWOpts;

typedef struct {
    Optional(AvenStrSlice) ccflags;
} LibAvenGlBuildSTBOpts;

typedef struct {
    LibAvenGlBuildGLFWOpts glfw;
    LibAvenGlBuildSTBOpts stb;
    AvenStrSlice syslibs;
    bool no_glfw;
} LibAvenGlBuildOpts;

static inline LibAvenGlBuildOpts libavengl_build_opts(
    AvenArgSlice args,
    AvenArena *arena
) {
    LibAvenGlBuildOpts opts = { 0 };

    if (aven_arg_has_arg(args, "-glfw-ccflags")) {
        opts.glfw.ccflags.valid = true;
        opts.glfw.ccflags.value = aven_str_split(
            aven_str_cstr(aven_arg_get_str(args, "-glfw-ccflags")),
            ' ',
            arena
        );
    }

    if (aven_arg_has_arg(args, "-stb-ccflags")) {
        opts.stb.ccflags.valid = true;
        opts.stb.ccflags.value = aven_str_split(
            aven_str_cstr(aven_arg_get_str(args, "-stb-ccflags")),
            ' ',
            arena
        );
    }

    opts.syslibs = aven_str_split(
        aven_str_cstr(aven_arg_get_str(args, "-syslibs")),
        ' ',
        arena
    );
    opts.no_glfw = aven_arg_get_bool(args, "-no-glfw");

    return opts;
}

static inline AvenStr libavengl_build_include_path(
    AvenStr root_path,
    AvenArena *arena
) {
    return aven_path(arena, root_path.ptr, "include", NULL);
}

static inline AvenStr libavengl_build_include_gles2(
    AvenStr root_path,
    AvenArena *arena
) {
    return aven_path(arena, root_path.ptr, "deps", "gles2", "include", NULL);
}

static inline AvenStr libavengl_build_include_glfw(
    AvenStr root_path,
    AvenArena *arena
) {
    return aven_path(arena, root_path.ptr, "deps", "glfw", "include", NULL);
}

static inline AvenStr libavengl_build_include_wayland(
    AvenStr root_path,
    AvenArena *arena
) {
    return aven_path(arena, root_path.ptr, "deps", "wayland", "include", NULL);
}

static inline AvenStr libavengl_build_include_x11(
    AvenStr root_path,
    AvenArena *arena
) {
    return aven_path(arena, root_path.ptr, "deps", "X11", "include", NULL);
}

static inline AvenStr libavengl_build_include_xkbcommon(
    AvenStr root_path,
    AvenArena *arena
) {
    return aven_path(
        arena,
        root_path.ptr,
        "deps",
        "xkbcommon",
        "include",
        NULL
    );
}

static inline AvenBuildStep libavengl_build_step_stb(
    AvenBuildCommonOpts *opts,
    LibAvenGlBuildOpts *libavengl_opts,
    AvenStr libaven_include_path,
    AvenStr root_path,
    AvenBuildStep *out_dir_step,
    AvenArena *arena
) {
    AvenBuildCommonOpts stb_opts = *opts;
    if (libavengl_opts->stb.ccflags.valid) {
        stb_opts.cc.flags = libavengl_opts->stb.ccflags.value;
    }

    AvenStr include_paths[] = {
        libaven_include_path
    };
    AvenStrSlice includes = slice_array(include_paths);

    return aven_build_common_step_cc_ex(
        &stb_opts,
        includes,
        (AvenStrSlice){ 0 },
        aven_path(arena, root_path.ptr, "deps", "stb", "stb.c", NULL),
        out_dir_step,
        arena
    );
}

static inline AvenBuildStep libavengl_build_step_glfw(
    AvenBuildCommonOpts *opts,
    LibAvenGlBuildOpts *libavengl_opts,
    AvenStr root_path,
    AvenBuildStep *out_dir_step,
    AvenArena *arena
) {
    AvenBuildCommonOpts glfw_opts = *opts;
    if (libavengl_opts->glfw.ccflags.valid) {
        glfw_opts.cc.flags = libavengl_opts->glfw.ccflags.value;
    }

    AvenStr include_paths[] = {
        libavengl_build_include_gles2(root_path, arena),
        libavengl_build_include_glfw(root_path, arena),
        libavengl_build_include_wayland(root_path, arena),
        libavengl_build_include_x11(root_path, arena),
        libavengl_build_include_xkbcommon(root_path, arena),
    };
    AvenStrSlice includes = slice_array(include_paths);

    return aven_build_common_step_cc_ex(
        &glfw_opts,
        includes,
        (AvenStrSlice){ 0 },
        aven_path(arena, root_path.ptr, "deps", "glfw", "glfw.c", NULL),
        out_dir_step,
        arena
    );
}

#endif // LIBAVENGL_BUILD_H
