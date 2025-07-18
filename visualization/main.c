#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
    #define _POSIX_C_SOURCE 200112L
#endif

#define AVEN_IMPLEMENTATION
#include <aven.h>
#include <aven/arena.h>
#include <aven/fs.h>
#include <aven/gl.h>
#include <aven/gl/window.h>
#include <aven/path.h>

#include <stdio.h>
#include <stdlib.h>

#include <GLFW/glfw3.h>

#include "game.h"

#if defined(HOT_RELOAD)
    #ifndef HOT_DLL_PATH
        #error "must define HOT_DLL_PATH for hot watch build"
    #endif
    #ifndef HOT_WATCH_PATH
        #error "must define HOT_WATCH_PATH for hot watch build"
    #endif
#endif

#if defined(HOT_RELOAD)
    #include <aven/dl.h>
    #include <aven/watch.h>

    typedef struct {
        void *handle;
        GameTable vtable;
    } VInfo;
    typedef enum {
        GAME_INFO_LOAD_ERROR_NONE = 0,
        GAME_INFO_LOAD_ERROR_OPEN,
        GAME_INFO_LOAD_ERROR_SYM,
    } VInfoError;
    typedef Result(VInfo, VInfoError) VInfoResult;

    static VInfoResult vinfo_load(AvenStr path, AvenArena temp_arena) {
        VInfo game_dll = { 0 };

        game_dll.handle = aven_dl_open(path, temp_arena);
        if (game_dll.handle == NULL) {
            return (VInfoResult){ .error = GAME_INFO_LOAD_ERROR_OPEN };
        }

        GameTable *table = aven_dl_sym(
            game_dll.handle,
            aven_str("game_table"),
            temp_arena
        );
        if (table == NULL) {
            return (VInfoResult){ .error = GAME_INFO_LOAD_ERROR_SYM };
        }

        game_dll.vtable = *table;

        return (VInfoResult){ .payload = game_dll };
    }

    static void vinfo_error_print(VInfoError error) {
        switch (error) {
            case GAME_INFO_LOAD_ERROR_OPEN:
                fprintf(stderr, "error opening dll\n");
                break;
            case GAME_INFO_LOAD_ERROR_SYM:
                fprintf(stderr, "error finding symbol in dll\n");
                break;
            default:
                fprintf(stderr, "unknown error\n");
                break;
        }
    }
#else // !defined(HOT_RELOAD)
    #include "game/game.c"

    typedef struct {
        GameTable vtable;
    } VInfo;
#endif // !defined(HOT_RELOAD)

// App data (made global for Emscripten and GLFW callbacks on Windows)
static AvenGlWindow win;
static GameCtx ctx;
static VInfo vinfo;
static AvenArena arena;

#ifdef __ANDROID__
    #ifdef HOT_RELOAD
        #error "hot reloading dll incompatible with android"
    #endif
    bool minimized;

    void on_android_pause_resume(GLFWwindow *window, int iconified) {
        if (iconified) {
            vinfo.vtable.unload(&ctx, &win.gl);
            minimized = true;
        } else {
            win.gl = aven_gl_load(glfwGetProcAddress, win.gl.es);
            vinfo.vtable.load(&ctx, &win.gl);
            minimized = false;
        }
    }
#endif

#ifdef HOT_RELOAD
    static AvenWatchHandle game_watch_handle;
    static AvenStr game_dll_path;
    static AvenStr watch_dir_path;
    static bool game_valid;
#endif

void main_loop_update(void) {
    int width;
    int height;
    glfwGetFramebufferSize(win.window, &width, &height);

    if (vinfo.vtable.update(&ctx, &win.gl, width, height, arena)) {
        glfwSwapBuffers(win.window);
    } else {
        aven_time_sleep_ms(AVEN_TIME_MSEC_PER_SEC / 60);
    }
}

void main_loop(void) {
#if defined(HOT_RELOAD)
    AvenWatchResult watch_result = aven_watch_check(game_watch_handle, 0);
    if (watch_result.error != 0) {
        fprintf(stderr, "FAILED TO WATCH: %s\n", watch_dir_path.ptr);
        aven_panic("aven_watch_check failed");
    }
    if (watch_result.payload != 0) {
        if (vinfo.handle != NULL) {
            aven_dl_close(vinfo.handle);
            vinfo.handle = NULL;
        }
        VInfoResult info_result = vinfo_load(game_dll_path, arena);
        if (info_result.error != 0) {
            vinfo_error_print(info_result.error);
            game_valid = false;
        } else {
            printf("reloading\n");
            vinfo = info_result.payload;
            vinfo.vtable.unload(&ctx, &win.gl);
            vinfo.vtable.load(&ctx, &win.gl);
            game_valid = true;
        }
    }
    if (!game_valid) {
        return;
    }
#endif // defined(HOT_RELOAD)
    main_loop_update();
    glfwPollEvents();
#ifdef __ANDROID__
    while (minimized) {
        glfwWaitEvents();
    }
#endif
}

#ifdef _WIN32
    void *glfwGetWin32Window(void *window);
    typedef void (*TimerCallbackFn)(
        void *,
        unsigned int,
        unsigned int,
        uint32_t
    );
    AVEN_WIN32_FN(int) SetTimer(
        void *hWnd,
        unsigned int id,
        unsigned int tstep,
        TimerCallbackFn callback
    );

    static void on_win_timestep(
        void *p1,
        unsigned int p2,
        unsigned int p3,
        uint32_t p4
    ) {
        (void)p1;
        (void)p2;
        (void)p3;
        (void)p4;
        main_loop_update();
    }
#endif

static void on_damage(GLFWwindow *w) {
    (void)w;
    vinfo.vtable.damage(&ctx);
#ifdef _WIN32
    main_loop_update();
#endif
}

static void on_move(GLFWwindow *w, int x, int y) {
    (void)x;
    (void)y;
#ifdef _WIN32
    on_damage(w);
#else
    (void)w;
#endif
}

static void key_callback(
    GLFWwindow *w,
    int key,
    int scancode,
    int action,
    int mods
) {
    (void)scancode;
    (void)mods;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(w, GLFW_TRUE);
    }
}

static void click_callback(GLFWwindow *w, int button, int action, int mods) {
    (void)mods;
    double xpos;
    double ypos;
    glfwGetCursorPos(w, &xpos, &ypos);
    vinfo.vtable.mouse_move(&ctx, (Vec2){ (float)xpos, (float)ypos });
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            vinfo.vtable.mouse_click(&ctx, AVEN_GL_UI_MOUSE_EVENT_DOWN);
        }
        if (action == GLFW_RELEASE) {
            vinfo.vtable.mouse_click(&ctx, AVEN_GL_UI_MOUSE_EVENT_UP);
        }
    }
}

static void cursor_callback(GLFWwindow *w, double xpos, double ypos) {
    (void)w;
    vinfo.vtable.mouse_move(&ctx, (Vec2){ (float)xpos, (float)ypos });
}

#if defined(__EMSCRIPTEN__)
    #include <emscripten.h>

    #ifdef HOT_RELOAD
        #error "hot reloading dll incompatible with emcc"
    #endif

    void on_resize(int width, int height) {
        glfwSetWindowSize(win.window, width, height);
        on_damage(win.window);
    }
#endif // defined(__EMSCRIPTEN__)

#define ARENA_SIZE (GAME_LEVEL_ARENA_SIZE + GAME_GL_ARENA_SIZE + 4096 * 4)

int run(void) {
    aven_fs_utf8_mode();

    // should probably switch to raw page allocation, but malloc is cross
    // platform and we are dynamically linking the system libc anyway
    void *mem = malloc(ARENA_SIZE);
    assert(mem != NULL);

    arena = aven_arena_init(mem, ARENA_SIZE);

    int width = GAME_INIT_WIDTH;
    int height = GAME_INIT_HEIGHT;

    win = aven_gl_window(width, height, "Path Coloring Plane Triangulations");

    glfwSetWindowRefreshCallback(win.window, on_damage);
    glfwSetWindowPosCallback(win.window, on_move);

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 0, 0);
#endif // __EMSCRIPTEN__

#ifdef __ANDROID__
    glfwSetWindowIconifyCallback(win.window, on_android_pause_resume);
#endif

    glfwSetKeyCallback(win.window, key_callback);
    glfwSetCursorPosCallback(win.window, cursor_callback);
    glfwSetMouseButtonCallback(win.window, click_callback);
    glfwMakeContextCurrent(win.window);
    glfwSwapInterval(1);

#if defined(HOT_RELOAD)
    AvenStr exe_path = aven_str(".");
    {
        AvenPathResult info_result = aven_path_exe(&arena);
        if (info_result.error == 0) {
            exe_path = info_result.payload;
        }
    }
    AvenStr exe_dir_path = aven_path_containing_dir(exe_path);
    game_dll_path = aven_path(&arena, exe_dir_path, aven_str(HOT_DLL_PATH));
    {
        VInfoResult result = vinfo_load(game_dll_path, arena);
        if (result.error != 0) {
            vinfo_error_print(result.error);
            return 1;
        }
        vinfo = result.payload;
    }

    watch_dir_path = aven_path(&arena, exe_dir_path, aven_str(HOT_WATCH_PATH));
    game_watch_handle = aven_watch_init(watch_dir_path, arena);
    if (game_watch_handle == AVEN_WATCH_HANDLE_INVALID) {
        fprintf(
            stderr,
            "FAILED TO WATCH: %s\n",
            aven_str_to_cstr(watch_dir_path, &arena)
        );
        return 1;
    }
    game_valid = true;
#else // !defined(HOT_RELOAD)
    vinfo.vtable = game_table;
#endif // !defined(HOT_RELOAD)

    ctx = vinfo.vtable.init(&win.gl, &arena);

#ifdef _WIN32
    int success = SetTimer(
        glfwGetWin32Window(win.window),
        1,
        AVEN_TIME_MSEC_PER_SEC / 60,
        on_win_timestep
    );
    if (!success) {
        aven_panic("failed to set window timer");
    }
#endif // defined(_WIN32)

#if !defined(__EMSCRIPTEN__) // !defined(__EMSCRIPTEN__)
    while (!glfwWindowShouldClose(win.window)) {
        main_loop();
    }

    vinfo.vtable.deinit(&ctx, &win.gl);

    glfwDestroyWindow(win.window);
    glfwTerminate();
#endif // !defined(__EMSCRIPTEN__)

    // we let the OS free arena memory (or leave it in the case of Emscripten)

    return 0;
}

#if defined(_MSC_VER)
    int WinMain(void) {
        return run();
    }
#endif // !defined(_MSC_VER)

int main(void) {
    return run();
}
