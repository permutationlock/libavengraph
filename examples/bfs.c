#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
    #define _POSIX_C_SOURCE 200112L
#endif

#define AVEN_IMPLEMENTATION
#include <aven.h>
#include <aven/fs.h>
#include <aven/gl.h>
#include <aven/graph.h>
#include <aven/graph/plane.h>
#include <aven/graph/bfs.h>
#include <aven/path.h>

typedef struct {
    AvenGlShapeCtx ctx;
    AvenGlShapeGeometry geometry;
    AvenGlShapeBuffer buffer;
} GraphShapes;

typedef struct {
    GraphShapes shapes;
    AvenGraph graph;
    Optional(AvenGraphBfsCtx) bfs_ctx;
    AvenArena arena;
    AvenArena init_arena;
} AppCtx;

static AppCtx app_init(AvenGl *gl, AvenArena *arena) {
    AppCtx ctx = { 0 };
 
    ctx.graph_shapes.ctx = aven_gl_shape_ctx_init(gl);
    ctx->shapes.geometry = aven_gl_shape_geometry_init(
        128,
        192,
        &ctx->arena
    );
    ctx->shapes.buffer = aven_gl_shape_buffer_init(
        gl,
        &ctx->shapes.geometry,
        AVEN_GL_BUFFER_USAGE_DYNAMIC
    );

    void *mem = malloc(GRAPH_ARENA_SIZE);
    assert(mem != NULL);

    AvenArena arena = aven_arena_init(mem, ARENA_SIZE);
}

static void app_update(AppCtx *ctx, AvenGl *gl) {
    
}

#define GRAPH_ARENA_SIZE (4096 * 1000)
#define ARENA_SIZE (GRAPH_ARENA_SIZE + 4096 * 1000)

#if defined(_MSC_VER)
int WinMain(void) {
#else // !defined(_MSC_VER)
int main(void) {
#endif // !defined(_MSC_VER)
    aven_fs_utf8_mode();

    // could probably switch to raw page allocation, but malloc is simple and
    // we are dynamically linking the system libc anyway
    void *mem = malloc(ARENA_SIZE);
    assert(mem != NULL);

    AvenArena arena = aven_arena_init(mem, ARENA_SIZE);

    int width = 480;
    int height = 480;

    glfwInit();
    glfwSetErrorCallback(error_callback);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    // raster msaa sample count should try to match the sample count of 4 used
    // for multisampling in our shape fragment shaders and stb font textures
    glfwWindowHint(GLFW_SAMPLES, 4);

    window = glfwCreateWindow(
        (int)width,
        (int)height,
        "AvenGraph Bfs Example",
        NULL,
        NULL
    );
    if (window == NULL) {
        glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
        window = glfwCreateWindow(
            (int)width,
            (int)height,
            "AvenGl Test",
            NULL,
            NULL
        );
        if (window == NULL) {
            printf("test failed: glfwCreateWindow\n");
            return 1;
        }
    }

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(main_loop, 0, 0);
#endif // __EMSCRIPTEN__

    glfwSetKeyCallback(window, key_callback);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    gl = aven_gl_load(glfwGetProcAddress);

#if !defined(__EMSCRIPTEN__) // !defined(__EMSCRIPTEN__)
    while (!glfwWindowShouldClose(window)) {
        glfwGetFramebufferSize(window, &width, &height);

        game_info.vtable.update(&ctx, &gl, width, height);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    game_info.vtable.deinit(&ctx, &gl);

    glfwDestroyWindow(window);
    glfwTerminate();
#endif // !defined(__EMSCRIPTEN__)

    // we let the OS free arena memory (or leave it in the case of Emscripten)

    return 0;
}
