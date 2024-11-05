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
#include <aven/graph/plane/hartman.h>
#include <aven/graph/plane/hartman/geometry.h>
#include <aven/graph/plane/gen.h>
#include <aven/graph/plane/geometry.h>
#include <aven/path.h>
#include <aven/rng.h>
#include <aven/rng/pcg.h>
#include <aven/time.h>

#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>

#include "font.h"

#define ARENA_PAGE_SIZE 4096
#define GRAPH_ARENA_PAGES 1000
#define ARENA_PAGES (GRAPH_ARENA_PAGES + 1000)

#define GRAPH_MAX_VERTICES (1000)
#define GRAPH_MAX_EDGES (3 * GRAPH_MAX_VERTICES - 6)

#define VERTEX_RADIUS 0.035f

#define BFS_TIMESTEP (AVEN_TIME_NSEC_PER_SEC / 60)
#define DONE_WAIT_STEPS (5 * (AVEN_TIME_NSEC_PER_SEC / BFS_TIMESTEP))

#define COLOR_DIVISIONS 8UL
#define MAX_COLOR (((COLOR_DIVISIONS - 1UL) * (COLOR_DIVISIONS - 2UL)) / 2UL)

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
    AvenGlTextFont font;
    AvenGlTextCtx ctx;
    AvenGlTextGeometry geometry;
    AvenGlTextBuffer buffer;
} VertexText;

typedef enum {
    APP_STATE_GEN,
    APP_STATE_HARTMAN,
} AppState;

typedef union {
    struct {
        AvenGraphPlaneGenTriCtx ctx;
        AvenGraphPlaneGenTriData data;
    } gen;
    struct {
        AvenGraphPlaneHartmanCtx ctx;
        AvenGraphPlaneHartmanListProp color_lists;
    } hartman;
} AppData;

typedef struct {
    Vec4 color_data[MAX_COLOR];
    AvenGraphPlaneHartmanGeometryInfo hartman_geometry_info;
    VertexShapes vertex_shapes;
    EdgeShapes edge_shapes;
    VertexText vertex_text;
    AvenGraphPlaneEmbedding embedding;
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

static void app_reset(void) {
    arena = ctx.init_arena;
    ctx.state = APP_STATE_GEN;

    ctx.data.gen.data = aven_graph_plane_gen_tri_data_alloc(
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
    aff2_stretch(
        area_transform,
        ctx.norm_dim,
        area_transform
    );
    ctx.data.gen.ctx = aven_graph_plane_gen_tri_init(
        ctx.embedding,
        area_transform,
        1.25f * (VERTEX_RADIUS * VERTEX_RADIUS),
        0.001f,
        true,
        &arena
    );
    AvenGraphPlaneGenData data = aven_graph_plane_gen_tri_data(
        &ctx.data.gen.ctx,
        &ctx.data.gen.data
    );
    ctx.graph = data.graph;
    ctx.embedding = data.embedding;

    ctx.count = 0;
}

static void app_init(void) {
    ctx = (AppCtx){ .norm_dim = { 1.0f, 1.0f } };

    size_t color_index = 0;
    for (size_t i = 0; i < COLOR_DIVISIONS - 2UL; i += 1) {
        for (size_t j = i + 1; j < COLOR_DIVISIONS - 1UL; j += 1) {
            float coeffs[3] = {
                (float)(i + 1) / (float)COLOR_DIVISIONS,
                (float)(j + 1) / (float)COLOR_DIVISIONS,
                (float)(COLOR_DIVISIONS - (j + 1)) / (float)COLOR_DIVISIONS,
            };

            Vec4 base_colors[3] = {
                { 0.8f, 0.1f, 0.1f, 1.0f },
                { 0.1f, 0.8f, 0.1f, 1.0f },
                { 0.1f, 0.1f, 0.8f, 1.0f },
            };
            for (size_t k = 0; k < 3; k += 1) {
                vec4_scale(base_colors[k], coeffs[k], base_colors[k]);
                vec4_add(
                    ctx.color_data[color_index],
                    cxt.color_data[color_index],
                    base_colors[k]
                );
            }
            color_index += 1;
        }
    }

    ctx.hartman_geometry_info = (AvenGraphPlaneHartmanGeometryInfo){
        .list_colors = slice_array(ctx.color_data),
        .outline_color = { 0.15f, 0.15f, 0.15f, 1.0f },
        .active_color = { 0.5f, 0.1f, 0.5f, 1.0f },
        .cycle_edge_color = { 0.5f, 0.5f, 0.1f, 1.0f },
        .py_vertex_color = { 0.1f, 0.5f, 0.5f, 1.0f },
        .xp_vertex_color = { 0.65f, 0.65f, 0.35f },
        .yx_vertex_color = { 0.65f, 0.35f, 0.65f },
        .edge_thickness = VERTEX_RADIUS / 3.5f,
        .border_thickness = VERTEX_RADIUS * 0.25f,
        .radius = VERTEX_RADIUS,
    };
 
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

    ByteSlice font_bytes = array_as_bytes(game_font_opensans_ttf);
    ctx.vertex_text.font = aven_gl_text_font_init(
        &gl,
        font_bytes,
        20.0f,
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

    while (ctx.elapsed >= BFS_TIMESTEP) {
        ctx.elapsed -= BFS_TIMESTEP;

        bool done = false;
        switch (ctx.state) {
            case APP_STATE_GEN:
                done = aven_graph_plane_gen_tri_step(
                    &ctx.data.gen.ctx,
                    ctx.rng
                );

                if (done) {
                    AvenGraphPlaneGenData data = aven_graph_plane_gen_tri_data(
                        &ctx.data.gen.ctx,
                        &ctx.data.gen.data
                    );

                    AvenGraphAug aug_graph = aven_graph_aug(data.graph, &arena);

                    ctx.embedding = data.embedding;
                    ctx.state = APP_STATE_HARTMAN;

                    uint32_t outer_face_data[4];
                    AvenGraphSubset outer_face = {
                        .len = countof(outer_face_data),
                        .ptr = outer_face_data,
                    };

                    uint32_t v1 = aven_rng_rand_bounded(
                        ctx.rng,
                        countof(outer_face_data)
                    );
                    for (uint32_t i = 0; i < outer_face.len; i += 1) {
                        slice_get(outer_face, i) = (v1 + i) % outer_face.len;
                    }

                    ctx.data.hartman.color_lists.len = ctx.graph.len;
                    ctx.data.hartman.color_lists.ptr = aven_arena_create_array(
                        AvenGraphPlaneHartmanList,
                        &arena,
                        ctx.data.hartman.color_lists.len
                    );

                    for (uint32_t i = 0; i < ctx.graph.len; i += 1) {
                        AveGraphPlaneHartmanList list = {
                            .len = 3,
                            .data = {
                                aven_rng_rand_bounded(ctx.rng, MAX_COLOR),
                                aven_rng_rand_bounded(ctx.rng, MAX_COLOR),
                                aven_rng_rand_bounded(ctx.rng, MAX_COLOR),
                            },
                        };

                        while (list.data[1] == list.data[0]) {
                            list.data[1] = aven_rng_rand_bounded(
                                ctx.rng,
                                MAX_COLOR
                            );
                        }
                        while (
                            list.data[2] == list.data[1] or
                            list.data[2] == list.data[0]
                        ) {
                            list.data[2] = aven_rng_rand_bounded(
                                ctx.rng,
                                MAX_COLOR
                            );
                        }

                        slice_get(ctx.data.hartman.color_lists, i) = list;
                    }
                    
                    ctx.data.hartman.ctx = aven_graph_plane_hartman_init(
                        ctx.data.hartman.color_lists,
                        aug_graph,
                        outer_face,
                        &arena
                    );
                }
                break;
            case APP_STATE_HARTMAN:
                done = aven_graph_plane_hartman_check(&ctx.data.hartman.ctx);
                if (done) {
                    ctx.count += 1;
                    if (ctx.count > DONE_WAIT_STEPS) {
                        app_reset();
                    }
                    break;
                }

                aven_graph_plane_hartman_step(&ctx.data.hartman.ctx);
                break;
        }
    }

    aven_gl_shape_geometry_clear(&ctx.edge_shapes.geometry);
    aven_gl_shape_rounded_geometry_clear(&ctx.vertex_shapes.geometry);
    aven_gl_text_geometry_clear(&ctx.vertex_text.geometry);

    Aff2 graph_transform;
    aff2_identity(graph_transform);
    // aff2_stretch(
    //     graph_transform,
    //     (Vec2){ norm_width, norm_height },
    //     graph_transform
    // );

    switch (ctx.state) {
        case APP_STATE_GEN:
            aven_graph_plane_gen_geometry_push_tri_ctx(
                &ctx.edge_shapes.geometry,
                &ctx.vertex_shapes.geometry,
                &ctx.data.gen.ctx,
                graph_transform,
                &gen_geometry_info,
                arena
            );
            break;
        case APP_STATE_HARTMAN:
            aven_graph_plane_hartman_geometry_push_ctx(
                &ctx.edge_shapes.geometry,
                &ctx.vertex_shapes.geometry,
                ctx.embedding,
                ctx.data.hartman.ctx,
                graph_trans,
                &ctx.hartman_geometry_info
            );
            break;
    }

    aven_graph_plane_geometry_push_labels(
        &ctx.vertex_text.geometry,
        &ctx.vertex_text.font,
        ctx.embedding,
        graph_transform,
        (Vec2){ 0.0f, (ctx.vertex_text.font.height * pixel_size) / 4.0f },
        pixel_size,
        (Vec4){ 0.1f, 0.1f, 0.1f, 1.0f },
        arena
    );

    gl.Viewport(0, 0, width, height);
    assert(gl.GetError() == 0);
    gl.ClearColor(0.65f, 0.65f, 0.65f, 1.0f);
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
    aven_gl_text_buffer_update(
        &gl,
        &ctx.vertex_text.buffer,
        &ctx.vertex_text.geometry
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
    aven_gl_text_geometry_draw(
        &gl,
        &ctx.vertex_text.ctx,
        &ctx.vertex_text.buffer,
        &ctx.vertex_text.font,
        cam_transform
    );

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
