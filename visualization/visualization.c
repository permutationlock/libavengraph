#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
    #define _POSIX_C_SOURCE 200112L
#endif

#define AVEN_IMPLEMENTATION
#include <aven.h>
#include <aven/arena.h>
#include <aven/fs.h>
#include <aven/gl.h>
#include <aven/gl/window.h>
#include <aven/gl/window/impl.h>
#include <aven/path.h>

#include <stdlib.h>

#include "lib.h"

#if defined(HOT_RELOAD)
    #if defined(__ANDROID__) or defined(__EMSCRIPTEN__)
        #error "hot reloading not supported for this platform"
    #endif
    #ifndef HOT_DLL_PATH
        #error "must define HOT_DLL_PATH for hot watch build"
    #endif
    #ifndef HOT_WATCH_PATH
        #error "must define HOT_WATCH_PATH for hot watch build"
    #endif
    #include <aven/io.h>
#endif

#if defined(HOT_RELOAD)
    #include <aven/dl.h>
    #include <aven/watch.h>

    typedef struct {
        void *handle;
        AvenGlWindowVtable vtable;
    } VInfo;
    typedef enum {
        LIB_INFO_LOAD_ERROR_NONE = 0,
        LIB_INFO_LOAD_ERROR_OPEN,
        LIB_INFO_LOAD_ERROR_SYM,
    } VInfoError;
    typedef Result(VInfo, VInfoError) VInfoResult;

    static VInfoResult vinfo_load(AvenStr path, AvenArena temp_arena) {
        VInfo lib = { 0 };

        lib.handle = aven_dl_open(path, temp_arena);
        if (lib.handle == NULL) {
            return (VInfoResult){ .error = LIB_INFO_LOAD_ERROR_OPEN };
        }

        AvenGlWindowVtable *table = aven_dl_sym(
            lib.handle,
            aven_str("lib_table"),
            temp_arena
        );
        if (table == NULL) {
            return (VInfoResult){ .error = LIB_INFO_LOAD_ERROR_SYM };
        }

        lib.vtable = *table;

        return (VInfoResult){ .payload = lib };
    }

    static void vinfo_error_print(VInfoError error) {
        switch (error) {
            case LIB_INFO_LOAD_ERROR_OPEN:
                aven_io_perr("error opening dll\n");
                break;
            case LIB_INFO_LOAD_ERROR_SYM:
                aven_io_perr("error finding symbol in dll\n");
                break;
            default:
                aven_io_perr("unknown error\n");
                break;
        }
    }
#else // !defined(HOT_RELOAD)
    #include "lib/lib.c"

    typedef struct {
        AvenGlWindowVtable vtable;
    } VInfo;
#endif // !defined(HOT_RELOAD)

// App data (made global for Emscripten and window callbacks on Windows)
static LibCtx ctx;
static VInfo vinfo;
static AvenArena arena;

#ifdef HOT_RELOAD
    static AvenWatchHandle lib_watch_handle;
    static AvenStr lib_path;
    static AvenStr watch_dir_path;
    static bool lib_valid;
#endif

void init(AvenGlWindow *win) {
#ifdef HOT_RELOAD
    if (!lib_valid) {
        return;
    }
#endif
    vinfo.vtable.init(win);
}

void deinit(AvenGlWindow *win) {
#ifdef HOT_RELOAD
    if (!lib_valid) {
        return;
    }
#endif
    vinfo.vtable.deinit(win);
}

AvenGlWindowAction update(AvenGlWindow *win) {
#if defined(HOT_RELOAD)
    AvenWatchResult watch_result = aven_watch_check(lib_watch_handle, 0);
    if (watch_result.error != 0) {
        aven_io_perrf("FAILED TO WATCH: {}\n", aven_fmt_str(watch_dir_path));
        aven_panic("aven_watch_check failed");
    }
    if (watch_result.payload != 0) {
        if (vinfo.handle != NULL) {
            aven_dl_close(vinfo.handle);
            vinfo.handle = NULL;
        }
        VInfoResult info_result = vinfo_load(lib_path, arena);
        if (info_result.error != 0) {
            vinfo_error_print(info_result.error);
            lib_valid = false;
        } else {
            aven_io_print("reloading\n");
            vinfo = info_result.payload;
            vinfo.vtable.deinit(win);
            vinfo.vtable.init(win);
            lib_valid = true;
        }
    }
    if (!lib_valid) {
        return AVEN_GL_WINDOW_ACTION_NONE;
    }
#endif // defined(HOT_RELOAD)
    return vinfo.vtable.update(win);
}

static void damage(AvenGlWindow *w) {
#ifdef HOT_RELOAD
    if (!lib_valid) {
        return;
    }
#endif
    if (vinfo.vtable.damage.valid) {
        vinfo.vtable.damage.value(w);
    }
}

static void mouse_click(
    AvenGlWindow *w,
    Vec2 pos,
    AvenGlWindowMouse button,
    AvenGlWindowPress action,
    uint32_t mods
) {
#ifdef HOT_RELOAD
    if (!lib_valid) {
        return;
    }
#endif
    if (vinfo.vtable.mouse_click.valid) {
        vinfo.vtable.mouse_click.value(w, pos, button, action, mods);
    }
}

static void mouse_move(AvenGlWindow *w, Vec2 pos) {
#ifdef HOT_RELOAD
    if (!lib_valid) {
        return;
    }
#endif
    if (vinfo.vtable.mouse_move.valid) {
        vinfo.vtable.mouse_move.value(w, pos);
    }
}

static void mouse_enter(AvenGlWindow *w, bool entered) {
#ifdef HOT_RELOAD
    if (!lib_valid) {
        return;
    }
#endif
    if (vinfo.vtable.mouse_enter.valid) {
        vinfo.vtable.mouse_enter.value(w, entered);
    }
}

static void key(
    AvenGlWindow *w,
    AvenGlWindowKey key,
    uint32_t scancode,
    AvenGlWindowPress action,
    uint32_t modes
) {
#ifdef HOT_RELOAD
    if (!lib_valid) {
        return;
    }
#endif
    if (vinfo.vtable.key.valid) {
        vinfo.vtable.key.value(w, key, scancode, action, modes);
    }
}

#define ARENA_SIZE (LIB_ARENA_SIZE + 4096 * 4)

int run(void) {
    // should probably switch to raw page allocation, but malloc is cross
    // platform and we are dynamically linking the system libc anyway
    void *mem = malloc(ARENA_SIZE);
    assert(mem != NULL);

    arena = aven_arena_init(mem, ARENA_SIZE);

    ctx = lib_ctx(&arena);

#if defined(HOT_RELOAD)
    AvenStr exe_path = aven_str(".");
    {
        AvenPathResult info_result = aven_path_exe(&arena);
        if (info_result.error == 0) {
            exe_path = info_result.payload;
        }
    }
    AvenStr exe_dir_path = aven_path_containing_dir(exe_path);
    lib_path = aven_path(&arena, exe_dir_path, aven_str(HOT_DLL_PATH));
    {
        VInfoResult result = vinfo_load(lib_path, arena);
        if (result.error != 0) {
            vinfo_error_print(result.error);
            return 1;
        }
        vinfo = result.payload;
    }

    watch_dir_path = aven_path(&arena, exe_dir_path, aven_str(HOT_WATCH_PATH));
    lib_watch_handle = aven_watch_init(watch_dir_path, arena);
    if (lib_watch_handle == AVEN_WATCH_HANDLE_INVALID) {
        aven_io_perrf("FAILED TO WATCH: {}\n", aven_fmt_str(watch_dir_path));
        return 1;
    }
    lib_valid = true;
#else // !defined(HOT_RELOAD)
    vinfo.vtable = lib_table;
#endif // !defined(HOT_RELOAD)

    AvenGlWindowCode rcode = aven_gl_window_impl(
        LIB_INIT_WIDTH,
        LIB_INIT_HEIGHT,
        "Path Coloring Plane Triangulations",
        (AvenGlWindowVtable){
            .init = init,
            .deinit = deinit,
            .update = update,
            .damage = { .value = damage },
            .mouse_click = { .value = mouse_click },
            .mouse_move = { .value = mouse_move },
            .mouse_enter = { .value = mouse_enter },
            .key = { .value = key },
        },
        &ctx
    );

    return (int)rcode;
}

#if defined(_MSC_VER)
    int WinMain(void) {
        return run();
    }
#endif // !defined(_MSC_VER)

int main(void) {
    return run();
}
