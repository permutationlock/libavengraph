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
    APP_STATE_POH,
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
    VertexShapes vertex_shapes;
    EdgeShapes edge_shapes;
    VertexText vertex_text;
    AvenGraph graph;
    AvenGraphPlaneEmbedding embedding;
    AvenRngPcg pcg;
    AvenRng rng;
    AppState state;
    AppData data;
    AvenArena init_arena;
    AvenTimeInst last_update;
    Aff2 last_camera_transform;
    Aff2 camera_transform;
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
        1.2f * (VERTEX_RADIUS * VERTEX_RADIUS),
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

                AvenGraphPlaneGenData data = aven_graph_plane_gen_tri_data(
                    &ctx.data.gen.ctx,
                    &ctx.data.gen.data
                );
                ctx.graph = data.graph;
                ctx.embedding = data.embedding;

                if (done) {
                    ctx.state = APP_STATE_POH;

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
                    
                    ctx.data.poh.ctx = aven_graph_plane_poh_init(
                        ctx.data.hartman.color_lists,
                        ctx.graph,
                        p,
                        q,
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

    AvenGraphPlaneHartmanGeometryInfo hartman_draw_info = {
        .list_colors = slice_array(list_color_data),
        .outline_color = { 0.15f, 0.15f, 0.15f, 1.0f },
        .active_color = { 0.5f, 0.1f, 0.5f, 1.0f },
        .cycle_edge_color = { 0.5f, 0.5f, 0.1f, 1.0f },
        .py_vertex_color = { 0.1f, 0.5f, 0.5f, 1.0f },
        .xp_vertex_color = { 0.65f, 0.65f, 0.35f },
        .yx_vertex_color = { 0.65f, 0.35f, 0.65f },
        .radius = VERTEX_RADIUS,
        .border_thickness = VERTEX_RADIUS / 5.0f,
    }

    AvenGraphPlaneGeometryEdge edge_active_info = {
        .color = { 0.5f, 0.1f, 0.5f, 1.0f },
        .thickness = (VERTEX_RADIUS / 1.5f),
    };
    AvenGraphPlaneGeometryNode node_active_info = {
        .color = { 0.5f, 0.1f, 0.5f, 1.0f },
        .mat = {
            { 1.33f * VERTEX_RADIUS, 0.0f },
            { 0.0f, 1.33f * VERTEX_RADIUS }
        },
        .shape = AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
        .roundness = 1.0f,
    };
    AvenGraphPlaneGeometryEdge edge_below_info = {
        .color = { 0.5f, 0.5f, 0.1f, 1.0f },
        .thickness = (VERTEX_RADIUS / 1.5f),
    };
    AvenGraphPlaneGeometryNode node_below_info = {
        .color = { 0.5f, 0.5f, 0.1f, 1.0f },
        .mat = {
            { 1.25f * VERTEX_RADIUS, 0.0f },
            { 0.0f, 1.25f * VERTEX_RADIUS }
        },
        .shape = AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
        .roundness = 1.0f,
    };
    AvenGraphPlaneGeometryEdge edge_marked_info = {
        .color = { 0.1f, 0.5f, 0.5f, 1.0f },
        .thickness = (VERTEX_RADIUS / 1.5f),
    };
    AvenGraphPlaneGeometryNode node_marked_info = {
        .color = { 0.1f, 0.5f, 0.5f, 1.0f },
        .mat = {
            { 1.25f * VERTEX_RADIUS, 0.0f },
            { 0.0f, 1.25f * VERTEX_RADIUS }
        },
        .shape = AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
        .roundness = 1.0f,
    };

    switch (ctx.state) {
        case APP_STATE_GEN:
            if (
                ctx.data.gen.ctx.active_face !=
                    AVEN_GRAPH_PLANE_GEN_FACE_INVALID
            ) {
                AvenGraphPlaneGenFace face = list_get(
                    ctx.data.gen.ctx.faces,
                    ctx.data.gen.ctx.active_face
                );
                for (uint32_t i = 0; i < 3; i += 1) {
                    uint32_t j = (i + 1) % 3;
                    aven_graph_plane_geometry_push_edge(
                        &ctx.edge_shapes.geometry,
                        ctx.embedding,
                        face.vertices[i],
                        face.vertices[j],
                        graph_transform,
                        &edge_active_info
                    );
                }
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
            break;
        case APP_STATE_POH:
            if (ctx.data.hartman.ctx.frames.len == 0) {
                break;
            }
            AvenGraphPlanePohFrame *frame = &list_get(
                ctx.data.poh.ctx.frames,
                ctx.data.poh.ctx.frames.len - 1
            );

            aven_graph_plane_geometry_push_marked_edges(
                &ctx.edge_shapes.geometry,
                ctx.graph,
                ctx.embedding,
                ctx.data.poh.ctx.marks,
                frame->face_mark,
                graph_transform,
                &edge_marked_info
            );
            aven_graph_plane_geometry_push_marked_edges(
                &ctx.edge_shapes.geometry,
                ctx.graph,
                ctx.embedding,
                ctx.data.poh.ctx.marks,
                frame->face_mark + 1,
                graph_transform,
                &edge_below_info
            );

            AvenGraphAdjList u_adj = slice_get(ctx.graph, frame->u);
            if (frame->edge_index < u_adj.len) {
                aven_graph_plane_geometry_push_edge(
                    &ctx.edge_shapes.geometry,
                    ctx.embedding,
                    frame->u,
                    slice_get(
                        u_adj,
                        (slice_get(ctx.data.poh.ctx.first_edges, frame->u) +
                            frame->edge_index) % u_adj.len
                    ),
                    graph_transform,
                    &edge_active_info
                );
            }

            aven_graph_plane_geometry_push_marked_vertices(
                &ctx.vertex_shapes.geometry,
                ctx.embedding,
                ctx.data.poh.ctx.marks,
                frame->face_mark,
                graph_transform,
                &node_marked_info
            );

            aven_graph_plane_geometry_push_marked_vertices(
                &ctx.vertex_shapes.geometry,
                ctx.embedding,
                ctx.data.poh.ctx.marks,
                frame->face_mark + 1,
                graph_transform,
                &node_below_info
            );

            aven_graph_plane_geometry_push_vertex(
                &ctx.vertex_shapes.geometry,
                ctx.embedding,
                frame->u,
                graph_transform,
                &node_active_info
            );

            break;
    }

    AvenGraphPlaneGeometryEdge edge_info = {
        .color = { 0.15f, 0.15f, 0.15f, 1.0f },
        .thickness = (VERTEX_RADIUS / 3.5f),
    };
    AvenGraphPlaneGeometryEdge edge_color_data[] = {
        {
            .color = { 0.75f, 0.25f, 0.25f, 1.0f },
            .thickness = (VERTEX_RADIUS / 2.5f),
        },
        {
            .color = { 0.2f, 0.55f, 0.15f, 1.0f },
            .thickness = (VERTEX_RADIUS / 2.5f),
        },
        {
            .color = { 0.15f, 0.35f, 0.75f, 1.0f },
            .thickness = (VERTEX_RADIUS / 2.5f),
        },
    };
    AvenGraphPlaneGeometryEdgeSlice edge_colors = slice_array(edge_color_data);

    switch (ctx.state) {
        case APP_STATE_GEN:
            aven_graph_plane_geometry_push_edges(
                &ctx.edge_shapes.geometry,
                ctx.graph,
                ctx.embedding,
                graph_transform,
                &edge_info
            );
            break;
        case APP_STATE_POH:
            aven_graph_plane_geometry_push_uncolored_edges(
                &ctx.edge_shapes.geometry,
                ctx.graph,
                ctx.embedding,
                ctx.data.poh.coloring,
                graph_transform,
                &edge_info
            );
            aven_graph_plane_geometry_push_colored_edges(
                &ctx.edge_shapes.geometry,
                ctx.graph,
                ctx.embedding,
                ctx.data.poh.coloring,
                graph_transform,
                edge_colors
            );
            break;
    }

    AvenGraphPlaneGeometryNode node_outline_info = {
        .color = { 0.15f, 0.15f, 0.15f, 1.0f },
        .mat = { { VERTEX_RADIUS, 0.0f }, { 0.0f, VERTEX_RADIUS } },
        .shape = AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
        .roundness = 1.0f,
    };

    aven_graph_plane_geometry_push_vertices(
        &ctx.vertex_shapes.geometry,
        ctx.embedding,
        graph_transform,
        &node_outline_info
    );

    AvenGraphPlaneGeometryNode node_empty_info = {
        .color = { 0.9f, 0.9f, 0.9f, 1.0f },
        .mat = {
            { 0.75f * VERTEX_RADIUS, 0.0f },
            { 0.0f, 0.75f * VERTEX_RADIUS }
        },
        .shape = AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
        .roundness = 1.0f,
    };

    AvenGraphPlaneGeometryNode node_red_info = {
        .color = { 0.75f, 0.25f, 0.25f, 1.0f },
        .mat = {
            { 0.75f * VERTEX_RADIUS, 0.0f },
            { 0.0f, 0.75f * VERTEX_RADIUS }
        },
        .shape = AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
        .roundness = 1.0f,
    };

    AvenGraphPlaneGeometryNode node_green_info = {
        .color = { 0.2f, 0.55f, 0.15f, 1.0f },
        .mat = {
            { 0.75f * VERTEX_RADIUS, 0.0f },
            { 0.0f, 0.75f * VERTEX_RADIUS }
        },
        .shape = AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
        .roundness = 1.0f,
    };

    AvenGraphPlaneGeometryNode node_blue_info = {
        .color = { 0.15f, 0.35f, 0.75f, 1.0f },
        .mat = {
            { 0.75f * VERTEX_RADIUS, 0.0f },
            { 0.0f, 0.75f * VERTEX_RADIUS }
        },
        .shape = AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
        .roundness = 1.0f,
    };

    AvenGraphPlaneGeometryNode node_color_data[] = {
        node_empty_info,
        node_red_info,
        node_green_info,
        node_blue_info,
    };
    AvenGraphPlaneGeometryNodeSlice node_colors = slice_array(node_color_data);

    switch (ctx.state) {
        case APP_STATE_GEN:
            aven_graph_plane_geometry_push_vertices(
                &ctx.vertex_shapes.geometry,
                ctx.embedding,
                graph_transform,
                &node_empty_info
            );
            break;
        case APP_STATE_POH:
            aven_graph_plane_geometry_push_colored_vertices(
                &ctx.vertex_shapes.geometry,
                ctx.embedding,
                ctx.data.poh.coloring,
                graph_transform,
                node_colors
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
