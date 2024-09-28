#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
    #define _POSIX_C_SOURCE 200112L
#endif

#include <stdlib.h>

#include "config.h"

#define AVEN_IMPLEMENTATION

#include "deps/libaven/include/aven.h"
#include "deps/libaven/include/aven/arena.h"
#include "deps/libaven/include/aven/arg.h"
#include "deps/libaven/include/aven/build.h"
#include "deps/libaven/include/aven/build/common.h"
#include "deps/libaven/include/aven/path.h"

#include "deps/libaven/build.h"
#include "deps/libavengl/build.h"

#include "build.h"
 
#define ARENA_SIZE (4096 * 2000)

int main(int argc, char **argv) {
    void *mem = malloc(ARENA_SIZE);
    assert(mem != NULL);

    AvenArena arena = aven_arena_init(mem, ARENA_SIZE);

    // Construct and parse build arguments

    AvenArgSlice common_args = aven_build_common_args();
    AvenArgSlice args = { .len = common_args.len + 1 };
    args.ptr = aven_arena_create_array(AvenArg, &arena, args.len);

    for(size_t i = 0; i < common_args.len; i += 1) {
        slice_get(args, i) = slice_get(common_args, i);
    }
    slice_get(args, common_args.len) = libaven_build_arg_windres_manifest;

    int error = aven_arg_parse(
        args,
        argv,
        argc,
        aven_build_common_overview().ptr,
        aven_build_common_usage().ptr
    );
    if (error != 0) {
        if (error != AVEN_ARG_ERROR_HELP) {
            fprintf(stderr, "ARG PARSE ERROR: %d\n", error);
            return error;
        }
        return 0;
    }

    AvenBuildCommonOpts opts = aven_build_common_opts(args, &arena);
    bool opt_winutf8 = libaven_build_opt_windres_manifest(args);

    // Build setup

    AvenStr root_path = aven_str(".");

    AvenStr libaven_path = aven_path(
        &arena,
        "deps",
        "libaven",
        NULL
    );
    AvenStr libavengl_path = aven_path(
        &arena,
        "deps",
        "libavengl",
        NULL
    );

    AvenStr include_data[] = {
        libaven_build_include_path(libaven_path, &arena),
        libavengl_build_include_path(libavengl_path, &arena),
        libavengraph_build_include_path(root_path, &arena),
    };
    AvenStrSlice includes = slice_array(include_data);
    AvenStrSlice macros = { 0 };
    AvenStrSlice syslibs = { 0 };

    AvenStr work_dir = aven_str("build_work");
    AvenBuildStep work_dir_step = aven_build_step_mkdir(work_dir);

    AvenStr out_dir = aven_str("build_out");
    AvenBuildStep out_dir_step = aven_build_step_mkdir(out_dir);

    Optional(AvenBuildStep) winutf8_obj_step = { 0 };
    if (opt_winutf8) {
        winutf8_obj_step.value = libaven_build_step_windres_manifest(
            &opts,
            libaven_path,
            &work_dir_step,
            &arena
        );
        winutf8_obj_step.valid = true;
    }

    // Build steps for examples
 
    AvenBuildStep bfs_obj_step = aven_build_common_step_cc_ex(
        &opts,
        includes,
        macros,
        aven_path(&arena, root_path.ptr, "src", "bfs.c", NULL),
        &work_dir_step,
        &arena
    );
    AvenBuildStep *bfs_obj_data[] = { &bfs_obj_step, &winutf8_obj_step.value };
    AvenBuildStepPtrSlice bfs_objs = slice_array(bfs_obj_data);

    if (!winutf8_obj_step.valid) {
        bfs_objs.len -= 1;
    }

    AvenBuildStep bfs_exe_step = aven_build_common_step_ld_exe_ex(
        &opts,
        syslibs,
        bfs_objs,
        &out_dir_step,
        aven_str("bfs"),
        true,
        &arena
    );

    AvenBuildStep root_step = aven_build_step_root();
    aven_build_step_add_dep(&root_step, &bfs_exe_step, &arena);

    // Build steps for tests

    AvenStr test_dir = aven_str("build_test");
    AvenBuildStep test_dir_step = aven_build_step_mkdir(test_dir);

    AvenBuildStep *test_linked_obj_refs[] = { &winutf8_obj_step.value };
    AvenBuildStepPtrSlice test_linked_objs = slice_array(test_linked_obj_refs);

    if (!winutf8_obj_step.valid) {
        test_linked_objs.len -= 1;
    }

    AvenStrSlice test_args =  { 0 };

    AvenBuildStep test_step = aven_build_common_step_cc_ld_run_exe_ex(
        &opts,
        includes,
        macros,
        syslibs,
        test_linked_objs,
        aven_path(&arena, root_path.ptr, "test.c", NULL),
        &test_dir_step,
        false,
        test_args,
        &arena
    );

    AvenBuildStep test_root_step = aven_build_step_root();
    aven_build_step_add_dep(&test_root_step, &test_step, &arena);

    // Run build steps according to args

    if (opts.clean) {
        aven_build_step_clean(&root_step);
        aven_build_step_clean(&test_root_step);
    } else if (opts.test) {
        error = aven_build_step_run(&test_root_step, arena);
        if (error != 0) {
            fprintf(stderr, "TEST FAILED\n");
        }
    } else {
        error = aven_build_step_run(&test_root_step, arena);
        if (error != 0) {
            fprintf(stderr, "BUILD FAILED\n");
        }
    }

    return 0;
}

