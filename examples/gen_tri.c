#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
    #define _POSIX_C_SOURCE 200112L
#endif

#define AVEN_IMPLEMENTATION
#include <aven.h>
#include <aven/fs.h>
#include <aven/gl.h>
#include <aven/gl/shape.h>
#include <aven/graph.h>
#include <aven/graph/plane.h>
#include <aven/graph/plane/gen.h>
#include <aven/graph/plane/geometry.h>
#include <aven/path.h>
#include <aven/rng.h>
#include <aven/rng/pcg.h>
#include <aven/time.h>

#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>

#define ARENA_PAGE_SIZE 4096
#define GRAPH_ARENA_PAGES 1000
#define ARENA_PAGES (GRAPH_ARENA_PAGES + 1000)

#define GRAPH_MAX_VERTICES (12)
#define GRAPH_MAX_EDGES (3 * GRAPH_MAX_VERTICES - 6)

#define VERTEX_RADIUS 0.05f

#define BFS_TIMESTEP (AVEN_TIME_NSEC_PER_SEC / 100000)

typedef struct {
    AvenGlShapeCtx ctx;
    AvenGlShapeGeometry geometry;
    AvenGlShapeBuffer buffer;
} EdgeShapes;

typedef struct {
    AvenGlShapeRoundedCtx ctx;
    AvenGlShapeRoundedGeometry geometry;
    AvenGlShapeRoundedBuffer buffer;
} VertexShapes;

typedef struct {
    VertexShapes vertex_shapes;
    EdgeShapes edge_shapes;
    AvenGraph graph;
    AvenGraphPlaneEmbedding embedding;
    AvenRngPcg pcg;
    AvenRng rng;
    AvenGraphPlaneGenTriCtx tri_ctx;
    AvenArena pre_tri_ctx_arena;
    AvenTimeInst last_update;
    int64_t elapsed;
    int count;
} AppCtx;

static GLFWwindow *window;
static AvenGl gl;
static AppCtx ctx;
static AvenArena arena;

static void app_init(void) {
    ctx = (AppCtx){ 0 };
 
    ctx.edge_shapes.ctx = aven_gl_shape_ctx_init(&gl);
    ctx.edge_shapes.geometry = aven_gl_shape_geometry_init(
        (GRAPH_MAX_EDGES) * 4,
        (GRAPH_MAX_EDGES) * 6,
        &arena
    );
    ctx.edge_shapes.buffer = aven_gl_shape_buffer_init(
        &gl,
        &ctx.edge_shapes.geometry,
        AVEN_GL_BUFFER_USAGE_DYNAMIC
    );

    ctx.vertex_shapes.ctx = aven_gl_shape_rounded_ctx_init(&gl);
    ctx.vertex_shapes.geometry = aven_gl_shape_rounded_geometry_init(
        (GRAPH_MAX_VERTICES + 3) * 4 * 2,
        (GRAPH_MAX_VERTICES + 3) * 6 * 2,
        &arena
    );
    ctx.vertex_shapes.buffer = aven_gl_shape_rounded_buffer_init(
        &gl,
        &ctx.vertex_shapes.geometry,
        AVEN_GL_BUFFER_USAGE_DYNAMIC
    );

    ctx.pcg = aven_rng_pcg_seed(0xb00b123UL, 0xfafafeedUL);
    ctx.rng = aven_rng_pcg(&ctx.pcg);

    ctx.pre_tri_ctx_arena = arena;
    ctx.tri_ctx = aven_graph_plane_gen_tri_init(
        GRAPH_MAX_VERTICES,
        3.5f * AVEN_MATH_SQRT3_F * (VERTEX_RADIUS * VERTEX_RADIUS),
        0.01f,
        false,
        &arena
    );

    AvenGraphPlaneGenData data = aven_graph_plane_gen_tri_data(
        &ctx.tri_ctx,
        &arena
    );
    ctx.graph = data.graph;
    ctx.embedding = data.embedding;

    ctx.last_update = aven_time_now();
}

static void app_reset(void) {
    arena = ctx.pre_tri_ctx_arena;
    ctx.tri_ctx = aven_graph_plane_gen_tri_init(
        GRAPH_MAX_VERTICES,
        3.5f * AVEN_MATH_SQRT3_F * (VERTEX_RADIUS * VERTEX_RADIUS),
        0.01f,
        false,
        &arena
    );
    AvenGraphPlaneGenData data = aven_graph_plane_gen_tri_data(
        &ctx.tri_ctx,
        &arena
    );
    ctx.graph = data.graph;
    ctx.embedding = data.embedding;
}

static void app_deinit(void) {
    aven_gl_shape_rounded_geometry_deinit(&ctx.vertex_shapes.geometry);
    aven_gl_shape_rounded_buffer_deinit(&gl, &ctx.vertex_shapes.buffer);
    aven_gl_shape_rounded_ctx_deinit(&gl, &ctx.vertex_shapes.ctx);
    aven_gl_shape_geometry_deinit(&ctx.edge_shapes.geometry);
    aven_gl_shape_buffer_deinit(&gl, &ctx.edge_shapes.buffer);
    aven_gl_shape_ctx_deinit(&gl, &ctx.edge_shapes.ctx);
}

static void app_update(
    int width,
    int height
) {
    float ratio = (float)width / (float)height;

    float norm_height = 1.0f;
    float norm_width = ratio;
    float pixel_size = 2.0f / (float)height;

    if (ratio < 1.0f) {
        norm_height = 1.0f / ratio;
        norm_width = 1.0f;
        pixel_size = 2.0f / (float)width;
    }

    AvenTimeInst now = aven_time_now();
    int64_t elapsed = aven_time_since(now, ctx.last_update);
    ctx.last_update = now;
    ctx.elapsed += elapsed;

    bool changed = false;
    while (ctx.elapsed >= BFS_TIMESTEP) {
        changed = true;
        ctx.elapsed -= BFS_TIMESTEP;
        bool done = aven_graph_plane_gen_tri_step(&ctx.tri_ctx, ctx.rng);
        if (done) {
            changed = false;
            AvenArena temp_arena = arena;
            AvenGraphPlaneGenData data = aven_graph_plane_gen_tri_data(
                &ctx.tri_ctx,
                &temp_arena
            );
            ctx.graph = data.graph;
            ctx.embedding = data.embedding;

            bool success = true;
            for (uint32_t i = 0; i < ctx.graph.len; i += 1) {
                if (slice_get(ctx.graph, i).len != 5) {
                    success = false;
                }
            }

            if (!success) {
                printf("count: %d\n", ctx.count);
                ctx.count += 1;
                app_reset();
            }
        }
    }

    if (changed) {
        AvenArena temp_arena = arena;
        AvenGraphPlaneGenData data = aven_graph_plane_gen_tri_data(
            &ctx.tri_ctx,
            &temp_arena
        );
        ctx.graph = data.graph;
        ctx.embedding = data.embedding;
    }

    aven_gl_shape_geometry_clear(&ctx.edge_shapes.geometry);
    aven_gl_shape_rounded_geometry_clear(&ctx.vertex_shapes.geometry);

    Aff2 graph_transform;
    aff2_identity(graph_transform);
    aff2_stretch(
        graph_transform,
        (Vec2){ norm_width, norm_height },
        graph_transform
    );

    AvenGraphPlaneGeometryEdge edge_info = {
        .color = { 0.0f, 0.0f, 0.0f, 1.0f },
        .thickness = (VERTEX_RADIUS / 3.0f),
    };

    aven_graph_plane_geometry_push_edges(
        &ctx.edge_shapes.geometry,
        ctx.graph,
        ctx.embedding,
        graph_transform,
        &edge_info
    ); 

    AvenGraphPlaneGeometryNode node_outline_info = {
        .color = { 0.0f, 0.0f, 0.0f, 1.0f },
        .mat = { { VERTEX_RADIUS, 0.0f }, { 0.0f, VERTEX_RADIUS } },
        .shape = AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
        .roundness = 1.0f,
    };

    AvenGraphPlaneGeometryNode node_empty_info = {
        .color = { 0.9f, 0.9f, 0.9f, 1.0f },
        .mat = {
            { 0.75f * VERTEX_RADIUS, 0.0f },
            { 0.0f, 0.75f * VERTEX_RADIUS }
        },
        .shape = AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
        .roundness = 1.0f,
    };

    aven_graph_plane_geometry_push_vertices(
        &ctx.vertex_shapes.geometry,
        ctx.embedding,
        graph_transform,
        &node_outline_info
    );
    aven_graph_plane_geometry_push_vertices(
        &ctx.vertex_shapes.geometry,
        ctx.embedding,
        graph_transform,
        &node_empty_info
    );

    if (ctx.tri_ctx.active_face != AVEN_GRAPH_PLANE_GEN_FACE_INVALID) {
        AvenGraphPlaneGenFace face = list_get(
            ctx.tri_ctx.faces,
            ctx.tri_ctx.active_face
        );
        AvenGraphPlaneGeometryNode node_active_info = {
            .color = { 0.9f, 0.1f, 0.1f, 1.0f },
            .mat = {
                { 0.75f * VERTEX_RADIUS, 0.0f },
                { 0.0f, 0.75f * VERTEX_RADIUS }
            },
            .shape = AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
            .roundness = 1.0f,
        };

        for (uint32_t i = 0; i < 3; i += 1) {
            aven_graph_plane_geometry_push_vertex(
                &ctx.vertex_shapes.geometry,
                ctx.embedding,
                face.vertices[i],
                graph_transform,
                &node_active_info
            );
        }
    }

    gl.Viewport(0, 0, width, height);
    assert(gl.GetError() == 0);
    gl.ClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    assert(gl.GetError() == 0);
    gl.Clear(GL_COLOR_BUFFER_BIT);
    assert(gl.GetError() == 0);

    float border_padding = 2.0f * VERTEX_RADIUS;

    Aff2 cam_transform;
    aff2_camera_position(
        cam_transform,
        (Vec2){ 0.0f, 0.0f },
        (Vec2){ norm_width + border_padding, norm_height + border_padding },
        0.0f
    );

    aven_gl_shape_buffer_update(
        &gl,
        &ctx.edge_shapes.buffer,
        &ctx.edge_shapes.geometry
    );
    aven_gl_shape_rounded_buffer_update(
        &gl,
        &ctx.vertex_shapes.buffer,
        &ctx.vertex_shapes.geometry
    );

    aven_gl_shape_draw(
        &gl,
        &ctx.edge_shapes.ctx,
        &ctx.edge_shapes.buffer,
        cam_transform
    );
    aven_gl_shape_rounded_draw(
        &gl,
        &ctx.vertex_shapes.ctx,
        &ctx.vertex_shapes.buffer,
        pixel_size,
        cam_transform
    );
}

static void error_callback(int error, const char *description) {
    fprintf(stderr, "GFLW Error (%d): %s", error, description);
}

static void key_callback(
    GLFWwindow *win,
    int key,
    int scancode,
    int action,
    int mods
) {
    (void)scancode;
    (void)mods;
    if (key == GLFW_KEY_ESCAPE and action == GLFW_PRESS) {
        glfwSetWindowShouldClose(win, GLFW_TRUE);
    }
    if (key == GLFW_KEY_SPACE and action == GLFW_PRESS) {
        app_reset();
    }
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

void on_resize(int width, int height) {
    glfwSetWindowSize(window, width, height);
    printf("resized!\n");
}

void main_loop(void) {
    int width;
    int height;
    glfwGetFramebufferSize(window, &width, &height);

    app_update(width, height);

    glfwSwapBuffers(window);
    glfwPollEvents();
}
#endif
 
#if defined(_MSC_VER)
int WinMain(void) {
#else // !defined(_MSC_VER)
int main(void) {
#endif // !defined(_MSC_VER)
    aven_fs_utf8_mode();

    // Should probably switch to raw page allocation, but malloc is cross
    // platform and we are dynamically linking the system libc anyway
    void *mem = malloc(ARENA_PAGES * ARENA_PAGE_SIZE);
    if (mem == NULL) {
        fprintf(stderr, "OUT OF MEMORY\n");
        abort();
    }

    arena = aven_arena_init(mem, ARENA_PAGES * ARENA_PAGE_SIZE);

    int width = 480;
    int height = 480;

    glfwInit();
    glfwSetErrorCallback(error_callback);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_EGL_CONTEXT_API);
    // Raster msaa sample count should try to match the sample count of 4 used
    // for multisampling in our shape fragment shaders and stb font textures
    glfwWindowHint(GLFW_SAMPLES, 4);

    window = glfwCreateWindow(
        (int)width,
        (int)height,
        "AvenGraph BFS Example",
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
    app_init();

#if !defined(__EMSCRIPTEN__) // !defined(__EMSCRIPTEN__)
    while (!glfwWindowShouldClose(window)) {
        glfwGetFramebufferSize(window, &width, &height);

        app_update(width, height);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    app_deinit();

    glfwDestroyWindow(window);
    glfwTerminate();
#endif // !defined(__EMSCRIPTEN__)

    // we let the OS free arena memory (or leave it in the case of Emscripten)

    return 0;
}
