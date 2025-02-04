#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
    #define _POSIX_C_SOURCE 200112L
#endif

//#define SHOW_VERTEX_LABELS

#define AVEN_GRAPH_PLANE_P3COLOR_PTHREAD

#define AVEN_IMPLEMENTATION
#include <aven.h>
#include <aven/fs.h>
#include <aven/path.h>
#include <aven/rng.h>
#include <aven/rng/pcg.h>
#include <aven/time.h>

#include <aven/gl.h>
#include <aven/gl/shape.h>

#include <graph.h>
#include <graph/plane.h>
#include <graph/plane/p3color.h>
#include <graph/plane/p3color/geometry.h>
#include <graph/plane/gen.h>
#include <graph/plane/gen/geometry.h>

#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>

#ifdef SHOW_VERTEX_LABELS
    #include "font.h"
#endif

#define ARENA_PAGE_SIZE 4096
#define GRAPH_ARENA_PAGES 1000
#define ARENA_PAGES (GRAPH_ARENA_PAGES + 1000)

#define GRAPH_MAX_VERTICES (300)
#define GRAPH_MAX_EDGES (3 * GRAPH_MAX_VERTICES - 6)

#define VERTEX_RADIUS 0.12f

#define BFS_TIMESTEP (AVEN_TIME_NSEC_PER_SEC / 2)
#define DONE_WAIT_STEPS (5 * (AVEN_TIME_NSEC_PER_SEC / BFS_TIMESTEP))

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

#ifdef SHOW_VERTEX_LABELS
typedef struct {
    AvenGlTextFont font;
    AvenGlTextCtx ctx;
    AvenGlTextGeometry geometry;
    AvenGlTextBuffer buffer;
} VertexText;
#endif

typedef enum {
    APP_STATE_GEN,
    APP_STATE_P3COLOR,
} AppState;

typedef union {
    struct {
        GraphPlaneGenTriCtx ctx;
        GraphPlaneGenTriData data;
    } gen;
    struct {
        GraphPlaneP3ColorCtx ctx;
        GraphPlaneP3ColorFrameOptional active_frames[1];
    } p3color;
} AppData;

typedef struct {
    VertexShapes vertex_shapes;
    EdgeShapes edge_shapes;
#ifdef SHOW_VERTEX_LABELS
    VertexText vertex_text;
#endif
    Graph graph;
    GraphPlaneEmbedding embedding;
    AvenRngPcg pcg;
    AvenRng rng;
    AppState state;
    AppData data;
    AvenArena init_arena;
    AvenTimeInst last_update;
    Vec2 norm_dim;
    int64_t elapsed;
    int count;
} AppCtx;

static GLFWwindow *window;
static AvenGl gl;
static AppCtx ctx;
static AvenArena arena;

static GraphPlaneGenGeometryTriInfo gen_geometry_info = {
    .node_color = { 0.9f, 0.9f, 0.9f, 1.0f },
    .outline_color = { 0.15f, 0.15f, 0.15f, 1.0f },
    .edge_color = { 0.15f, 0.15f, 0.15f, 1.0f },
    .active_color = { 0.5f, 0.1f, 0.5f, 1.0f },
    .radius = VERTEX_RADIUS,
    .border_thickness = VERTEX_RADIUS * 0.25f,
    .edge_thickness = VERTEX_RADIUS / 3.5f,
};

static GraphPlaneP3ColorGeometryInfo p3color_geometry_info = {
    .colors = {
        { 0.9f, 0.9f, 0.9f, 1.0f },
        { 0.75f, 0.25f, 0.25f, 1.0f },
        { 0.2f, 0.55f, 0.15f, 1.0f },
        { 0.15f, 0.35f, 0.75f, 1.0f },
    },
    .outline_color = { 0.15f, 0.15f, 0.15f, 1.0f },
    .done_color = { 0.5f, 0.5f, 0.5f, 1.0f },
    .face_color = { 0.1f, 0.5f, 0.5f, 1.0f },
    .below_color = { 0.5f, 0.5f, 0.1f, 1.0f },
    .active_color = { 0.5f, 0.1f, 0.5f, 1.0f },
    .edge_thickness = VERTEX_RADIUS / 3.5f,
    .border_thickness = VERTEX_RADIUS * 0.25f,
    .radius = VERTEX_RADIUS,
};

static void app_reset(void) {
    arena = ctx.init_arena;
    ctx.state = APP_STATE_GEN;

    ctx.data.gen.data = graph_plane_gen_tri_data_alloc(
        GRAPH_MAX_VERTICES,
        &arena
    );
    ctx.embedding.len = GRAPH_MAX_VERTICES;
    ctx.embedding.ptr = aven_arena_create_array(
        Vec2,
        &arena,
        ctx.embedding.len
    );

    Aff2 area_transform;
    aff2_identity(area_transform);
    ctx.data.gen.ctx = graph_plane_gen_tri_init(
        ctx.embedding,
        area_transform,
        1.33f * (VERTEX_RADIUS * VERTEX_RADIUS),
        0.001f,
        true,
        &arena
    );
    GraphPlaneGenData data = graph_plane_gen_tri_data(
        &ctx.data.gen.ctx,
        &ctx.data.gen.data
    );
    ctx.graph = data.graph;
    ctx.embedding = data.embedding;

    ctx.count = 0;
}

static void app_init(void) {
    ctx = (AppCtx){ .norm_dim = { 1.0f, 1.0f } };
 
    ctx.edge_shapes.ctx = aven_gl_shape_ctx_init(&gl);
    ctx.edge_shapes.geometry = aven_gl_shape_geometry_init(
        (GRAPH_MAX_EDGES) * 4 * 4,
        (GRAPH_MAX_EDGES) * 6 * 4,
        &arena
    );
    ctx.edge_shapes.buffer = aven_gl_shape_buffer_init(
        &gl,
        &ctx.edge_shapes.geometry,
        AVEN_GL_BUFFER_USAGE_DYNAMIC
    );

    ctx.vertex_shapes.ctx = aven_gl_shape_rounded_ctx_init(&gl);
    ctx.vertex_shapes.geometry = aven_gl_shape_rounded_geometry_init(
        (GRAPH_MAX_VERTICES + 3) * 4 * 4,
        (GRAPH_MAX_VERTICES + 3) * 6 * 4,
        &arena
    );
    ctx.vertex_shapes.buffer = aven_gl_shape_rounded_buffer_init(
        &gl,
        &ctx.vertex_shapes.geometry,
        AVEN_GL_BUFFER_USAGE_DYNAMIC
    );

#ifdef SHOW_VERTEX_LABELS
    ByteSlice font_bytes = array_as_bytes(game_font_opensans_ttf);
    ctx.vertex_text.font = aven_gl_text_font_init(
        &gl,
        font_bytes,
        16.0f,
        arena
    );

    ctx.vertex_text.ctx = aven_gl_text_ctx_init(&gl);
    ctx.vertex_text.geometry = aven_gl_text_geometry_init(
        GRAPH_MAX_VERTICES * 10,
        &arena
    );
    ctx.vertex_text.buffer = aven_gl_text_buffer_init(
        &gl,
        &ctx.vertex_text.geometry,
        AVEN_GL_BUFFER_USAGE_DYNAMIC
    );
#endif 

    ctx.pcg = aven_rng_pcg_seed(0xbeef1234UL, 0x9abcdeadUL);
    ctx.rng = aven_rng_pcg(&ctx.pcg);
    
    ctx.init_arena = arena;
    app_reset();

    ctx.last_update = aven_time_now();
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

    ctx.norm_dim[0] = norm_width;
    ctx.norm_dim[1] = norm_height;

    int64_t timestep = BFS_TIMESTEP;
    if (ctx.state == APP_STATE_GEN) {
        timestep = BFS_TIMESTEP / 8;
    }

    while (ctx.elapsed >= timestep) {
        ctx.elapsed -= timestep;

        bool done = false;
        switch (ctx.state) {
            case APP_STATE_GEN:
                done = graph_plane_gen_tri_step(
                    &ctx.data.gen.ctx,
                    ctx.rng
                );

                GraphPlaneGenData data = graph_plane_gen_tri_data(
                    &ctx.data.gen.ctx,
                    &ctx.data.gen.data
                );
                ctx.graph = data.graph;
                ctx.embedding = data.embedding;

                if (done) {
                    ctx.state = APP_STATE_P3COLOR;

                    uint32_t p_data[3];
                    uint32_t q_data[3];

                    uint32_t p1 = aven_rng_rand_bounded(ctx.rng, 4);
                    GraphSubset p = {
                        .len = 1 + aven_rng_rand_bounded(ctx.rng, 3),
                        .ptr = p_data,
                    };
                    for (uint32_t i = 0; i < p.len; i += 1) {
                        get(p, i) = (p1 + i) % 4;
                    }

                    uint32_t q1 = (p1 + (uint32_t)p.len) % 4;
                    GraphSubset q = { .len = 4 - p.len, .ptr = q_data };
                    for (uint32_t i = 0; i < q.len; i += 1) {
                        get(q, q.len - i - 1) = (q1 + i) % 4;
                    }

                    ctx.data.p3color.ctx = graph_plane_p3color_init(
                        ctx.graph,
                        p,
                        q,
                        &arena
                    );
                    ctx.data.p3color.active_frames[0] = graph_plane_p3color_next_frame(
                        &ctx.data.p3color.ctx
                    );
                }
                break;
            case APP_STATE_P3COLOR: {
                bool all_done = true;
                for (uint32_t i = 0; i < countof(ctx.data.p3color.active_frames); i += 1) {
                    if (ctx.data.p3color.active_frames[i].valid) {
                        bool frame_done = graph_plane_p3color_frame_step(
                            &ctx.data.p3color.ctx,
                            &ctx.data.p3color.active_frames[i].value
                        );
                        if (frame_done) {
                            if (ctx.data.p3color.ctx.frames.len > 0) {
                                ctx.data.p3color.active_frames[i] = graph_plane_p3color_next_frame(
                                    &ctx.data.p3color.ctx
                                );
                            } else {
                                ctx.data.p3color.active_frames[i].valid = false;
                            }
                        }
                        all_done = false;
                    } else if (ctx.data.p3color.ctx.frames.len > 0) {
                        ctx.data.p3color.active_frames[i] = graph_plane_p3color_next_frame(
                            &ctx.data.p3color.ctx
                        );
                        all_done = false;
                    }
                }
                if (all_done) {
                    ctx.count += 1;
                    if (ctx.count > DONE_WAIT_STEPS) {
                        app_reset();
                    }
                    break;
                }
                break;
            }
        }

        timestep = BFS_TIMESTEP;
        if (ctx.state == APP_STATE_GEN) {
            timestep = BFS_TIMESTEP / 8;
        }
    }

    aven_gl_shape_geometry_clear(&ctx.edge_shapes.geometry);
    aven_gl_shape_rounded_geometry_clear(&ctx.vertex_shapes.geometry);
#ifdef SHOW_VERTEX_LABELS
    aven_gl_text_geometry_clear(&ctx.vertex_text.geometry);
#endif

    Aff2 graph_transform;
    aff2_identity(graph_transform);

    switch (ctx.state) {
        case APP_STATE_GEN:
            graph_plane_gen_geometry_push_tri_ctx(
                &ctx.edge_shapes.geometry,
                &ctx.vertex_shapes.geometry,
                &ctx.data.gen.ctx,
                graph_transform,
                &gen_geometry_info,
                arena
            );
            break;
        case APP_STATE_P3COLOR:
            graph_plane_p3color_geometry_push_ctx(
                &ctx.edge_shapes.geometry,
                &ctx.vertex_shapes.geometry,
                ctx.embedding,
                &ctx.data.p3color.ctx,
                &ctx.data.p3color.active_frames[0],
                graph_transform,
                &p3color_geometry_info,
                arena
            );
            break;
    }

#ifdef SHOW_VERTEX_LABELS
    graph_plane_geometry_push_labels(
        &ctx.vertex_text.geometry,
        &ctx.vertex_text.font,
        ctx.embedding,
        graph_transform,
        (Vec2){ 0.0f, (ctx.vertex_text.font.height * pixel_size) / 4.0f },
        pixel_size,
        (Vec4){ 0.1f, 0.1f, 0.1f, 1.0f },
        arena
    );
#endif

    gl.Viewport(0, 0, width, height);
    assert(gl.GetError() == 0);
    gl.ClearColor(0.65f, 0.65f, 0.65f, 1.0f);
    assert(gl.GetError() == 0);
    gl.Clear(GL_COLOR_BUFFER_BIT);
    assert(gl.GetError() == 0);

    float border_padding = 2.0f * VERTEX_RADIUS;
    float scale = 1.0f / (1.0f + border_padding);

    Aff2 cam_transform;
    aff2_camera_position(
        cam_transform,
        (Vec2){ 0.0f, 0.0f },
        (Vec2){ norm_width + border_padding, norm_height + border_padding },
        0.0f
    );
    aff2_stretch(cam_transform, (Vec2){ scale, scale }, cam_transform);

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
#ifdef SHOW_VERTEX_LABELS
    aven_gl_text_buffer_update(
        &gl,
        &ctx.vertex_text.buffer,
        &ctx.vertex_text.geometry
    );
#endif

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
#ifdef SHOW_VERTEX_LABELS 
    aven_gl_text_geometry_draw(
        &gl,
        &ctx.vertex_text.ctx,
        &ctx.vertex_text.buffer,
        &ctx.vertex_text.font,
        cam_transform
    );
#endif

    gl.ColorMask(false, false, false, true);
    assert(gl.GetError() == 0);
    gl.ClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    assert(gl.GetError() == 0);
    gl.Clear(GL_COLOR_BUFFER_BIT);
    assert(gl.GetError() == 0);
    gl.ColorMask(true, true, true, true);
    assert(gl.GetError() == 0);
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
    glfwWindowHint(GLFW_SAMPLES, 16);

    window = glfwCreateWindow(
        (int)width,
        (int)height,
        "Graph BFS Example",
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
