#ifndef LIBAVEN_BUILD_H
#define LIBAVEN_BUILD_H

static AvenArg libaven_build_arg_data[] = {
    {
        .name = "-winutf8",
        .description = "Link a Windows resource to enable UTF8 mode",
        .type = AVEN_ARG_TYPE_BOOL,
        .value = {
            .type = AVEN_ARG_TYPE_BOOL,
    #if defined(LIBAVEN_BUILD_DEFUALT_WINUTF8)
            .data = { .arg_bool = LIBAVEN_BUILD_DEFUALT_WINUTF8 },
    #elif defined(_WIN32)
            .data = { .arg_bool = 1 },
    #else
            .data = { .arg_bool = 0 },
    #endif
        },
    },
};

static AvenArgSlice libaven_build_args(void) {
    AvenArgSlice args = slice_array(libaven_build_arg_data);
    return args;
}

typedef struct {
    bool winutf8;
} LibAvenBuildOpts;

static inline LibAvenBuildOpts libaven_build_opts(
    AvenArgSlice args,
    AvenArena *arena
) {
    (void)arena;

    LibAvenBuildOpts opts = { 0 };
    opts.winutf8 = aven_arg_get_bool(args, "-winutf8");

    return opts;
}

static inline AvenStr libaven_build_include_path(
    AvenStr root_path,
    AvenArena *arena
) {
    return aven_path(arena, root_path.ptr, "include", NULL);
}

static inline AvenBuildStep libaven_build_step(
    AvenBuildCommonOpts *opts,
    AvenStr root_path,
    AvenBuildStep *out_dir_step,
    AvenArena *arena
) {
    AvenStr include_paths[] = { libaven_build_include_path(root_path, arena) };
    AvenStrSlice includes = slice_array(include_paths);
    AvenStrSlice macros = { 0 };

    return aven_build_common_step_cc_ex(
        opts,
        includes,
        macros,
        aven_path(arena, root_path.ptr, "src", "aven.c", NULL),
        out_dir_step,
        arena
    );
}

static inline AvenBuildStep libaven_build_step_windres_manifest(
    AvenBuildCommonOpts *opts,
    AvenStr root_path,
    AvenBuildStep *out_dir_step,
    AvenArena *arena
) {
    return aven_build_common_step_windres(
        opts,
        aven_path(arena, root_path.ptr, "src", "windows", "manifest.rc", NULL),
        out_dir_step,
        arena
    );
}

#endif // LIBAVEN_BUILD_H
