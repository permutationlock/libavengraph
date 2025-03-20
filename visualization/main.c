#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
    #define _POSIX_C_SOURCE 200112L
#endif

#define AVEN_IMPLEMENTATION
#include <aven.h>
#include <aven/fs.h>
#include <aven/gl.h>
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

    static void vinfo_error_print(int error) {
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

static void error_callback(int error, const char* description) {
    (void)error;
    fprintf(stderr, "GLFW Error: %s\n", description);
}

// App data (made global for Emscripten)
static GLFWwindow *window;
static AvenGl gl;
static GameCtx ctx;
static VInfo vinfo;
static AvenArena arena;

void main_loop_update(void) {
    int width;
    int height;
    glfwGetFramebufferSize(window, &width, &height);

    if (vinfo.vtable.update(&ctx, &gl, width, height, arena)) {
        glfwSwapBuffers(window);
    } else {
        aven_time_sleep_ms(AVEN_TIME_MSEC_PER_SEC / 60);
    }
}

void main_loop(void) {
#if defined(HOT_RELOAD)
    AvenWatchResult watch_result = aven_watch_check(game_watch_handle, 0);
    if (watch_result.error != 0) {
        fprintf(stderr, "FAILED TO WATCH: %s\n", watch_dir_path.ptr);
        return 1;
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
            vinfo.vtable.reload(&ctx, &gl);
            game_valid = true;
        }
    }
    if (!game_valid) {
        continue;
    }
#endif // defined(HOT_RELOAD)
    main_loop_update();
    glfwPollEvents();
}

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
    GLFWwindow* w,
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

static void click_callback(
    GLFWwindow* w,
    int button,
    int action,
    int mods
) {
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

static void cursor_callback(
    GLFWwindow *w,
    double xpos,
    double ypos
) {
    (void)w;
    vinfo.vtable.mouse_move(&ctx, (Vec2){ (float)xpos, (float)ypos });
}

#if defined(__EMSCRIPTEN__)
    #include <emscripten.h>

    #ifdef HOT_RELOAD
        #error "hot reloading dll incompatible with emcc"
    #endif

    void on_resize(int width, int height) {
        glfwSetWindowSize(window, width, height);
        on_damage(window);

    }
#endif // defined(__EMSCRIPTEN__)

#define ARENA_SIZE (GAME_LEVEL_ARENA_SIZE + GAME_GL_ARENA_SIZE + 4096 * 4)

#if defined(_MSC_VER)
int WinMain(void) {
#else // !defined(_MSC_VER)
int main(void) {
#endif // !defined(_MSC_VER)
    aven_fs_utf8_mode();

    // should probably switch to raw page allocation, but malloc is cross
    // platform and we are dynamically linking the system libc anyway
    void *mem = malloc(ARENA_SIZE);
    assert(mem != NULL);

    arena = aven_arena_init(mem, ARENA_SIZE);

    int width = GAME_INIT_WIDTH;
    int height = GAME_INIT_HEIGHT;

    glfwInit();
    glfwSetErrorCallback(error_callback);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    glfwWindowHint(GLFW_SAMPLES, 16);

    window = glfwCreateWindow(
        (int)width,
        (int)height,
        "Path Coloring Plane Triangulations",
        NULL,
        NULL
    );
    if (window == NULL) {
        glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
        window = glfwCreateWindow(
            (int)width,
            (int)height,
            "Path Coloring Plane Triangulations",
            NULL,
            NULL
        );
        if (window == NULL) {
            printf("test failed: glfwCreateWindow\n");
            return 1;
        }
    }

    glfwSetWindowRefreshCallback(window, on_damage);
    glfwSetWindowPosCallback(window, on_move);

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 0, 0);
#endif // __EMSCRIPTEN__

    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, cursor_callback);
    glfwSetMouseButtonCallback(window, click_callback);
    glfwMakeContextCurrent(window);
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
    AvenStr game_dll_path = aven_path(
        &arena,
        exe_dir_path,
        aven_str(HOT_DLL_PATH)
    );
    {
        VInfoResult result = vinfo_load(game_dll_path, arena);
        if (result.error != 0) {
            vinfo_error_print(result.error);
            return 1;
        }
        vinfo = result.payload;
    }

    AvenStr watch_dir_path = aven_path(
        &arena,
        exe_dir_path,
        aven_str(HOT_WATCH_PATH)
    );
    AvenWatchHandle game_watch_handle = aven_watch_init(watch_dir_path, arena);
    if (game_watch_handle == AVEN_WATCH_HANDLE_INVALID) {
        fprintf(
            stderr,
            "FAILED TO WATCH: %s\n",
            aven_str_to_cstr(watch_dir_path, &arena)
        );
        return 1;
    }
    bool game_valid = true;
#else // !defined(HOT_RELOAD)
    vinfo.vtable = game_table;
#endif // !defined(HOT_RELOAD)

    gl = aven_gl_load(glfwGetProcAddress);
    ctx = vinfo.vtable.init(&gl, &arena);

#if !defined(__EMSCRIPTEN__) // !defined(__EMSCRIPTEN__)
    while (!glfwWindowShouldClose(window)) {
        main_loop();
    }

    vinfo.vtable.deinit(&ctx, &gl);

    glfwDestroyWindow(window);
    glfwTerminate();
#endif // !defined(__EMSCRIPTEN__)

    // we let the OS free arena memory (or leave it in the case of Emscripten)

    return 0;
}
