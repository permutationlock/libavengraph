#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
    #define _POSIX_C_SOURCE 200112L
#endif

#define AVEN_IMPLEMENTATION
#include <aven.h>
#include <aven/arena.h>
#include <aven/fs.h>
#include <aven/gl.h>
#include <aven/gl/shape.h>
#include <graph.h>
#include <graph/path_color.h>
#include <graph/plane.h>
#include <graph/plane/p3choose.h>
#include <graph/plane/p3choose/geometry.h>
#include <graph/plane/gen.h>
#include <graph/plane/gen/geometry.h>
#include <graph/plane/geometry.h>
#include <aven/path.h>
#include <aven/rng.h>
#include <aven/rng/pcg.h>
#include <aven/time.h>

#include <GLFW/glfw3.h>

#include <stdio.h>
#include <stdlib.h>

// #define SHOW_VERTEX_LABELS

// #ifdef SHOW_VERTEX_LABELS
    #include "font.h"
// #endif

#define ARENA_PAGE_SIZE 4096
#define GRAPH_ARENA_PAGES 2500
#define ARENA_PAGES (GRAPH_ARENA_PAGES + 1000)

#define SCREEN_UPDATES 4

#define GRAPH_MAX_VERTICES (1600)
#define GRAPH_MAX_EDGES (3 * GRAPH_MAX_VERTICES - 6)

#define VERTEX_RADIUS 0.12f

#define TIMESTEP (3L * (AVEN_TIME_NSEC_PER_SEC / 4L))

#define COLOR_DIVISIONS 2UL
#define MAX_COLOR (((COLOR_DIVISIONS + 2) * (COLOR_DIVISIONS + 1)) / 2UL)

#define HELP_FONT_SIZE 36.0f

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

typedef struct {
    AvenGlTextFont font;
    AvenGlTextCtx ctx;
    AvenGlTextGeometry geometry;
    AvenGlTextBuffer buffer;
} HelpText;

typedef enum {
    APP_STATE_GEN,
    APP_STATE_P3CHOOSE,
    APP_STATE_COLORED,
} AppState;

typedef union {
    struct {
        GraphPlaneGenTriCtx ctx;
        GraphPlaneGenTriData data;
    } gen;
    struct {
        GraphPlaneP3ChooseCtx ctx;
        GraphPlaneP3ChooseFrameOptional frames[3];
        GraphPlaneP3ChooseListProp color_lists;
        bool cases_satisfied[12];
    } p3choose;
    struct {
        GraphPropUint8 coloring;
    } colored;
} AppData;

typedef struct {
    Vec4 color_data[MAX_COLOR + 1];
    GraphPlaneP3ChooseGeometryInfo p3choose_geometry_info;
    VertexShapes vertex_shapes;
    EdgeShapes edge_shapes;
    HelpText help_text;
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
    AvenTimeInst step_update;
    AvenTimeInst last_update;
    Vec2 norm_dim;
    int64_t elapsed;
    int64_t timestep;
    size_t threads;
    int64_t count;
    float radius;
    int updates;
    bool paused;
    bool autoplay;
    bool step;
    bool step_held;
} AppCtx;

static GLFWwindow *window;
static AvenGl gl;
static AppCtx ctx;
static AvenArena arena;

static GraphPlaneGenGeometryTriInfo gen_geometry_info;

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
        2.0f * (ctx.radius * ctx.radius),
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

    gen_geometry_info = (GraphPlaneGenGeometryTriInfo){
        .node_color = { 0.9f, 0.9f, 0.9f, 1.0f },
        .outline_color = { 0.1f, 0.1f, 0.1f, 1.0f },
        .edge_color = { 0.1f, 0.1f, 0.1f, 1.0f },
        .active_color = { 0.5f, 0.1f, 0.5f, 1.0f },
        .radius = ctx.radius,
        .border_thickness = ctx.radius * 0.25f,
        .edge_thickness = ctx.radius / 2.5f,
    };

    ctx.p3choose_geometry_info = (GraphPlaneP3ChooseGeometryInfo){
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
            .active_color = { 0.4f, 0.4f, 0.4f, 1.0f },
            .xp_color = { 0.4f, 0.4f, 0.4f, 1.0f },
            .py_color = { 0.4f, 0.4f, 0.4f, 1.0f },
            .cycle_color = { 0.4f, 0.4f, 0.4f, 1.0f },
        },
        .edge_thickness = ctx.radius / 2.5f,
        .border_thickness = ctx.radius * 0.25f,
        .radius = ctx.radius,
    };
}

static void app_init(void) {
    ctx = (AppCtx){
        .norm_dim = { 1.0f, 1.0f },
        .radius = VERTEX_RADIUS,
        .timestep = TIMESTEP,
        .threads = 1,
        .autoplay = true,
    };

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

    ByteSlice font_bytes = array_as_bytes(game_font_opensans_ttf);
    ctx.help_text.font = aven_gl_text_font_init(
        &gl,
        font_bytes,
        HELP_FONT_SIZE,
        arena
    );

    ctx.help_text.ctx = aven_gl_text_ctx_init(&gl);
    ctx.help_text.geometry = aven_gl_text_geometry_init(
        GRAPH_MAX_VERTICES * 10,
        &arena
    );
    ctx.help_text.buffer = aven_gl_text_buffer_init(
        &gl,
        &ctx.help_text.geometry,
        AVEN_GL_BUFFER_USAGE_DYNAMIC
    );

#ifdef SHOW_VERTEX_LABELS
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

    float text_scale = 0.08f / HELP_FONT_SIZE;

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

    int64_t timestep = ctx.timestep;
    if (ctx.state == APP_STATE_GEN) {
        timestep /= 8;
    }

    if (
        ctx.paused and !(
            ctx.step_held and
            aven_time_since(now, ctx.step_update) >
                AVEN_TIME_NSEC_PER_SEC / 2
        )
    ) {
        ctx.elapsed = 0;
    }

    while (ctx.step or ctx.elapsed >= timestep) {
        ctx.step = false;
        ctx.elapsed -= timestep;

        bool done = false;
        switch (ctx.state) {
            case APP_STATE_GEN:
                ctx.updates = 0;
                done = graph_plane_gen_tri_step(
                    &ctx.data.gen.ctx,
                    ctx.rng
                );
                ctx.embedding.len = ctx.data.gen.ctx.embedding.len;

                if (done) {
                    GraphPlaneGenData data = graph_plane_gen_tri_data(
                        &ctx.data.gen.ctx,
                        &ctx.data.gen.data
                    );

                    GraphAug aug_graph = graph_aug(data.graph, &arena);

                    ctx.graph = data.graph;
                    ctx.embedding = data.embedding;
                    ctx.state = APP_STATE_P3CHOOSE;

                    uint32_t outer_face_data[4];
                    GraphSubset outer_face = {
                        .len = countof(outer_face_data),
                        .ptr = outer_face_data,
                    };

                    uint32_t v1 = aven_rng_rand_bounded(
                        ctx.rng,
                        countof(outer_face_data)
                    );
                    for (uint32_t i = 0; i < outer_face.len; i += 1) {
                        get(outer_face, i) = (uint32_t)(
                            (v1 + i) % outer_face.len
                        );
                    }

                    ctx.data.p3choose.color_lists.len = aug_graph.len;
                    ctx.data.p3choose.color_lists.ptr = aven_arena_create_array(
                        GraphPlaneP3ChooseList,
                        &arena,
                        ctx.data.p3choose.color_lists.len
                    );

                    for (uint32_t j = 0; j < aug_graph.len; j += 1) {
                        GraphPlaneP3ChooseList list = { .len = 3 };
                        get(list, 0) = (uint8_t)(
                            1 + aven_rng_rand_bounded(ctx.rng, MAX_COLOR)
                        );
                        get(list, 1) = (uint8_t)(
                            1 + aven_rng_rand_bounded(ctx.rng, MAX_COLOR)
                        );
                        get(list, 2) = (uint8_t)(
                            1 + aven_rng_rand_bounded(ctx.rng, MAX_COLOR)
                        );

                        while (get(list, 1) == get(list, 0)) {
                            get(list, 1) = (uint8_t)(
                                1 + aven_rng_rand_bounded(ctx.rng, MAX_COLOR)
                            );
                        }
                        while (
                            get(list, 2) == get(list, 1) or
                            get(list, 2) == get(list, 0)
                        ) {
                            get(list, 2) = (uint8_t)(
                                1 + aven_rng_rand_bounded(ctx.rng, MAX_COLOR)
                            );
                        }

                        get(ctx.data.p3choose.color_lists, j) = list;
                    }
                    
                    ctx.data.p3choose.ctx = graph_plane_p3choose_init(
                        aug_graph,
                        ctx.data.p3choose.color_lists,
                        outer_face,
                        &arena
                    );
                    ctx.data.p3choose.frames[0] = graph_plane_p3choose_next_frame(
                        &ctx.data.p3choose.ctx
                    );
                    for (
                        size_t j = 0;
                        j < countof(ctx.data.p3choose.cases_satisfied);
                        j += 1
                    ) {
                        ctx.data.p3choose.cases_satisfied[j] = 0;
                    }
                    for (
                        size_t i = 1;
                        i < countof(ctx.data.p3choose.frames);
                        i += 1
                    ) {
                        ctx.data.p3choose.frames[i] =
                            (GraphPlaneP3ChooseFrameOptional){ 0 };
                    }
                }
                break;
            case APP_STATE_P3CHOOSE:
                ctx.updates = 0;
                done = true;
                for (
                    size_t i = 0;
                    i < ctx.threads;
                    i += 1
                ) {
                    GraphPlaneP3ChooseFrameOptional *frame =
                        &ctx.data.p3choose.frames[i];
                    if (frame->valid) {
                        ctx.data.p3choose.cases_satisfied[
                            graph_plane_p3choose_frame_case(
                                &ctx.data.p3choose.ctx,
                                &frame->value
                            )
                        ] = true;
                        frame->valid = !graph_plane_p3choose_frame_step(
                            &ctx.data.p3choose.ctx,
                            &frame->value
                        );
                        done = false;
                    } else {
                        for (
                            size_t j = ctx.data.p3choose.ctx.frames.len;
                            j > 0;
                            j -= 1
                        ) {
                            GraphPlaneP3ChooseFrame *nframe = &list_get(
                                ctx.data.p3choose.ctx.frames,
                                j - 1
                            );
                            if (
                                get(
                                    ctx.data.p3choose.ctx.vertex_info,
                                    nframe->z
                                ).colors.len == 1
                            ) {
                                frame->value = *nframe;
                                frame->valid = true;
                                *nframe = list_get(
                                    ctx.data.p3choose.ctx.frames,
                                    ctx.data.p3choose.ctx.frames.len - 1
                                );
                                ctx.data.p3choose.ctx.frames.len -= 1;
                                break;
                            }
                        }
                    }

                    if (frame->valid) {
                        done = false;
                    }
                }
                // }
                if (done) {
                    int ncases = 0;
                    for (
                        size_t j = 0;
                        j < countof(ctx.data.p3choose.cases_satisfied);
                        j += 1
                    ) {
                        if (ctx.data.p3choose.cases_satisfied[j]) {
                            ncases += 1;
                        }
                    }

                    if (ncases > 8) {
                        while (1);
                    }

                    ctx.state = APP_STATE_COLORED;
                    GraphPropUint8 coloring = { .len = ctx.graph.len };
                    coloring.ptr = aven_arena_create_array(
                        uint8_t,
                        &arena,
                        coloring.len
                    );
                    for (uint32_t v = 0; v < ctx.graph.len; v += 1) {
                        get(coloring, v) = get(
                            get(ctx.data.p3choose.ctx.vertex_info, v).colors,
                            0
                        );
                    }
                    ctx.data.colored.coloring = coloring;
                    ctx.count = 0;
                    ctx.updates = 0;
                }
                break;
            case APP_STATE_COLORED:
                ctx.count += 1;
                if (
                    ctx.paused or
                    ctx.count > 0L * (AVEN_TIME_NSEC_PER_SEC / ctx.timestep)
                ) {
                    ctx.updates = 0;
                    if (
                        graph_path_color_verify(
                            ctx.graph,
                            ctx.data.colored.coloring, arena
                        )
                    ) {
                        if (ctx.autoplay and ctx.radius > 0.011f) {
                            float old_radius = ctx.radius;
                            ctx.radius -= 0.01f;
                            ctx.timestep = (int64_t)(
                                (float)ctx.timestep *
                                (ctx.radius / old_radius) *
                                (ctx.radius / old_radius)
                            );
                        }
                        app_reset();
                    }
                }
                break;
        }

        if (ctx.state == APP_STATE_GEN) {
            timestep = ctx.timestep / 8;
        } else {
            timestep = ctx.timestep;
        }
    }

    if (ctx.updates > SCREEN_UPDATES) {
        return;
    }

    ctx.updates += 1;

    aven_gl_shape_geometry_clear(&ctx.edge_shapes.geometry);
    aven_gl_shape_rounded_geometry_clear(&ctx.vertex_shapes.geometry);
    aven_gl_text_geometry_clear(&ctx.help_text.geometry);
#ifdef SHOW_VERTEX_LABELS
    aven_gl_text_geometry_clear(&ctx.vertex_text.geometry);
#endif

    Aff2 text_transform;
    aff2_identity(text_transform);
    aff2_add_vec2(text_transform, text_transform, (Vec2){ 0.0f, 0.98f });

    AvenArena text_arena = arena;
    AvenGlTextLine hello_line = aven_gl_text_line(
        &ctx.help_text.font,
        aven_str("Pause: <Space>  Step: <D>  Speed: <0-9>  Scale: <+/->  Threads: <T>"),
        &text_arena
    );
    aven_gl_text_geometry_push_line(
        &ctx.help_text.geometry,
        &hello_line,
        text_transform,
        text_scale,
        (Vec4){ 0.0f, 0.0f, 0.0f, 1.0f }
    );

    float border_padding = 1.5f * ctx.radius + 4.0f * pixel_size;
    float scale = 1.0f / (1.0f + border_padding);

    Aff2 graph_transform;
    aff2_identity(graph_transform);
    aff2_stretch(
        graph_transform,
        (Vec2){ scale, scale },
        graph_transform
    );
    aff2_stretch(
        graph_transform,
        (Vec2){ 0.96f, 0.96f },
        graph_transform
    );
    aff2_sub_vec2(graph_transform, graph_transform, (Vec2){ 0.0f, 0.04f });

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
        case APP_STATE_P3CHOOSE: {
            GraphPlaneP3ChooseFrame valid_frame_data[
                countof(ctx.data.p3choose.frames)
            ];
            List(GraphPlaneP3ChooseFrame) valid_frames = list_array(
                valid_frame_data
            );
            for (size_t i = 0; i < countof(ctx.data.p3choose.frames); i += 1) {
                GraphPlaneP3ChooseFrameOptional *frame =
                    &ctx.data.p3choose.frames[i];
                if (frame->valid) {
                    list_push(valid_frames) = frame->value;
                }
            }
            GraphPlaneP3ChooseFrameSlice vf_slice = slice_list(valid_frames);
            graph_plane_p3choose_geometry_push_ctx(
                &ctx.edge_shapes.geometry,
                &ctx.vertex_shapes.geometry,
                ctx.embedding,
                &ctx.data.p3choose.ctx,
                vf_slice,
                graph_transform,
                &ctx.p3choose_geometry_info
            );
            break;
        }
        case APP_STATE_COLORED: {
            GraphPlaneGeometryEdge simple_edge_info = {
                .thickness = ctx.p3choose_geometry_info.edge_thickness,
            };
            vec4_copy(
                simple_edge_info.color,
                ctx.p3choose_geometry_info.uncolored_edge_color
            );
            graph_plane_geometry_push_uncolored_edges(
                &ctx.edge_shapes.geometry,
                ctx.graph,
                ctx.embedding,
                ctx.data.colored.coloring,
                graph_transform,
                &simple_edge_info
            );
            GraphPlaneGeometryEdge colored_edge_data[MAX_COLOR];
            GraphPlaneGeometryEdgeSlice colored_edges = slice_array(
                colored_edge_data
            );
            for (uint32_t i = 0; i < colored_edges.len; i += 1) {
                get(colored_edges, i).thickness =
                    ctx.p3choose_geometry_info.edge_thickness;
                vec4_copy(
                    get(colored_edges, i).color,
                    get(ctx.p3choose_geometry_info.colors, i + 1)
                );
            }
            graph_plane_geometry_push_colored_edges(
                &ctx.edge_shapes.geometry,
                ctx.graph,
                ctx.embedding,
                ctx.data.colored.coloring,
                graph_transform,
                colored_edges
            );

            GraphPlaneGeometryNode vertex_outline = {
                .mat = {
                    { ctx.p3choose_geometry_info.radius, 0.0f },
                    { 0.0f, ctx.p3choose_geometry_info.radius },
                },
                .shape = GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
                .roundness = 1.0f,
            };
            vec4_copy(
                vertex_outline.color,
                ctx.p3choose_geometry_info.outline_color
            );
            graph_plane_geometry_push_vertices(
                &ctx.vertex_shapes.geometry,
                ctx.embedding,
                graph_transform,
                &vertex_outline
            );

            GraphPlaneGeometryNode colored_vertex_data[MAX_COLOR + 1];
            GraphPlaneGeometryNodeSlice colored_vertices = slice_array(
                colored_vertex_data
            );
            float radius = ctx.p3choose_geometry_info.radius -
                ctx.p3choose_geometry_info.border_thickness;
            for (uint32_t i = 0; i < colored_vertices.len; i += 1) {
                get(colored_vertices, i) = (GraphPlaneGeometryNode) {
                    .mat = {
                        { radius, 0.0f, },
                        { 0.0f, radius },
                    },
                    .shape = GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
                    .roundness = 1.0f,
                };
                vec4_copy(
                    get(colored_vertices, i).color,
                    get(ctx.p3choose_geometry_info.colors, i)
                );
            }
            graph_plane_geometry_push_colored_vertices(
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
    gl.ClearColor(0.75f, 0.75f, 0.75f, 1.0f);
    assert(gl.GetError() == 0);
    gl.Clear(GL_COLOR_BUFFER_BIT);
    assert(gl.GetError() == 0);

    // float camera_frac = (float)ctx->camera_time / (float)CAMERA_TIMESTEP;

    Aff2 cam_transform;
    aff2_camera_position(
        cam_transform,
        (Vec2){ 0.0f, 0.0f },
        (Vec2){ norm_width, norm_height },
        0.0f
    );
    //aff2_stretch(cam_transform, (Vec2){ scale, scale }, cam_transform);

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
        &ctx.help_text.buffer,
        &ctx.help_text.geometry
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
    aven_gl_text_geometry_draw(
        &gl,
        &ctx.help_text.ctx,
        &ctx.help_text.buffer,
        &ctx.help_text.font,
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

    if (action == GLFW_RELEASE) {
        if (key == GLFW_KEY_RIGHT or key == GLFW_KEY_D) {
            ctx.step_held = false;
        }
        return;
    }

    if (action != GLFW_PRESS) {
        return;
    }

    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(win, GLFW_TRUE);
    }
    if (key == GLFW_KEY_EQUAL) {
        ctx.autoplay = false;
        if (ctx.radius < 0.12f) {
            ctx.radius += 0.005f;
            app_reset();
        }
    }
    if (key == GLFW_KEY_MINUS) {
        ctx.autoplay = false;
        if (ctx.radius > 0.0101f) {
            ctx.radius -= 0.005f;
            app_reset();
        }
    }
    if (key == GLFW_KEY_RIGHT or key == GLFW_KEY_D) {
        ctx.step = true;
        ctx.step_update = aven_time_now();
        ctx.step_held = true;
    }
    if (key == GLFW_KEY_1) {
        ctx.autoplay = false;
        ctx.timestep = 3 * (AVEN_TIME_NSEC_PER_SEC / 4);
    }
    if (key == GLFW_KEY_2) {
        ctx.autoplay = false;
        ctx.timestep = AVEN_TIME_NSEC_PER_SEC / 3L;
    }
    if (key == GLFW_KEY_3) {
        ctx.autoplay = false;
        ctx.timestep = AVEN_TIME_NSEC_PER_SEC / 6L;
    }
    if (key == GLFW_KEY_4) {
        ctx.autoplay = false;
        ctx.timestep = AVEN_TIME_NSEC_PER_SEC / 8L;
    }
    if (key == GLFW_KEY_5) {
        ctx.autoplay = false;
        ctx.timestep = AVEN_TIME_NSEC_PER_SEC / 10L;
    }
    if (key == GLFW_KEY_6) {
        ctx.autoplay = false;
        ctx.timestep = AVEN_TIME_NSEC_PER_SEC / 12L;
    }
    if (key == GLFW_KEY_7) {
        ctx.autoplay = false;
        ctx.timestep = AVEN_TIME_NSEC_PER_SEC / 15L;
    }
    if (key == GLFW_KEY_8) {
        ctx.autoplay = false;
        ctx.timestep = AVEN_TIME_NSEC_PER_SEC / 30L;
    }
    if (key == GLFW_KEY_9) {
        ctx.autoplay = false;
        ctx.timestep = AVEN_TIME_NSEC_PER_SEC / 60L;
    }
    if (key == GLFW_KEY_0) {
        ctx.autoplay = false;
        ctx.timestep = AVEN_TIME_NSEC_PER_SEC / 120L;
    }
    if (key == GLFW_KEY_SPACE and action == GLFW_PRESS) {
        ctx.paused = !ctx.paused;
    }
    if (key == GLFW_KEY_T) {
        if (ctx.threads > 1) {
            if (ctx.state == APP_STATE_P3CHOOSE) {
                for (size_t i = 1; i < ctx.threads; i += 1) {
                    if (!ctx.data.p3choose.frames[i].valid) {
                        continue;
                    }
                    list_push(ctx.data.p3choose.ctx.frames) =
                        ctx.data.p3choose.frames[i].value;
                    ctx.data.p3choose.frames[i] =
                        (GraphPlaneP3ChooseFrameOptional){ 0 };
                }
            }
            ctx.threads = 1;
        } else {
            ctx.threads = 3;
        }
    }
    if (key == GLFW_KEY_P) {
        ctx.autoplay = true;
    }
}

void on_damage(GLFWwindow *w) {
    (void)w;
    ctx.updates = 0;
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

void on_resize(int width, int height) {
    glfwSetWindowSize(window, width, height);
    ctx.updates = 0;
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
    glfwSetWindowRefreshCallback(window, on_damage);

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
