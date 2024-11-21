#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
    #define _POSIX_C_SOURCE 200112L
#endif

#define AVEN_IMPLEMENTATION
#include <aven.h>
#include <aven/arena.h>
#include <aven/fs.h>
#include <aven/gl.h>
#include <aven/gl/shape.h>
#include <aven/graph.h>
#include <aven/graph/path_color.h>
#include <aven/graph/plane.h>
#include <aven/graph/plane/hartman.h>
#include <aven/graph/plane/hartman/geometry.h>
#include <aven/graph/plane/gen.h>
#include <aven/graph/plane/gen/geometry.h>
#include <aven/graph/plane/geometry.h>
#include <aven/path.h>
#include <aven/rng.h>
#include <aven/rng/pcg.h>
#include <aven/time.h>

#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>

// #define SHOW_VERTEX_LABELS

#ifdef SHOW_VERTEX_LABELS
#include "font.h"
#endif

#define ARENA_PAGE_SIZE 4096
#define GRAPH_ARENA_PAGES 10000
#define ARENA_PAGES (GRAPH_ARENA_PAGES + 1000)

#define GRAPH_MAX_VERTICES (4000)
#define GRAPH_MAX_EDGES (3 * GRAPH_MAX_VERTICES - 6)

#define VERTEX_RADIUS 0.02f

#define BFS_TIMESTEP (AVEN_TIME_NSEC_PER_SEC / 30)
#define CHANGE_V_WAIT_STEPS 1
#define DONE_WAIT_STEPS (5 * (AVEN_TIME_NSEC_PER_SEC / BFS_TIMESTEP))

#define COLOR_DIVISIONS 2UL
#define MAX_COLOR (((COLOR_DIVISIONS + 2) * (COLOR_DIVISIONS + 1)) / 2UL)

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
    APP_STATE_HARTMAN,
    APP_STATE_COLORED,
} AppState;

typedef union {
    struct {
        AvenGraphPlaneGenTriCtx ctx;
        AvenGraphPlaneGenTriData data;
    } gen;
    struct {
        AvenGraphPlaneHartmanCtx ctx;
        AvenGraphPlaneHartmanFrameOptional frames[2];
        AvenGraphPlaneHartmanListProp color_lists;
    } hartman;
    struct {
        AvenGraphPropUint8 coloring;
    } colored;
} AppData;

typedef struct {
    Vec4 color_data[MAX_COLOR + 1];
    AvenGraphPlaneHartmanGeometryInfo hartman_geometry_info;
    VertexShapes vertex_shapes;
    EdgeShapes edge_shapes;
#ifdef SHOW_VERTEX_LABELS
    VertexText vertex_text;
#endif
    AvenGraph graph;
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

static AvenGraphPlaneGenGeometryTriInfo gen_geometry_info = {
    .node_color = { 0.9f, 0.9f, 0.9f, 1.0f },
    .outline_color = { 0.1f, 0.1f, 0.1f, 1.0f },
    .edge_color = { 0.1f, 0.1f, 0.1f, 1.0f },
    .active_color = { 0.5f, 0.1f, 0.5f, 1.0f },
    .radius = VERTEX_RADIUS,
    .border_thickness = VERTEX_RADIUS * 0.3f,
    .edge_thickness = VERTEX_RADIUS / 2.5f,
};

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
    // aff2_stretch(
    //     area_transform,
    //     ctx.norm_dim,
    //     area_transform
    // );
    ctx.data.gen.ctx = aven_graph_plane_gen_tri_init(
        ctx.embedding,
        area_transform,
        1.15f * (VERTEX_RADIUS * VERTEX_RADIUS),
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

    vec4_copy(
        ctx.color_data[0],
        (Vec4){ 0.9f, 0.9f, 0.9f, 1.0f}
    );

    size_t color_index = 1;
    for (size_t i = 0; i <= COLOR_DIVISIONS; i += 1) {
        for (size_t j = i; j <= COLOR_DIVISIONS; j += 1) {
            float coeffs[3] = {
                (float)(i) / (float)COLOR_DIVISIONS,
                (float)(j - i) / (float)COLOR_DIVISIONS,
                (float)(COLOR_DIVISIONS - j) /
                    (float)COLOR_DIVISIONS,
            };

            Vec4 base_colors[3] = {
                // { 0.9f, 0.2f, 0.2f, 1.0f },
                // { 0.2f, 0.9f, 0.2f, 1.0f },
                // { 0.2f, 0.2f, 0.9f, 1.0f },
                { 0.85f, 0.15f, 0.15f, 1.0f },
                { 0.15f, 0.75f, 0.15f, 1.0f },
                { 0.15f, 0.15f, 0.85f, 1.0f },
            };
            for (size_t k = 0; k < 3; k += 1) {
                vec4_scale(base_colors[k], coeffs[k], base_colors[k]);
                vec4_add(
                    ctx.color_data[color_index],
                    ctx.color_data[color_index],
                    base_colors[k]
                );
            }
            color_index += 1;
        }
    }

    ctx.hartman_geometry_info = (AvenGraphPlaneHartmanGeometryInfo){
        .colors = slice_array(ctx.color_data),
        .outline_color = { 0.1f, 0.1f, 0.1f, 1.0f },
        .edge_color = { 0.1f, 0.1f, 0.1f, 1.0f },
        .uncolored_edge_color = { 0.5f, 0.5f, 0.5f, 1.0f },
        .active_frame = {
            .active_color = { 0.5f, 0.1f, 0.5f, 1.0f },
            .xp_color = { 0.55f, 0.65f, 0.15f, 1.0f },
            .py_color = { 0.15f, 0.6f, 0.6f, 1.0f },
            .cycle_color = { 0.65f, 0.25f, 0.15f, 1.0f },
        },
        .inactive_frame = {
            .active_color = { 0.25f, 0.25f, 0.25f, 1.0f },
            .xp_color = { 0.4f, 0.4f, 0.4f, 1.0f },
            .py_color = { 0.4f, 0.4f, 0.4f, 1.0f },
            .cycle_color = { 0.35f, 0.35f, 0.35f, 1.0f },
        },
        .edge_thickness = VERTEX_RADIUS / 2.5f,
        .border_thickness = VERTEX_RADIUS * 0.3f,
        .radius = VERTEX_RADIUS,
    };
 
    ctx.edge_shapes.ctx = aven_gl_shape_ctx_init(&gl);
    ctx.edge_shapes.geometry = aven_gl_shape_geometry_init(
        (GRAPH_MAX_EDGES) * 4 * 10,
        (GRAPH_MAX_EDGES) * 6 * 10,
        &arena
    );
    ctx.edge_shapes.buffer = aven_gl_shape_buffer_init(
        &gl,
        &ctx.edge_shapes.geometry,
        AVEN_GL_BUFFER_USAGE_DYNAMIC
    );

    ctx.vertex_shapes.ctx = aven_gl_shape_rounded_ctx_init(&gl);
    ctx.vertex_shapes.geometry = aven_gl_shape_rounded_geometry_init(
        (GRAPH_MAX_VERTICES + 3) * 4 * 10,
        (GRAPH_MAX_VERTICES + 3) * 6 * 10,
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
        9.0f,
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

    ctx.pcg = aven_rng_pcg_seed(0xbeef2341UL, 0xdeada555UL);
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

    // if (ctx->camera_time > 0) {
    //     int64_t new_camera_time = ctx->camera_time - ctx.elapsed;
    //     ctx.elapsed = max(0, ctx.elapsed - ctx->camera_time);
    //     ctx->camera_time = new_camera_time;
    // }

    int64_t timestep = BFS_TIMESTEP;
    if (ctx.state == APP_STATE_GEN) {
        timestep = BFS_TIMESTEP / 8;
    }

    while (ctx.elapsed >= timestep) {
        ctx.elapsed -= timestep;

        bool done = false;
        switch (ctx.state) {
            case APP_STATE_GEN:
                done = aven_graph_plane_gen_tri_step(
                    &ctx.data.gen.ctx,
                    ctx.rng
                );
                ctx.embedding.len = ctx.data.gen.ctx.embedding.len;

                if (done) {
                    AvenGraphPlaneGenData data = aven_graph_plane_gen_tri_data(
                        &ctx.data.gen.ctx,
                        &ctx.data.gen.data
                    );

                    AvenGraphAug aug_graph = aven_graph_aug(data.graph, &arena);

                    ctx.graph = data.graph;
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
                        slice_get(outer_face, i) = (uint32_t)(
                            (v1 + i) % outer_face.len
                        );
                    }

                    ctx.data.hartman.color_lists.len = aug_graph.len;
                    ctx.data.hartman.color_lists.ptr = aven_arena_create_array(
                        AvenGraphPlaneHartmanList,
                        &arena,
                        ctx.data.hartman.color_lists.len
                    );

                    for (uint32_t i = 0; i < aug_graph.len; i += 1) {
                        AvenGraphPlaneHartmanList list = {
                            .len = 3,
                            .data = {
                                1 + aven_rng_rand_bounded(ctx.rng, MAX_COLOR),
                                1 + aven_rng_rand_bounded(ctx.rng, MAX_COLOR),
                                1 + aven_rng_rand_bounded(ctx.rng, MAX_COLOR),
                            },
                        };

                        while (list.data[1] == list.data[0]) {
                            list.data[1] = 1 + aven_rng_rand_bounded(
                                ctx.rng,
                                MAX_COLOR
                            );
                        }
                        while (
                            list.data[2] == list.data[1] or
                            list.data[2] == list.data[0]
                        ) {
                            list.data[2] = 1 + aven_rng_rand_bounded(
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
                    ctx.data.hartman.frames[0] = aven_graph_plane_hartman_next_frame(
                        &ctx.data.hartman.ctx
                    );
                }
                break;
            case APP_STATE_HARTMAN:
                ctx.count += 1;
                if (ctx.count > CHANGE_V_WAIT_STEPS) {
                    done = true;
                    for (
                        size_t i = 0;
                        i < countof(ctx.data.hartman.frames);
                        i += 1
                    ) {
                        AvenGraphPlaneHartmanFrameOptional *frame =
                            &ctx.data.hartman.frames[i];
                        if (frame->valid) {
                            frame->valid = !aven_graph_plane_hartman_frame_step(
                                &ctx.data.hartman.ctx,
                                &frame->value
                            );
                            done = false;
                        } else {
                            for (
                                size_t j = 0;
                                j < ctx.data.hartman.ctx.frames.len;
                                j += 1
                            ) {
                                AvenGraphPlaneHartmanFrame *nframe = &list_get(
                                    ctx.data.hartman.ctx.frames,
                                    j
                                );
                                if (
                                    slice_get(
                                        ctx.data.hartman.ctx.color_lists,
                                        nframe->v
                                    ).len == 1
                                ) {
                                    frame->value = *nframe;
                                    frame->valid = true;
                                    *nframe = list_get(
                                        ctx.data.hartman.ctx.frames,
                                        ctx.data.hartman.ctx.frames.len - 1
                                    );
                                    ctx.data.hartman.ctx.frames.len -= 1;
                                    break;
                                }
                            }
                        }

                        if (frame->valid) {
                            done = false;
                        }
                    }
                }
                if (done) {
                    ctx.state = APP_STATE_COLORED;
                    AvenGraphPropUint8 coloring = { .len = ctx.graph.len };
                    coloring.ptr = aven_arena_create_array(
                        uint8_t,
                        &arena,
                        coloring.len
                    );
                    for (uint32_t v = 0; v < ctx.graph.len; v += 1) {
                        slice_get(coloring, v) = (uint8_t)slice_get(
                            ctx.data.hartman.ctx.color_lists,
                            v
                        ).data[0];
                    }
                    ctx.data.colored.coloring = coloring;
                    ctx.count = 0;
                }
                break;
            case APP_STATE_COLORED:
                ctx.count += 1;
                if (ctx.count > DONE_WAIT_STEPS) {
                    if (aven_graph_path_color_verify(ctx.graph, ctx.data.colored.coloring, arena)) {
                        app_reset();
                    }
                }
                break;
        }

        if (ctx.state == APP_STATE_GEN) {
            timestep = BFS_TIMESTEP / 8;
        } else {
            timestep = BFS_TIMESTEP;
        }
    }

    aven_gl_shape_geometry_clear(&ctx.edge_shapes.geometry);
    aven_gl_shape_rounded_geometry_clear(&ctx.vertex_shapes.geometry);
#ifdef SHOW_VERTEX_LABELS
    aven_gl_text_geometry_clear(&ctx.vertex_text.geometry);
#endif

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
        case APP_STATE_HARTMAN: {
            AvenGraphPlaneHartmanFrame valid_frame_data[
                countof(ctx.data.hartman.frames)
            ];
            List(AvenGraphPlaneHartmanFrame) valid_frames = list_array(
                valid_frame_data
            );
            for (size_t i = 0; i < countof(ctx.data.hartman.frames); i += 1) {
                AvenGraphPlaneHartmanFrameOptional *frame =
                    &ctx.data.hartman.frames[i];
                if (frame->valid) {
                    list_push(valid_frames) = frame->value;
                }
            }
            AvenGraphPlaneHartmanFrameSlice vf_slice = slice_list(valid_frames);
            aven_graph_plane_hartman_geometry_push_ctx(
                &ctx.edge_shapes.geometry,
                &ctx.vertex_shapes.geometry,
                ctx.embedding,
                &ctx.data.hartman.ctx,
                vf_slice,
                graph_transform,
                &ctx.hartman_geometry_info
            );
            break;
        }
        case APP_STATE_COLORED: {
            AvenGraphPlaneGeometryEdge simple_edge_info = {
                .thickness = ctx.hartman_geometry_info.edge_thickness,
            };
            vec4_copy(
                simple_edge_info.color,
                ctx.hartman_geometry_info.uncolored_edge_color
            );
            aven_graph_plane_geometry_push_uncolored_edges(
                &ctx.edge_shapes.geometry,
                ctx.graph,
                ctx.embedding,
                ctx.data.colored.coloring,
                graph_transform,
                &simple_edge_info
            );
            AvenGraphPlaneGeometryEdge colored_edge_data[MAX_COLOR];
            AvenGraphPlaneGeometryEdgeSlice colored_edges = slice_array(
                colored_edge_data
            );
            for (uint32_t i = 0; i < colored_edges.len; i += 1) {
                slice_get(colored_edges, i).thickness =
                    ctx.hartman_geometry_info.edge_thickness;
                vec4_copy(
                    slice_get(colored_edges, i).color,
                    slice_get(ctx.hartman_geometry_info.colors, i + 1)
                );
            }
            aven_graph_plane_geometry_push_colored_edges(
                &ctx.edge_shapes.geometry,
                ctx.graph,
                ctx.embedding,
                ctx.data.colored.coloring,
                graph_transform,
                colored_edges
            );

            AvenGraphPlaneGeometryNode vertex_outline = {
                .mat = {
                    { ctx.hartman_geometry_info.radius, 0.0f },
                    { 0.0f, ctx.hartman_geometry_info.radius },
                },
                .shape = AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
                .roundness = 1.0f,
            };
            vec4_copy(
                vertex_outline.color,
                ctx.hartman_geometry_info.outline_color
            );
            aven_graph_plane_geometry_push_vertices(
                &ctx.vertex_shapes.geometry,
                ctx.embedding,
                graph_transform,
                &vertex_outline
            );

            AvenGraphPlaneGeometryNode colored_vertex_data[MAX_COLOR + 1];
            AvenGraphPlaneGeometryNodeSlice colored_vertices = slice_array(
                colored_vertex_data
            );
            float radius = ctx.hartman_geometry_info.radius -
                ctx.hartman_geometry_info.border_thickness;
            for (uint32_t i = 0; i < colored_vertices.len; i += 1) {
                slice_get(colored_vertices, i) = (AvenGraphPlaneGeometryNode) {
                    .mat = {
                        { radius, 0.0f, },
                        { 0.0f, radius },
                    },
                    .shape = AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
                    .roundness = 1.0f,
                };
                vec4_copy(
                    slice_get(colored_vertices, i).color,
                    slice_get(ctx.hartman_geometry_info.colors, i)
                );
            }
            aven_graph_plane_geometry_push_colored_vertices(
                &ctx.vertex_shapes.geometry,
                ctx.embedding,
                ctx.data.colored.coloring,
                graph_transform,
                colored_vertices
            );
            break;
        }
    }

#ifdef SHOW_VERTEX_LABELS
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
#endif

    gl.Viewport(0, 0, width, height);
    assert(gl.GetError() == 0);
    gl.ClearColor(0.75f, 0.75f, 0.75f, 1.0f);
    assert(gl.GetError() == 0);
    gl.Clear(GL_COLOR_BUFFER_BIT);
    assert(gl.GetError() == 0);

    // float camera_frac = (float)ctx->camera_time / (float)CAMERA_TIMESTEP;
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
