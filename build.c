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
    AvenArgSlice libaven_args = libaven_build_args();
    AvenArgSlice libavengl_args = libavengl_build_args();

    AvenArgSlice args = {
        .len = 1 + common_args.len + libavengl_args.len + libaven_args.len
    };
    args.ptr = aven_arena_create_array(AvenArg, &arena, args.len);

    size_t arg_index = 0;
    get(args, arg_index) = (AvenArg){
        .name = "bench",
        .description = "Build and run benchmarks",
        .type = AVEN_ARG_TYPE_BOOL,
    };
    arg_index += 1;
    for (size_t i = 0; i < libaven_args.len; i += 1) {
        get(args, arg_index) = get(libaven_args, i);
        arg_index += 1;
    }
    for (size_t i = 0; i < libavengl_args.len; i += 1) {
        get(args, arg_index) = get(libavengl_args, i);
        arg_index += 1;
    }
    for(size_t i = 0; i < common_args.len; i += 1) {
        get(args, arg_index) = get(common_args, i);
        arg_index += 1;
    }

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

    bool opt_bench = aven_arg_get_bool(args, "bench");
    AvenBuildCommonOpts opts = aven_build_common_opts(args, &arena);
    LibAvenBuildOpts libaven_opts = libaven_build_opts(args, &arena);
    LibAvenGlBuildOpts libavengl_opts = libavengl_build_opts(args, &arena);

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

    AvenStr libaven_include_path = libaven_build_include_path(
        libaven_path,
        &arena
    );

    AvenStr include_data[3];
    List(AvenStr) include_list = list_array(include_data);
    list_push(include_list) = libaven_include_path;
    list_push(include_list) = libavengraph_build_include_path(root_path, &arena);
    if (libaven_opts.winpthreads.local) {
        list_push(include_list) = libaven_build_include_winpthreads(
            libaven_path,
            &arena
        );
    }
    
    AvenStrSlice includes = slice_list(include_list);
    AvenStrSlice macros = { 0 };
    AvenStrSlice syslibs = { 0 };

    AvenStr work_dir = aven_str("build_work");
    AvenBuildStep work_dir_step = aven_build_step_mkdir(work_dir);

    AvenStr out_dir = aven_str("build_out");
    AvenBuildStep out_dir_step = aven_build_step_mkdir(out_dir);

    Optional(AvenBuildStep) winutf8_obj_step = { 0 };
    if (libaven_opts.winutf8) {
        winutf8_obj_step.value = libaven_build_step_windres_manifest(
            &opts,
            libaven_path,
            &work_dir_step,
            &arena
        );
        winutf8_obj_step.valid = true;
    }

    Optional(AvenBuildStep) winpthreads_obj_step = { 0 };
    if (libaven_opts.winpthreads.local) {
        winpthreads_obj_step.value = libaven_build_step_winpthreads(
            &opts,
            &libaven_opts,
            libaven_path,
            &work_dir_step,
            &arena
        );
        winpthreads_obj_step.valid = true;
    }

    // Build steps for examples

    AvenStr graphics_include_data[countof(include_data) + 3];
    List(AvenStr) graphics_include_list = list_array(graphics_include_data);

    for (size_t i = 0; i < includes.len; i += 1) {
        list_push(graphics_include_list) = get(includes, i);
    }
    
    list_push(graphics_include_list) = libavengl_build_include_path(
        libavengl_path,
        &arena
    );
    list_push(graphics_include_list) = libavengl_build_include_glfw(
        libavengl_path,
        &arena
    );
    list_push(graphics_include_list) = libavengl_build_include_gles2(
        libavengl_path,
        &arena
    );

    AvenStrSlice graphics_includes = slice_list(graphics_include_list);

    AvenBuildStep stb_obj_step = libavengl_build_step_stb(
        &opts,
        &libavengl_opts,
        libaven_include_path,
        libavengl_path,
        &work_dir_step,
        &arena
    );

    Optional(AvenBuildStep) glfw_obj_step = { 0 };
    if (!libavengl_opts.no_glfw) {
        glfw_obj_step.value = libavengl_build_step_glfw(
            &opts,
            &libavengl_opts,
            libavengl_path,
            &work_dir_step,
            &arena
        );
        glfw_obj_step.valid = true;
    }
 
    AvenBuildStep bfs_obj_step = aven_build_common_step_cc_ex(
        &opts,
        graphics_includes,
        macros,
        aven_path(&arena, root_path.ptr, "examples", "bfs.c", NULL),
        &work_dir_step,
        &arena
    );

    AvenBuildStep *bfs_obj_data[5];
    List(AvenBuildStep *) bfs_obj_list = list_array(bfs_obj_data);

    list_push(bfs_obj_list) = &bfs_obj_step;
    list_push(bfs_obj_list) = &stb_obj_step;
    
    if (winutf8_obj_step.valid) {
        list_push(bfs_obj_list) = &winutf8_obj_step.value;
    }

    if (winpthreads_obj_step.valid) {
        list_push(bfs_obj_list) = &winpthreads_obj_step.value;
    }

    if (glfw_obj_step.valid) {
        list_push(bfs_obj_list) = &glfw_obj_step.value;
    }

    AvenBuildStepPtrSlice bfs_objs = slice_list(bfs_obj_list);

    AvenBuildStep bfs_exe_step = aven_build_common_step_ld_exe_ex(
        &opts,
        libavengl_opts.syslibs,
        bfs_objs,
        &out_dir_step,
        aven_str("bfs"),
        true,
        &arena
    );

    AvenBuildStep gen_tri_obj_step = aven_build_common_step_cc_ex(
        &opts,
        graphics_includes,
        macros,
        aven_path(&arena, root_path.ptr, "examples", "gen_tri.c", NULL),
        &work_dir_step,
        &arena
    );

    AvenBuildStep *gen_tri_obj_data[5];
    List(AvenBuildStep *) gen_tri_obj_list = list_array(gen_tri_obj_data);

    list_push(gen_tri_obj_list) = &gen_tri_obj_step;
    list_push(gen_tri_obj_list) = &stb_obj_step;
    
    if (winutf8_obj_step.valid) {
        list_push(gen_tri_obj_list) = &winutf8_obj_step.value;
    }

    if (winpthreads_obj_step.valid) {
        list_push(gen_tri_obj_list) = &winpthreads_obj_step.value;
    }

    if (glfw_obj_step.valid) {
        list_push(gen_tri_obj_list) = &glfw_obj_step.value;
    }

    AvenBuildStepPtrSlice gen_tri_objs = slice_list(gen_tri_obj_list);

    AvenBuildStep gen_tri_exe_step = aven_build_common_step_ld_exe_ex(
        &opts,
        libavengl_opts.syslibs,
        gen_tri_objs,
        &out_dir_step,
        aven_str("gen_tri"),
        true,
        &arena
    );

    AvenBuildStep poh_obj_step = aven_build_common_step_cc_ex(
        &opts,
        graphics_includes,
        macros,
        aven_path(&arena, root_path.ptr, "examples", "poh.c", NULL),
        &work_dir_step,
        &arena
    );

    AvenBuildStep *poh_obj_data[5];
    List(AvenBuildStep *) poh_obj_list = list_array(poh_obj_data);

    list_push(poh_obj_list) = &poh_obj_step;
    list_push(poh_obj_list) = &stb_obj_step;
    
    if (winutf8_obj_step.valid) {
        list_push(poh_obj_list) = &winutf8_obj_step.value;
    }

    if (winpthreads_obj_step.valid) {
        list_push(poh_obj_list) = &winpthreads_obj_step.value;
    }

    if (glfw_obj_step.valid) {
        list_push(poh_obj_list) = &glfw_obj_step.value;
    }

    AvenBuildStepPtrSlice poh_objs = slice_list(poh_obj_list);

    AvenBuildStep poh_exe_step = aven_build_common_step_ld_exe_ex(
        &opts,
        libavengl_opts.syslibs,
        poh_objs,
        &out_dir_step,
        aven_str("poh"),
        true,
        &arena
    );

    AvenBuildStep hartman_obj_step = aven_build_common_step_cc_ex(
        &opts,
        graphics_includes,
        macros,
        aven_path(&arena, root_path.ptr, "examples", "hartman.c", NULL),
        &work_dir_step,
        &arena
    );

    AvenBuildStep *hartman_obj_data[5];
    List(AvenBuildStep *) hartman_obj_list = list_array(hartman_obj_data);

    list_push(hartman_obj_list) = &hartman_obj_step;
    list_push(hartman_obj_list) = &stb_obj_step;
    
    if (winutf8_obj_step.valid) {
        list_push(hartman_obj_list) = &winutf8_obj_step.value;
    }

    if (winpthreads_obj_step.valid) {
        list_push(hartman_obj_list) = &winpthreads_obj_step.value;
    }

    if (glfw_obj_step.valid) {
        list_push(hartman_obj_list) = &glfw_obj_step.value;
    }

    AvenBuildStepPtrSlice hartman_objs = slice_list(hartman_obj_list);

    AvenBuildStep hartman_exe_step = aven_build_common_step_ld_exe_ex(
        &opts,
        libavengl_opts.syslibs,
        hartman_objs,
        &out_dir_step,
        aven_str("hartman"),
        true,
        &arena
    );

    AvenBuildStep root_step = aven_build_step_root();
    aven_build_step_add_dep(&root_step, &bfs_exe_step, &arena);
    // aven_build_step_add_dep(&root_step, &gen_tri_exe_step, &arena);
    // (void)gen_tri_exe_step;
    aven_build_step_add_dep(&root_step, &poh_exe_step, &arena);
    aven_build_step_add_dep(&root_step, &hartman_exe_step, &arena);
    // (void)hartman_exe_step;

    // Build steps for tests

    AvenStr test_dir = aven_str("build_test");
    AvenBuildStep test_dir_step = aven_build_step_mkdir(test_dir);

    AvenBuildStep *test_obj_data[1];
    List(AvenBuildStep *) test_obj_list = list_array(test_obj_data);

    if (winutf8_obj_step.valid) {
        list_push(test_obj_list) = &winutf8_obj_step.value;
    }

    AvenBuildStepPtrSlice test_objs = slice_list(test_obj_list);

    AvenStrSlice test_args =  { 0 };

    AvenBuildStep test_step = aven_build_common_step_cc_ld_run_exe_ex(
        &opts,
        includes,
        macros,
        syslibs,
        test_objs,
        aven_path(&arena, root_path.ptr, "test.c", NULL),
        &test_dir_step,
        false,
        test_args,
        &arena
    );

    AvenBuildStep test_root_step = aven_build_step_root();
    aven_build_step_add_dep(&test_root_step, &test_step, &arena);

    // Build steps for benchmarks

    AvenStr bench_dir = aven_str("build_bench");
    AvenBuildStep bench_dir_step = aven_build_step_mkdir(bench_dir);

    AvenBuildStep *bench_obj_data[2];
    List(AvenBuildStep *) bench_obj_list = list_array(bench_obj_data);

    if (winutf8_obj_step.valid) {
        list_push(bench_obj_list) = &winutf8_obj_step.value;
    }

    if (winpthreads_obj_step.valid) {
        list_push(bench_obj_list) = &winpthreads_obj_step.value;
    }

    AvenBuildStepPtrSlice bench_objs = slice_list(bench_obj_list);

    AvenStrSlice bench_args =  { 0 };

    AvenBuildStep bench_gen_tri_step = aven_build_common_step_cc_ld_run_exe_ex(
        &opts,
        includes,
        macros,
        syslibs,
        bench_objs,
        aven_path(&arena, root_path.ptr, "benchmarks", "gen_tri.c", NULL),
        &bench_dir_step,
        false,
        bench_args,
        &arena
    );
    AvenBuildStep gen_tri_root_step = aven_build_step_root();
    aven_build_step_add_dep(&gen_tri_root_step, &bench_gen_tri_step, &arena);

    AvenBuildStep bench_poh_bfs_step = aven_build_common_step_cc_ld_run_exe_ex(
        &opts,
        includes,
        macros,
        syslibs,
        bench_objs,
        aven_path(&arena, root_path.ptr, "benchmarks", "poh_bfs.c", NULL),
        &bench_dir_step,
        false,
        bench_args,
        &arena
    );
    AvenBuildStep poh_bfs_root_step = aven_build_step_root();
    aven_build_step_add_dep(&poh_bfs_root_step, &bench_poh_bfs_step, &arena);

    AvenBuildStep bench_poh_step = aven_build_common_step_cc_ld_run_exe_ex(
        &opts,
        includes,
        macros,
        syslibs,
        bench_objs,
        aven_path(&arena, root_path.ptr, "benchmarks", "poh.c", NULL),
        &bench_dir_step,
        false,
        bench_args,
        &arena
    );
    AvenBuildStep poh_root_step = aven_build_step_root();
    aven_build_step_add_dep(&poh_root_step, &bench_poh_step, &arena);

    AvenBuildStep bench_hartman_step = aven_build_common_step_cc_ld_run_exe_ex(
        &opts,
        includes,
        macros,
        syslibs,
        bench_objs,
        aven_path(&arena, root_path.ptr, "benchmarks", "hartman.c", NULL),
        &bench_dir_step,
        false,
        bench_args,
        &arena
    );
    AvenBuildStep hartman_root_step = aven_build_step_root();
    aven_build_step_add_dep(&hartman_root_step, &bench_hartman_step, &arena);

    AvenBuildStep bench_poh_thread_step = aven_build_common_step_cc_ld_run_exe_ex(
        &opts,
        includes,
        macros,
        syslibs,
        bench_objs,
        aven_path(&arena, root_path.ptr, "benchmarks", "poh_thread.c", NULL),
        &bench_dir_step,
        false,
        bench_args,
        &arena
    );
    AvenBuildStep poh_thread_root_step = aven_build_step_root();
    aven_build_step_add_dep(&poh_thread_root_step, &bench_poh_thread_step, &arena);

    AvenBuildStep bench_hartman_thread_step = aven_build_common_step_cc_ld_run_exe_ex(
        &opts,
        includes,
        macros,
        syslibs,
        bench_objs,
        aven_path(&arena, root_path.ptr, "benchmarks", "hartman_thread.c", NULL),
        &bench_dir_step,
        false,
        bench_args,
        &arena
    );
    AvenBuildStep hartman_thread_root_step = aven_build_step_root();
    aven_build_step_add_dep(&hartman_thread_root_step, &bench_hartman_thread_step, &arena);

    AvenBuildStep bench_bfs_step = aven_build_common_step_cc_ld_run_exe_ex(
        &opts,
        includes,
        macros,
        syslibs,
        bench_objs,
        aven_path(&arena, root_path.ptr, "benchmarks", "bfs.c", NULL),
        &bench_dir_step,
        false,
        bench_args,
        &arena
    );
    AvenBuildStep bfs_root_step = aven_build_step_root();
    aven_build_step_add_dep(&bfs_root_step, &bench_bfs_step, &arena);

    AvenBuildStep bench_root_step = aven_build_step_root();
    // aven_build_step_add_dep(&bench_root_step, &hartman_thread_root_step, &arena);
    // aven_build_step_add_dep(&bench_root_step, &hartman_root_step, &arena);
    // aven_build_step_add_dep(&bench_root_step, &poh_thread_root_step, &arena);
    // aven_build_step_add_dep(&bench_root_step, &poh_root_step, &arena);
    aven_build_step_add_dep(&bench_root_step, &poh_bfs_root_step, &arena);
    // aven_build_step_add_dep(&bench_root_step, &bfs_root_step, &arena);
    // aven_build_step_add_dep(&bench_root_step, &gen_tri_root_step, &arena);

    // Run build steps according to args

    if (opts.clean) {
        aven_build_step_clean(&root_step);
        aven_build_step_clean(&test_root_step);
        aven_build_step_clean(&bench_root_step);
    } else if (opts.test) {
        error = aven_build_step_run(&test_root_step, arena);
        if (error != 0) {
            fprintf(stderr, "TEST FAILED\n");
        }
    } else if (opt_bench) {
        error = aven_build_step_run(&bench_root_step, arena);
        if (error != 0) {
            fprintf(stderr, "BENCHMARK FAILED\n");
        }
    } else {
        error = aven_build_step_run(&root_step, arena);
        if (error != 0) {
            fprintf(stderr, "BUILD FAILED\n");
        }
    }

    return 0;
}

