#ifndef LIBAVEN_BUILD_H
#define LIBAVEN_BUILD_H

static AvenArg libaven_build_arg_windres_manifest = {
    .name = "-winutf8",
    .description = "Link a Windows resource to enable UTF8 mode",
    .type = AVEN_ARG_TYPE_INT,
    .value = {
        .type = AVEN_ARG_TYPE_INT,
#if defined(LIBAVEN_BUILD_DEFUALT_WINUTF8)
        .data = { .arg_int = LIBAVEN_BUILD_DEFUALT_WINUTF8 },
#elif defined(_WIN32)
        .data = { .arg_int = 1 },
#else
        .data = { .arg_int = 0 },
#endif
    },
};

static inline bool libaven_build_opt_windres_manifest(AvenArgSlice args) {
    return aven_arg_get_int(args, "-winutf8") != 0;
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
