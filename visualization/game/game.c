#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
    #define _POSIX_C_SOURCE 200112L
#endif

#ifndef AVEN_IMPLEMENTATION
    #define AVEN_IMPLEMENTATION
#endif
#include <aven.h>
#include <aven/arena.h>
#include <aven/gl/shape.h>
#include <aven/math.h>
#include <aven/str.h>
#include <aven/time.h>

#include "../game.h"

#include <graph/plane/gen.h>
#include <graph/plane/p3color_bfs/geometry.h>
#include <graph/plane/p3color/geometry.h>
#include <graph/plane/p3choose/geometry.h>

static Vec4 vertex_colors[3] = {
    { 0.85f, 0.15f, 0.15f, 1.0f },
    { 0.15f, 0.75f, 0.15f, 1.0f },
    { 0.15f, 0.15f, 0.85f, 1.0f },
};

static const float vertex_radii[] = {
    0.15f,
    0.10f,
    0.075f,
    0.05f,
    0.025f,
    0.015f,
    0.01f,
};

static GameInfoAlg game_info_alg_init(size_t alg_arena_size, AvenArena *arena) {
    GameInfoAlg alg = { 0 };
    alg.init_arena = aven_arena_init(
        aven_arena_alloc(arena, alg_arena_size, AVEN_ARENA_BIGGEST_ALIGNMENT, 1),
        alg_arena_size
    );

    return alg;
}

static void game_info_alg_setup(
    GameInfoSession *info_session,
    GameInfoAlg *info_alg,
    GameInfoSessionOpts *session_opts
) {
    info_alg->arena = info_alg->init_arena;
    info_alg->type = session_opts->alg_type;
    info_alg->steps = 0;
    info_alg->done = false;

    switch (info_alg->type) {
        case GAME_DATA_ALG_TYPE_P3COLOR: {
            GameInfoAlgP3Color *alg = &info_alg->data.p3color;

            *alg = (GameInfoAlgP3Color){
                .frames = { .len = session_opts->nthreads },
            };
            alg->frames.ptr = aven_arena_create_array(
                GraphPlaneP3ColorFrameOptional,
                &info_alg->arena,
                alg->frames.len
            );
            for (size_t i = 0; i < alg->frames.len; i += 1) {
                get(alg->frames, i).valid = false;
            }
            alg->ctx = graph_plane_p3color_init(
                info_session->graph,
                info_session->p1,
                info_session->p2,
                &info_alg->arena
            );
            GraphPlaneP3ColorFrame frame = graph_plane_p3color_next_frame(
                &alg->ctx
            ).value;
            while (!graph_plane_p3color_frame_step(&alg->ctx, &frame)) {}
            break;
        }
        case GAME_DATA_ALG_TYPE_P3COLOR_BFS: {
            GameInfoAlgP3ColorBfs *alg = &info_alg->data.p3color_bfs;

            *alg = (GameInfoAlgP3ColorBfs){
                .queues = { .len = session_opts->nthreads },
                .frames = { .len = session_opts->nthreads },
            };
            alg->queues.ptr = aven_arena_create_array(
                GraphPlaneP3ColorBfsQueue,
                &info_alg->arena,
                alg->queues.len
            );
            alg->frames.ptr = aven_arena_create_array(
                GraphPlaneP3ColorBfsFrameOptional,
                &info_alg->arena,
                alg->frames.len
            );
            for (size_t i = 0; i < session_opts->nthreads; i += 1) {
                get(alg->queues, i).cap = info_session->graph.adj.len;
                get(alg->queues, i).ptr = aven_arena_create_array(
                    uint32_t,
                    &info_alg->arena,
                    get(alg->queues, i).cap
                );
                queue_clear(get(alg->queues, i));
                get(alg->frames, i).valid = false;
            }
            alg->ctx = graph_plane_p3color_bfs_init(
                info_session->graph,
                info_session->p1,
                info_session->p2,
                &info_alg->arena
            );
            break;
        }
        case GAME_DATA_ALG_TYPE_P3CHOOSE: {
            GameInfoAlgP3Choose *alg = &info_alg->data.p3choose;

            *alg = (GameInfoAlgP3Choose){
                .frames = { .len = session_opts->nthreads },
            };
            alg->frames.ptr = aven_arena_create_array(
                GraphPlaneP3ChooseFrameOptional,
                &info_alg->arena,
                alg->frames.len
            );
            for (size_t i = 0; i < alg->frames.len; i += 1) {
                get(alg->frames, i).valid = false;
            }
            alg->ctx = graph_plane_p3choose_init(
                info_session->aug_graph,
                info_session->color_lists,
                info_session->outer_cycle,
                &info_alg->arena
            );
            break;
        }
    }
}

static void game_info_alg_step(GameInfoAlg *info_alg) {
    if (info_alg->done) {
        return;
    }

    info_alg->steps += 1;

    switch (info_alg->type) {
        case GAME_DATA_ALG_TYPE_P3COLOR: {
            GameInfoAlgP3Color *alg = &info_alg->data.p3color;
            GraphPlaneP3ColorCtx *ctx = &alg->ctx;

            bool finished = true;
            for (size_t i = 0; i < alg->frames.len; i += 1) {
                GraphPlaneP3ColorFrameOptional *frame = &get(alg->frames, i);
                if (!frame->valid) {
                    *frame = graph_plane_p3color_next_frame(ctx);
                    if (frame->valid) {
                        finished = false;
                    }
                } else {
                    finished = false;

                    bool done = graph_plane_p3color_frame_step(
                        ctx,
                        &frame->value
                    );
                    if (done) {
                        frame->valid = false;
                    }
                }
            }
            if (finished) {
                info_alg->done = true;
            }
            break;
        }
        case GAME_DATA_ALG_TYPE_P3COLOR_BFS: {
            GameInfoAlgP3ColorBfs *alg = &info_alg->data.p3color_bfs;
            GraphPlaneP3ColorBfsCtx *ctx = &alg->ctx;

            bool finished = true;
            for (size_t i = 0; i < alg->frames.len; i += 1) {
                GraphPlaneP3ColorBfsFrameOptional *frame = &get(alg->frames, i);
                if (!frame->valid) {
                    *frame = graph_plane_p3color_bfs_next_frame(ctx);
                    if (frame->valid) {
                        finished = false;
                    }
                } else {
                    finished = false;

                    bool done = graph_plane_p3color_bfs_frame_step(
                        ctx,
                        &frame->value,
                        &get(alg->queues, i)
                    );
                    if (done) {
                        frame->valid = false;
                    }
                }
            }
            if (finished) {
                info_alg->done = true;
            }
            break;
        }
        case GAME_DATA_ALG_TYPE_P3CHOOSE: {
            GameInfoAlgP3Choose *alg = &info_alg->data.p3choose;
            GraphPlaneP3ChooseCtx *ctx = &alg->ctx;

            bool finished = true;
            for (size_t i = 0; i < alg->frames.len; i += 1) {
                GraphPlaneP3ChooseFrameOptional *frame = &get(alg->frames, i);
                if (!frame->valid) {
                    for (size_t j = ctx->frames.len; j > 0; j -= 1) {
                        GraphPlaneP3ChooseFrame *nframe = &get(
                            ctx->frames,
                            j - 1
                        );
                        if (get(ctx->vertex_info, nframe->z).colors.len == 1) {
                            frame->valid = true;
                            frame->value = *nframe;
                            *nframe = get(ctx->frames, ctx->frames.len - 1);
                            ctx->frames.len -= 1;
                            break;
                        }
                    }
                    if (frame->valid) {
                        finished = false;
                    }
                } else {
                    finished = false;

                    bool done = graph_plane_p3choose_frame_step(
                        ctx,
                        &frame->value
                    );
                    if (done) {
                        frame->valid = false;
                    }
                }
            }
            if (finished) {
                info_alg->done = true;
            }
            break;
        }
    }
}

static GameInfo game_info_init(size_t info_arena_size, AvenArena *arena) {
    GameInfo info = { 0 };
    info.init_arena = aven_arena_init(
        aven_arena_alloc(
            arena,
            info_arena_size,
            AVEN_ARENA_BIGGEST_ALIGNMENT,
            1
        ),
        info_arena_size
    );

    return info;
}

static GameInfoSession game_info_session_init(
    GameInfoSessionOpts *session_opts,
    AvenRng rng,
    AvenArena *arena
) {
    GameInfoSession session = { 0 };

    float radius = vertex_radii[
        min(session_opts->radius, countof(vertex_radii))
    ];
    (void)radius;

    switch (session_opts->graph_type) {
        case GAME_INFO_GRAPH_TYPE_PYRAMID: {
            Aff2 identity;
            aff2_identity(identity);
            GraphPlaneGenData gen_data = graph_plane_gen_pyramid(
                33,
                identity,
                5.0f * (radius * radius) + 0.35f * radius,
                arena
            );

            session.graph = gen_data.graph;
            session.embedding = gen_data.embedding;
            session.aug_graph = graph_aug(session.graph, arena);

            GraphSubset p1 = aven_arena_create_slice(uint32_t, arena, 2);
            get(p1, 0) = 1;
            get(p1, 1) = 2;
            session.p1 = p1;

            GraphSubset p2 = aven_arena_create_slice(uint32_t, arena, 1);
            get(p2, 0) = 0;
            session.p2 = p2;

            GraphSubset outer_face = aven_arena_create_slice(uint32_t, arena, 3);
            for (uint32_t v = 0; v < 3; v += 1) {
                get(outer_face, v) = v;
            }
            session.outer_cycle = outer_face;
            break;
        }
        case GAME_INFO_GRAPH_TYPE_RAND: {
            Aff2 identity;
            aff2_identity(identity);
            GraphPlaneGenData gen_data = graph_plane_gen_triangulation(
                GAME_MAX_VERTICES,
                identity,
                1.8f * (radius * radius),
                GAME_MIN_COEFF,
                true,
                rng,
                arena
            );

            session.graph = gen_data.graph;
            session.embedding = gen_data.embedding;
            session.aug_graph = graph_aug(session.graph, arena);

            uint32_t v1 = aven_rng_rand_bounded(rng, 4);
            uint32_t p1_len = 1 + aven_rng_rand_bounded(rng, 3);

            List(uint32_t) p1_list = aven_arena_create_list(uint32_t, arena, 4);
            for (uint32_t i = 0; i < p1_len; i += 1) {
                list_push(p1_list) = (v1 + i) % 4;
            }
            GraphSubset p1 = slice_list(p1_list);
            session.p1 = p1;

            List(uint32_t) p2_list = aven_arena_create_list(uint32_t, arena, 4);
            for (uint32_t i = 0; i < 4 - p1_len; i += 1) {
                list_push(p2_list) = (v1 + 3 - i) % 4;
            }
            GraphSubset p2 = slice_list(p2_list);
            session.p2 = p2;

            GraphSubset outer_face = aven_arena_create_slice(uint32_t, arena, 4);
            for (uint32_t i = 0; i < 4; i += 1) {
                get(outer_face, i) = (v1 + i) % 4;
            }
            session.outer_cycle = outer_face;
            break;
        }
    }

    uint32_t color_divisions = GAME_COLOR_DIVISIONS;
    uint32_t max_color = ((color_divisions + 2) * (color_divisions + 1)) / 2U;
    assert(max_color < 256);

    session.color_lists.len = session.graph.adj.len;
    session.color_lists.ptr = aven_arena_create_array(
        GraphPlaneP3ChooseList,
        arena,
        session.color_lists.len
    );

    for (uint32_t v = 0; v < session.color_lists.len; v += 1) {
        GraphPlaneP3ChooseList list = { .len = 3 };
        get(list, 0) = (uint8_t)(1 + aven_rng_rand_bounded(rng, max_color));
        get(list, 1) = (uint8_t)(1 + aven_rng_rand_bounded(rng, max_color));
        get(list, 2) = (uint8_t)(1 + aven_rng_rand_bounded(rng, max_color));

        while (get(list, 1) == get(list, 0)) {
            get(list, 1) = (uint8_t)(1 + aven_rng_rand_bounded(rng, max_color));
        }
        while (get(list, 2) == get(list, 1) or get(list, 2) == get(list, 0)) {
            get(list, 2) = (uint8_t)(1 + aven_rng_rand_bounded(rng, max_color));
        }

        get(session.color_lists, v) = list;
    }

    session.colors.len = max_color + 1;
    session.colors.ptr = aven_arena_create_array(
        Vec4,
        arena,
        session.colors.len
    );

    vec4_copy(get(session.colors, 0), (Vec4){ 0.9f, 0.9f, 0.9f, 1.0f });

    size_t color_index = 1;
    for (uint8_t i = 0; i <= color_divisions; i += 1) {
        for (uint8_t j = i; j <= color_divisions; j += 1) {
            float coeffs[3] = {
                (float)(i) / (float)color_divisions,
                (float)(j - i) / (float)color_divisions,
                (float)(color_divisions - j) / (float)color_divisions,
            };

            vec4_copy(get(session.colors, color_index), (Vec4){ 0 });
            for (size_t k = 0; k < 3; k += 1) {
                Vec4 scolor;
                vec4_scale(scolor, coeffs[k], vertex_colors[k]);
                vec4_add(
                    get(session.colors, color_index),
                    get(session.colors, color_index),
                    scolor
                );
            }
            color_index += 1;
        }
    }

    return session;
}

static void game_info_setup(
    GameInfo *info,
    GameInfoSessionOpts *session_opts,
    size_t alg_arena_size,
    AvenRngPcg pcg
) {
    info->arena = info->init_arena;
    info->pcg = pcg;

    info->alg = game_info_alg_init(alg_arena_size, &info->arena);

    info->session = game_info_session_init(
        session_opts,
        aven_rng_pcg(&info->pcg),
        &info->arena
    );
}

#if !defined(HOT_RELOAD)
    static
#elif defined(_MSC_VER)
    __declspec(dllexport)
#endif
const GameTable game_table = {
    .init = game_init,
    .load = game_load,
    .unload = game_unload,
    .update = game_update,
    .damage = game_damage,
    .deinit = game_deinit,
    .mouse_click = game_mouse_click,
    .mouse_move = game_mouse_move,
};

void game_load(GameCtx *ctx, AvenGl *gl) {
    ctx->arena = ctx->init_arena;

    ctx->shapes.ctx = aven_gl_shape_ctx_init(gl);
    ctx->shapes.geometry = aven_gl_shape_geometry_init(
        GAME_GEOMETRY_VERTICES,
        (GAME_GEOMETRY_VERTICES * 6) / 4,
        &ctx->arena
    );
    ctx->shapes.buffer = aven_gl_shape_buffer_init(
        gl,
        &ctx->shapes.geometry,
        AVEN_GL_BUFFER_USAGE_DYNAMIC
    );

    ctx->rounded_shapes.ctx = aven_gl_shape_rounded_ctx_init(gl);
    ctx->rounded_shapes.geometry = aven_gl_shape_rounded_geometry_init(
        GAME_ROUNDED_GEOMETRY_VERTICES,
        (GAME_ROUNDED_GEOMETRY_VERTICES * 6) / 4,
        &ctx->arena
    );
    ctx->rounded_shapes.buffer = aven_gl_shape_rounded_buffer_init(
        gl,
        &ctx->rounded_shapes.geometry,
        AVEN_GL_BUFFER_USAGE_DYNAMIC
    );

#ifdef TEXTURE_OPTIMIZATION
    {
        AvenArena temp_arena = ctx->arena;
        ctx->graph_texture.ctx = aven_gl_texture_ctx_init(
            gl,
            (size_t)ctx->width,
            (size_t)ctx->height,
            (AvenGlTextureBytesOptional){ 0 }
        );
        AvenGlTextureGeometry texture_geometry = aven_gl_texture_geometry_init(
            1,
            &temp_arena
        );
        Aff2 ident;
        aff2_identity(ident);
        aven_gl_texture_geometry_push_square(&texture_geometry, ident, ident);
        ctx->graph_texture.buffer = aven_gl_texture_buffer_init(
            gl,
            &texture_geometry,
            AVEN_GL_BUFFER_USAGE_STATIC
        );
    }
#endif // TEXTURE_OPTIMIZATION

    {
        AvenGlUiColors window_colors = {
            .background = { 0.4f, 0.4f, 0.4f, 1.0f },
            .primary = { 0.15f, 0.25f, 0.05f, 1.0f },
            .secondary = { 0.25f, 0.05f, 0.15f, 1.0f },
        };
        AvenGlUiColors base_colors = {
            .background = { 0.55f, 0.55f, 0.55f, 1.0f },
            .primary = { 0.15f, 0.25f, 0.05f, 1.0f },
            .secondary = { 0.25f, 0.05f, 0.15f, 1.0f },
        };
        AvenGlUiColors hovered_colors = {
            .background = { 0.575f, 0.575f, 0.575f, 1.0f },
            .primary = { 0.175f, 0.275f, 0.075f, 1.0f },
            .secondary = { 0.275f, 0.075f, 0.175f, 1.0f },
        };
        AvenGlUiColors clicked_colors = base_colors;
        AvenGlUiColors disabled_colors = {
            .background = { 0.55f, 0.55f, 0.55f, 1.0f },
            .primary = { 0.175f, 0.175f, 0.175f, 1.0f },
            .secondary = { 0.275f, 0.275f, 0.275f, 1.0f },
        };
        ctx->ui = aven_gl_ui_init(
            gl,
            128,
            window_colors,
            base_colors,
            hovered_colors,
            clicked_colors,
            disabled_colors,
            &ctx->arena
        );
    }

    ctx->graph_up_to_date = false;
    ctx->ui_up_to_date = false;
    ctx->screen_updates = 0;
}

void game_unload(GameCtx *ctx, AvenGl *gl) {
    aven_gl_ui_deinit(gl, &ctx->ui);

#ifdef TEXTURE_OPTIMIZATION
    aven_gl_texture_buffer_deinit(gl, &ctx->graph_texture.buffer);
    aven_gl_texture_ctx_deinit(gl, &ctx->graph_texture.ctx);
    ctx->graph_texture = (GameGraphTexture){ 0 };
#endif // TEXTURE_OPTIMIZATION

    aven_gl_shape_rounded_buffer_deinit(gl, &ctx->rounded_shapes.buffer);
    aven_gl_shape_rounded_ctx_deinit(gl, &ctx->rounded_shapes.ctx);
    ctx->rounded_shapes = (GameRoundedShapes){ 0 };

    aven_gl_shape_buffer_deinit(gl, &ctx->shapes.buffer);
    aven_gl_shape_ctx_deinit(gl, &ctx->shapes.ctx);
    ctx->shapes = (GameShapes){ 0 };
}

GameCtx game_init(AvenGl *gl, AvenArena *arena) {
    GameCtx ctx = {
        .width = GAME_INIT_WIDTH,
        .height = GAME_INIT_HEIGHT,
        .session_opts = {
            .alg_type = GAME_DATA_ALG_TYPE_P3COLOR_BFS,
            .graph_type = GAME_INFO_GRAPH_TYPE_RAND,
            .nthreads = 1,
            .radius = 0,
        },
        .alg_opts = { .time_step = GAME_TIME_STEP },
    };

    ctx.init_arena = aven_arena_init(
        aven_arena_alloc(
            arena,
            GAME_GL_ARENA_SIZE,
            AVEN_ARENA_BIGGEST_ALIGNMENT,
            1
        ),
        GAME_GL_ARENA_SIZE
    );

    game_load(&ctx, gl);

    ctx.info = game_info_init(GAME_LEVEL_ARENA_SIZE, arena);

    ctx.last_update = aven_time_now();

    AvenTimeInst now = aven_time_now();
    ctx.pcg = aven_rng_pcg_seed((uint64_t)now.tv_nsec, (uint64_t)now.tv_sec);

    game_info_setup(&ctx.info, &ctx.session_opts, GAME_ALG_ARENA_SIZE, ctx.pcg);
    game_info_alg_setup(&ctx.info.session, &ctx.info.alg, &ctx.session_opts);

    return ctx;
}

void game_deinit(GameCtx *ctx, AvenGl *gl) {
    game_unload(ctx, gl);
    *ctx = (GameCtx){ 0 };
}

void game_mouse_move(GameCtx *ctx, Vec2 pos) {
    ctx->screen_updates = 0;

    float side = (float)ctx->height;
    if (ctx->height > ctx->width) {
        side = (float)ctx->width;
    }
    Aff2 screen_trans;
    aff2_camera_position(
        screen_trans,
        (Vec2){ (float)ctx->width / 2.0f, (float)ctx->height / 2.0f },
        (Vec2){ side / 2.0f, -side / 2.0f }
    );

    aff2_transform(pos, screen_trans, pos);

    aven_gl_ui_mouse_move(&ctx->ui, pos);
}

void game_mouse_click(GameCtx *ctx, AvenGlUiMouseEvent event) {
    ctx->screen_updates = 0;
    aven_gl_ui_mouse_click(&ctx->ui, event);
}

bool game_update(
    GameCtx *ctx,
    AvenGl *gl,
    int width,
    int height,
    AvenArena arena
) {
    (void)arena;

    ctx->width = width;
    ctx->height = height;

    AvenTimeInst now = aven_time_now();
    int64_t ns_since_update = aven_time_since(now, ctx->last_update);
    ctx->elapsed += ns_since_update;
    ctx->last_update = now;

    float screen_ratio = (float)width / (float)height;

    float norm_height = 1.0f;
    float norm_width = screen_ratio;
    float pixel_size = 2.0f / (float)height;
    float free_space = norm_width - norm_height;

    if (screen_ratio < 1.0f) {
        norm_height = 1.0f / screen_ratio;
        norm_width = 1.0f;
        screen_ratio = 1.0f / screen_ratio;
        pixel_size = 2.0f / (float)width;
        free_space = norm_height - norm_width;
    }

    if (!ctx->alg_opts.playing) {
        while (ctx->elapsed >= ctx->alg_opts.time_step) {
            ctx->elapsed -= ctx->alg_opts.time_step;
            ctx->preview.edge_index += 1;
            if (ctx->preview.edge_index == GAME_PREVIEW_EDGES) {
                ctx->preview.edge_index = 0;
            }

            if (ctx->active_window == GAME_UI_WINDOW_PREVIEW) {
                ctx->screen_updates = 0;
                ctx->ui_up_to_date = false;
            }
        }
    } else {
        ctx->active_window = GAME_UI_WINDOW_NONE;
        ctx->preview.edge_index = 0;

        while (ctx->elapsed >= ctx->alg_opts.time_step) {
            ctx->elapsed -= ctx->alg_opts.time_step;
            ctx->screen_updates = 0;
            ctx->graph_up_to_date = false;

            game_info_alg_step(&ctx->info.alg);
            if (ctx->info.alg.done) {
                ctx->alg_opts.playing = false;
            }
        }
    }

    if (ctx->screen_updates >= GAME_SCREEN_UPDATES) {
        ctx->ui_up_to_date = true;
        return false;
    }

    ctx->screen_updates += 1;

    float padding = 0.02f;
    float ui_width = (2.0f - 2.0f * padding) / 8.0f;
    float ui_window_width = ui_width + padding;
    float graph_offset = ui_window_width / 2.0f;
    float ui_offset = -(2.0f + 2.0f * free_space - ui_window_width) / 2.0f;
    float graph_scale = min(2.0f, 2.0f + free_space - ui_window_width) / 2.0f;

    AvenGlUiBorder popup_border = { true, true, false, true };

    Aff2 cam_trans;
    aff2_camera_position(
        cam_trans,
        (Vec2){ 0.0f, 0.0f },
        (Vec2){ norm_width, norm_height }
    );

    // Generate UI geometry and evaluate UI logic

    AvenGlUiId last_hot = ctx->ui.hot_id;
    AvenGlUiId last_active = ctx->ui.active_id;
    GameUiWindow last_window = ctx->active_window;

    float draw_angle = 0.0f;
    if (norm_width < norm_height) {
        draw_angle = AVEN_MATH_PI_F / 2.0f;
    }

    Aff2 ui_trans;
    aff2_position(ui_trans, (Vec2){ ui_offset, 0.0f }, (Vec2){ 1.0f, 1.0f });
    aff2_rotate(ui_trans, ui_trans, draw_angle);

    {
        Aff2 backdrop_trans;
        aff2_position(
            backdrop_trans,
            (Vec2){ 0.0f, 0.0f },
            (Vec2){ (ui_window_width + padding) / 2.0f, 1.0f }
        );
        aff2_compose(backdrop_trans, ui_trans, backdrop_trans);
        if (aven_gl_ui_window(&ctx->ui, backdrop_trans, AVEN_GL_UI_BORDER_ALL)) {
            ctx->active_window = GAME_UI_WINDOW_NONE;
        }
    }

    float top_y = 1.0f - padding - ui_width / 2.0f;

    float button_count = 0;
    if (ctx->active_window == GAME_UI_WINDOW_ALG) {
        float window_left_x = 1.5f * padding + ui_width / 2.0f;
        float window_y = top_y - (button_count * ui_width);
        float window_buttons = 3.0f;

        Aff2 window_trans;
        aff2_position(
            window_trans,
            (Vec2){ window_left_x + window_buttons * ui_width / 2.0f, window_y },
            (Vec2){
                (window_buttons / 2.0f) * ui_width + padding,
                ui_width / 2.0f + padding,
            }
        );
        aff2_compose(window_trans, ui_trans, window_trans);
        aven_gl_ui_window(&ctx->ui, window_trans, popup_border);

        float window_button_count = 0.0f;
        {
            Aff2 button_trans;
            aff2_position_rangle(
                button_trans,
                (Vec2){
                    window_left_x +
                    ui_width / 2.0f +
                    window_button_count * ui_width,
                    window_y,
                },
                (Vec2){ ui_width / 2.15f, ui_width / 2.15f },
                -draw_angle
            );
            aff2_compose(button_trans, ui_trans, button_trans);
            if (
                aven_gl_ui_button(
                    &ctx->ui,
                    button_trans,
                    AVEN_GL_UI_BUTTON_TYPE_CIRCLE_TREE
                )
            ) {
                ctx->active_window = GAME_UI_WINDOW_NONE;
                ctx->session_opts.alg_type = GAME_DATA_ALG_TYPE_P3COLOR_BFS;
                size_t steps = ctx->info.alg.steps;
                game_info_alg_setup(
                    &ctx->info.session,
                    &ctx->info.alg,
                    &ctx->session_opts
                );
                for (size_t i = 0; i < steps; i += 1) {
                    game_info_alg_step(&ctx->info.alg);
                    if (ctx->info.alg.done) {
                        break;
                    }
                }
                ctx->graph_up_to_date = false;
            }
            window_button_count += 1;
        }
        {
            Aff2 button_trans;
            aff2_position_rangle(
                button_trans,
                (Vec2){
                    window_left_x +
                    ui_width / 2.0f +
                    window_button_count * ui_width,
                    window_y,
                },
                (Vec2){ ui_width / 2.15f, ui_width / 2.15f },
                -draw_angle
            );
            aff2_compose(button_trans, ui_trans, button_trans);
            if (
                aven_gl_ui_button(
                    &ctx->ui,
                    button_trans,
                    AVEN_GL_UI_BUTTON_TYPE_CIRCLE_PATHS
                )
            ) {
                ctx->active_window = GAME_UI_WINDOW_NONE;
                ctx->session_opts.alg_type = GAME_DATA_ALG_TYPE_P3COLOR;
                size_t steps = ctx->info.alg.steps;
                game_info_alg_setup(
                    &ctx->info.session,
                    &ctx->info.alg,
                    &ctx->session_opts
                );
                for (size_t i = 0; i < steps; i += 1) {
                    game_info_alg_step(&ctx->info.alg);
                    if (ctx->info.alg.done) {
                        break;
                    }
                }
                ctx->graph_up_to_date = false;
            }
            window_button_count += 1;
        }
        {
            Aff2 button_trans;
            aff2_position_rangle(
                button_trans,
                (Vec2){
                    window_left_x +
                    ui_width / 2.0f +
                    window_button_count * ui_width,
                    window_y,
                },
                (Vec2){ ui_width / 2.15f, ui_width / 2.15f },
                -draw_angle
            );
            aff2_compose(button_trans, ui_trans, button_trans);
            if (
                aven_gl_ui_button(
                    &ctx->ui,
                    button_trans,
                    AVEN_GL_UI_BUTTON_TYPE_PIE
                )
            ) {
                ctx->active_window = GAME_UI_WINDOW_NONE;
                ctx->session_opts.alg_type = GAME_DATA_ALG_TYPE_P3CHOOSE;
                size_t steps = ctx->info.alg.steps;
                game_info_alg_setup(
                    &ctx->info.session,
                    &ctx->info.alg,
                    &ctx->session_opts
                );
                for (size_t i = 0; i < steps; i += 1) {
                    game_info_alg_step(&ctx->info.alg);
                    if (ctx->info.alg.done) {
                        break;
                    }
                }
                ctx->graph_up_to_date = false;
            }
            window_button_count += 1;
        }
    }
    {
        Aff2 button_trans;
        aff2_position_rangle(
            button_trans,
            (Vec2){ 0.0f, top_y - (button_count * ui_width) },
            (Vec2){ ui_width / 2.15f, ui_width / 2.15f },
            -draw_angle
        );
        aff2_compose(button_trans, ui_trans, button_trans);
        bool enabled = !ctx->alg_opts.playing and
            ctx->active_window != GAME_UI_WINDOW_ALG;
        switch (ctx->session_opts.alg_type) {
            case GAME_DATA_ALG_TYPE_P3COLOR: {
                if (
                    aven_gl_ui_button_maybe(
                        &ctx->ui,
                        button_trans,
                        AVEN_GL_UI_BUTTON_TYPE_CIRCLE_PATHS,
                        enabled
                    )
                ) {
                    ctx->active_window = GAME_UI_WINDOW_ALG;
                }
                break;
            }
            case GAME_DATA_ALG_TYPE_P3COLOR_BFS: {
                if (
                    aven_gl_ui_button_maybe(
                        &ctx->ui,
                        button_trans,
                        AVEN_GL_UI_BUTTON_TYPE_CIRCLE_TREE,
                        enabled
                    )
                ) {
                    ctx->active_window = GAME_UI_WINDOW_ALG;
                }
                break;
            }
            case GAME_DATA_ALG_TYPE_P3CHOOSE: {
                if (
                    aven_gl_ui_button_maybe(
                        &ctx->ui,
                        button_trans,
                        AVEN_GL_UI_BUTTON_TYPE_PIE,
                        enabled
                    )
                ) {
                    ctx->active_window = GAME_UI_WINDOW_ALG;
                }
                break;
            }
        }
        button_count += 1;
    }

    if (ctx->active_window == GAME_UI_WINDOW_THREAD) {
        float window_left_x = 1.5f * padding + ui_width / 2.0f;
        float window_y = top_y - (button_count * ui_width);
        float window_buttons = 4.0f;

        Aff2 window_trans;
        aff2_position(
            window_trans,
            (Vec2){ window_left_x + window_buttons * ui_width / 2.0f, window_y },
            (Vec2){
                (window_buttons / 2.0f) * ui_width + padding,
                ui_width / 2.0f + padding,
            }
        );
        aff2_compose(window_trans, ui_trans, window_trans);
        aven_gl_ui_window(&ctx->ui, window_trans, popup_border);

        float window_button_count = 0.0f;
        {
            Aff2 button_trans;
            aff2_position_rangle(
                button_trans,
                (Vec2){
                    window_left_x +
                    ui_width / 2.0f +
                    window_button_count * ui_width,
                    window_y,
                },
                (Vec2){ ui_width / 2.15f, ui_width / 2.15f },
                -draw_angle
            );
            aff2_compose(button_trans, ui_trans, button_trans);
            if (
                aven_gl_ui_button(
                    &ctx->ui,
                    button_trans,
                    AVEN_GL_UI_BUTTON_TYPE_THREAD
                )
            ) {
                ctx->active_window = GAME_UI_WINDOW_NONE;
                ctx->session_opts.nthreads = 1;
                size_t steps = ctx->info.alg.steps;
                game_info_alg_setup(
                    &ctx->info.session,
                    &ctx->info.alg,
                    &ctx->session_opts
                );
                for (size_t i = 0; i < steps; i += 1) {
                    game_info_alg_step(&ctx->info.alg);
                    if (ctx->info.alg.done) {
                        break;
                    }
                }
                ctx->graph_up_to_date = false;
            }
            window_button_count += 1;
        }
        {
            Aff2 button_trans;
            aff2_position_rangle(
                button_trans,
                (Vec2){
                    window_left_x +
                    ui_width / 2.0f +
                    window_button_count * ui_width,
                    window_y,
                },
                (Vec2){ ui_width / 2.15f, ui_width / 2.15f },
                -draw_angle
            );
            aff2_compose(button_trans, ui_trans, button_trans);
            if (
                aven_gl_ui_button(
                    &ctx->ui,
                    button_trans,
                    AVEN_GL_UI_BUTTON_TYPE_DOUBLE_THREAD
                )
            ) {
                ctx->active_window = GAME_UI_WINDOW_NONE;
                ctx->session_opts.nthreads = 2;
                size_t steps = ctx->info.alg.steps;
                game_info_alg_setup(
                    &ctx->info.session,
                    &ctx->info.alg,
                    &ctx->session_opts
                );
                for (size_t i = 0; i < steps; i += 1) {
                    game_info_alg_step(&ctx->info.alg);
                    if (ctx->info.alg.done) {
                        break;
                    }
                }
                ctx->graph_up_to_date = false;
            }
            window_button_count += 1;
        }
        {
            Aff2 button_trans;
            aff2_position_rangle(
                button_trans,
                (Vec2){
                    window_left_x +
                    ui_width / 2.0f +
                    window_button_count * ui_width,
                    window_y,
                },
                (Vec2){ ui_width / 2.15f, ui_width / 2.15f },
                -draw_angle
            );
            aff2_compose(button_trans, ui_trans, button_trans);
            if (
                aven_gl_ui_button(
                    &ctx->ui,
                    button_trans,
                    AVEN_GL_UI_BUTTON_TYPE_TRIPLE_THREAD
                )
            ) {
                ctx->active_window = GAME_UI_WINDOW_NONE;
                ctx->session_opts.nthreads = 3;
                size_t steps = ctx->info.alg.steps;
                game_info_alg_setup(
                    &ctx->info.session,
                    &ctx->info.alg,
                    &ctx->session_opts
                );
                for (size_t i = 0; i < steps; i += 1) {
                    game_info_alg_step(&ctx->info.alg);
                    if (ctx->info.alg.done) {
                        break;
                    }
                }
                ctx->graph_up_to_date = false;
            }
            window_button_count += 1;
        }
        {
            Aff2 button_trans;
            aff2_position_rangle(
                button_trans,
                (Vec2){
                    window_left_x +
                    ui_width / 2.0f +
                    window_button_count * ui_width,
                    window_y,
                },
                (Vec2){ ui_width / 2.15f, ui_width / 2.15f },
                -draw_angle
            );
            aff2_compose(button_trans, ui_trans, button_trans);
            if (
                aven_gl_ui_button(
                    &ctx->ui,
                    button_trans,
                    AVEN_GL_UI_BUTTON_TYPE_QUADRA_THREAD
                )
            ) {
                ctx->active_window = GAME_UI_WINDOW_NONE;
                ctx->session_opts.nthreads = 4;
                size_t steps = ctx->info.alg.steps;
                game_info_alg_setup(
                    &ctx->info.session,
                    &ctx->info.alg,
                    &ctx->session_opts
                );
                for (size_t i = 0; i < steps; i += 1) {
                    game_info_alg_step(&ctx->info.alg);
                    if (ctx->info.alg.done) {
                        break;
                    }
                }
                ctx->graph_up_to_date = false;
            }
            window_button_count += 1;
        }
    }
    {
        Aff2 button_trans;
        aff2_position_rangle(
            button_trans,
            (Vec2){ 0.0f, top_y - (button_count * ui_width) },
            (Vec2){ ui_width / 2.15f, ui_width / 2.15f },
            -draw_angle
        );
        aff2_compose(button_trans, ui_trans, button_trans);
        bool enabled = !ctx->alg_opts.playing and
            ctx->active_window != GAME_UI_WINDOW_THREAD;
        switch (ctx->session_opts.nthreads) {
            case 1: {
                if (
                    aven_gl_ui_button_maybe(
                        &ctx->ui,
                        button_trans,
                        AVEN_GL_UI_BUTTON_TYPE_THREAD,
                        enabled
                    )
                ) {
                    ctx->active_window = GAME_UI_WINDOW_THREAD;
                }
                break;
            }
            case 2: {
                if (
                    aven_gl_ui_button_maybe(
                        &ctx->ui,
                        button_trans,
                        AVEN_GL_UI_BUTTON_TYPE_DOUBLE_THREAD,
                        enabled
                    )
                ) {
                    ctx->active_window = GAME_UI_WINDOW_THREAD;
                }
                break;
            }
            case 3: {
                if (
                    aven_gl_ui_button_maybe(
                        &ctx->ui,
                        button_trans,
                        AVEN_GL_UI_BUTTON_TYPE_TRIPLE_THREAD,
                        enabled
                    )
                ) {
                    ctx->active_window = GAME_UI_WINDOW_THREAD;
                }
                break;
            }
            case 4: {
                if (
                    aven_gl_ui_button_maybe(
                        &ctx->ui,
                        button_trans,
                        AVEN_GL_UI_BUTTON_TYPE_QUADRA_THREAD,
                        enabled
                    )
                ) {
                    ctx->active_window = GAME_UI_WINDOW_THREAD;
                }
                break;
            }
            case 5: {
                assert(false);
            }
        }
        button_count += 1;
    }

    if (ctx->active_window == GAME_UI_WINDOW_RADIUS) {
        float window_left_x = 1.5f * padding + ui_width / 2.0f;
        float window_y = top_y - (button_count * ui_width);
        float window_buttons = (float)countof(vertex_radii);

        Aff2 window_trans;
        aff2_position(
            window_trans,
            (Vec2){ window_left_x + window_buttons * ui_width / 2.0f, window_y },
            (Vec2){
                (window_buttons / 2.0f) * ui_width + padding,
                ui_width / 2.0f + padding,
            }
        );
        aff2_compose(window_trans, ui_trans, window_trans);
        aven_gl_ui_window(&ctx->ui, window_trans, popup_border);

        {
            size_t nradii = countof(vertex_radii);
            for (
                size_t radius_count = 0;
                radius_count < nradii;
                radius_count += 1
            ) {
                Aff2 button_trans;
                aff2_position_rangle(
                    button_trans,
                    (Vec2){
                        window_left_x +
                        ui_width / 2.0f +
                        (float)radius_count * ui_width,
                        window_y,
                    },
                    (Vec2){ ui_width / 2.15f, ui_width / 2.15f },
                    -draw_angle
                );
                aff2_compose(button_trans, ui_trans, button_trans);
                if (
                    aven_gl_ui_button_uid(
                        &ctx->ui,
                        button_trans,
                        AVEN_GL_UI_BUTTON_TYPE_NONE,
                        radius_count
                    )
                ) {
                    ctx->active_window = GAME_UI_WINDOW_NONE;
                    ctx->session_opts.radius = radius_count;
                    game_info_setup(
                        &ctx->info,
                        &ctx->session_opts,
                        GAME_ALG_ARENA_SIZE,
                        ctx->pcg
                    );
                    game_info_alg_setup(
                        &ctx->info.session,
                        &ctx->info.alg,
                        &ctx->session_opts
                    );
                    ctx->graph_up_to_date = false;
                }
                Vec2 dim;
                vec2_lerp(
                    dim,
                    (Vec2){ 0.8f, 0.8f },
                    (Vec2){ 0.1f, 0.1f },
                    (float)radius_count / (float)nradii
                );
                Aff2 circle_trans;
                aff2_position(circle_trans, (Vec2){ 0.0f, 0.0f }, dim);
                aff2_compose(circle_trans, button_trans, circle_trans);
                aven_gl_shape_rounded_geometry_push_square(
                    &ctx->ui.shape.geometry,
                    circle_trans,
                    1.0f,
                    ctx->ui.base_colors.primary
                );
            }
        }
    }
    {
        Aff2 button_trans;
        aff2_position_rangle(
            button_trans,
            (Vec2){ 0.0f, top_y - (button_count * ui_width) },
            (Vec2){ ui_width / 2.15f, ui_width / 2.15f },
            -draw_angle
        );
        aff2_compose(button_trans, ui_trans, button_trans);
        bool enabled = !ctx->alg_opts.playing and
            ctx->active_window != GAME_UI_WINDOW_RADIUS;
        if (
            aven_gl_ui_button_maybe(
                &ctx->ui,
                button_trans,
                AVEN_GL_UI_BUTTON_TYPE_NONE,
                enabled
            )
        ) {
            ctx->active_window = GAME_UI_WINDOW_RADIUS;
        }

        size_t nradii = countof(vertex_radii);
        Vec2 dim;
        vec2_lerp(
            dim,
            (Vec2){ 0.8f, 0.8f },
            (Vec2){ 0.1f, 0.1f },
            (float)ctx->session_opts.radius / (float)nradii
        );
        Aff2 circle_trans;
        aff2_position(circle_trans, (Vec2){ 0.0f, 0.0f }, dim);
        aff2_compose(circle_trans, button_trans, circle_trans);
        aven_gl_shape_rounded_geometry_push_square(
            &ctx->ui.shape.geometry,
            circle_trans,
            1.0f,
            enabled ?
                ctx->ui.base_colors.primary :
                ctx->ui.disabled_colors.primary
        );

        button_count += 1;
    }

    if (ctx->active_window == GAME_UI_WINDOW_GRAPH) {
        float window_left_x = 1.5f * padding + ui_width / 2.0f;
        float window_y = top_y - (button_count * ui_width);
        float window_buttons = 2.0f;

        Aff2 window_trans;
        aff2_position(
            window_trans,
            (Vec2){ window_left_x + window_buttons * ui_width / 2.0f, window_y },
            (Vec2){
                (window_buttons / 2.0f) * ui_width + padding,
                ui_width / 2.0f + padding,
            }
        );
        aff2_compose(window_trans, ui_trans, window_trans);
        aven_gl_ui_window(&ctx->ui, window_trans, popup_border);

        float window_button_count = 0.0f;
        {
            Aff2 button_trans;
            aff2_position_rangle(
                button_trans,
                (Vec2){
                    window_left_x +
                    ui_width / 2.0f +
                    window_button_count * ui_width,
                    window_y,
                },
                (Vec2){ ui_width / 2.15f, ui_width / 2.15f },
                -draw_angle
            );
            aff2_compose(button_trans, ui_trans, button_trans);
            if (
                aven_gl_ui_button(
                    &ctx->ui,
                    button_trans,
                    AVEN_GL_UI_BUTTON_TYPE_DICE
                )
            ) {
                ctx->pcg = ctx->info.pcg;
                ctx->active_window = GAME_UI_WINDOW_NONE;
                ctx->session_opts.graph_type = GAME_INFO_GRAPH_TYPE_RAND;
                game_info_setup(
                    &ctx->info,
                    &ctx->session_opts,
                    GAME_ALG_ARENA_SIZE,
                    ctx->pcg
                );
                game_info_alg_setup(
                    &ctx->info.session,
                    &ctx->info.alg,
                    &ctx->session_opts
                );
                ctx->graph_up_to_date = false;
            }
            window_button_count += 1;
        }
        {
            Aff2 button_trans;
            aff2_position_rangle(
                button_trans,
                (Vec2){
                    window_left_x +
                    ui_width / 2.0f +
                    window_button_count * ui_width,
                    window_y,
                },
                (Vec2){ ui_width / 2.15f, ui_width / 2.15f },
                -draw_angle
            );
            aff2_compose(button_trans, ui_trans, button_trans);
            if (
                aven_gl_ui_button(
                    &ctx->ui,
                    button_trans,
                    AVEN_GL_UI_BUTTON_TYPE_TRIANGLE
                )
            ) {
                ctx->pcg = ctx->info.pcg;
                ctx->active_window = GAME_UI_WINDOW_NONE;
                ctx->session_opts.graph_type = GAME_INFO_GRAPH_TYPE_PYRAMID;
                game_info_setup(
                    &ctx->info,
                    &ctx->session_opts,
                    GAME_ALG_ARENA_SIZE,
                    ctx->pcg
                );
                game_info_alg_setup(
                    &ctx->info.session,
                    &ctx->info.alg,
                    &ctx->session_opts
                );
                ctx->graph_up_to_date = false;
            }
            window_button_count += 1;
        }
    }
    {
        Aff2 button_trans;
        aff2_position_rangle(
            button_trans,
            (Vec2){ 0.0f, top_y - (button_count * ui_width) },
            (Vec2){ ui_width / 2.15f, ui_width / 2.15f },
            -draw_angle
        );
        aff2_compose(button_trans, ui_trans, button_trans);
        switch (ctx->session_opts.graph_type) {
            case GAME_INFO_GRAPH_TYPE_RAND: {
                if (
                    aven_gl_ui_button_maybe(
                        &ctx->ui,
                        button_trans,
                        AVEN_GL_UI_BUTTON_TYPE_DICE,
                        !ctx->alg_opts.playing and
                        ctx->active_window != GAME_UI_WINDOW_GRAPH
                    )
                ) {
                    ctx->active_window = GAME_UI_WINDOW_GRAPH;
                }
                break;
            }
            case GAME_INFO_GRAPH_TYPE_PYRAMID: {
                if (
                    aven_gl_ui_button_maybe(
                        &ctx->ui,
                        button_trans,
                        AVEN_GL_UI_BUTTON_TYPE_TRIANGLE,
                        !ctx->alg_opts.playing and
                        ctx->active_window != GAME_UI_WINDOW_GRAPH
                    )
                ) {
                    ctx->active_window = GAME_UI_WINDOW_GRAPH;
                }
                break;
            }
        }

        button_count += 1;
    }

    {
        Aff2 button_trans;
        aff2_position_rangle(
            button_trans,
            (Vec2){ 0.0f, top_y - (button_count * ui_width) },
            (Vec2){ ui_width / 2.15f, ui_width / 2.15f },
            -draw_angle
        );
        aff2_compose(button_trans, ui_trans, button_trans);
        {
            Aff2 left_trans;
            aff2_position(
                left_trans,
                (Vec2){ -0.525f, 0.0f },
                (Vec2){ 0.475f, 1.0f }
            );
            aff2_compose(left_trans, button_trans, left_trans);
            if (
                aven_gl_ui_button_maybe(
                    &ctx->ui,
                    left_trans,
                    AVEN_GL_UI_BUTTON_TYPE_START,
                    !ctx->alg_opts.playing and ctx->info.alg.steps > 0
                )
            ) {
                ctx->active_window = GAME_UI_WINDOW_NONE;
                game_info_alg_setup(
                    &ctx->info.session,
                    &ctx->info.alg,
                    &ctx->session_opts
                );
                ctx->graph_up_to_date = false;
            }
        }
        {
            Aff2 right_trans;
            aff2_position(
                right_trans,
                (Vec2){ 0.525f, 0.0f },
                (Vec2){ 0.475f, 1.0f }
            );
            aff2_compose(right_trans, button_trans, right_trans);
            if (
                aven_gl_ui_button_maybe(
                    &ctx->ui,
                    right_trans,
                    AVEN_GL_UI_BUTTON_TYPE_END,
                    !ctx->alg_opts.playing and !ctx->info.alg.done
                )
            ) {
                ctx->active_window = GAME_UI_WINDOW_NONE;
                while (!ctx->info.alg.done) {
                    game_info_alg_step(&ctx->info.alg);
                }
                ctx->graph_up_to_date = false;
            }
        }

        button_count += 1;
    }

    {
        Aff2 button_trans;
        aff2_position_rangle(
            button_trans,
            (Vec2){ 0.0f, top_y - (button_count * ui_width) },
            (Vec2){ ui_width / 2.15f, ui_width / 2.15f },
            -draw_angle
        );
        aff2_compose(button_trans, ui_trans, button_trans);
        {
            Aff2 left_trans;
            aff2_position(
                left_trans,
                (Vec2){ -0.525f, 0.0f },
                (Vec2){ 0.475f, 1.0f }
            );
            aff2_compose(left_trans, button_trans, left_trans);
            if (
                aven_gl_ui_button_maybe(
                    &ctx->ui,
                    left_trans,
                    AVEN_GL_UI_BUTTON_TYPE_LEFT,
                    !ctx->alg_opts.playing and ctx->info.alg.steps > 0
                )
            ) {
                ctx->active_window = GAME_UI_WINDOW_NONE;
                size_t steps = ctx->info.alg.steps - 1;
                game_info_alg_setup(
                    &ctx->info.session,
                    &ctx->info.alg,
                    &ctx->session_opts
                );
                for (size_t i = 0; i < steps; i += 1) {
                    game_info_alg_step(&ctx->info.alg);
                }
                ctx->graph_up_to_date = false;
            }
        }
        {
            Aff2 right_trans;
            aff2_position(
                right_trans,
                (Vec2){ 0.525f, 0.0f },
                (Vec2){ 0.475f, 1.0f }
            );
            aff2_compose(right_trans, button_trans, right_trans);
            if (
                aven_gl_ui_button_maybe(
                    &ctx->ui,
                    right_trans,
                    AVEN_GL_UI_BUTTON_TYPE_RIGHT,
                    !ctx->alg_opts.playing and !ctx->info.alg.done
                )
            ) {
                ctx->active_window = GAME_UI_WINDOW_NONE;
                game_info_alg_step(&ctx->info.alg);
                ctx->graph_up_to_date = false;
            }
        }

        button_count += 1;
    }

    {
        Aff2 button_trans;
        aff2_position_rangle(
            button_trans,
            (Vec2){ 0.0f, top_y - (button_count * ui_width) },
            (Vec2){ ui_width / 2.15f, ui_width / 2.15f },
            -draw_angle
        );
        aff2_compose(button_trans, ui_trans, button_trans);
        {
            Aff2 left_trans;
            aff2_position(
                left_trans,
                (Vec2){ -0.525f, 0.0f },
                (Vec2){ 0.475f, 1.0f }
            );
            aff2_compose(left_trans, button_trans, left_trans);
            if (
                aven_gl_ui_button_maybe(
                    &ctx->ui,
                    left_trans,
                    AVEN_GL_UI_BUTTON_TYPE_REWIND,
                    ctx->alg_opts.time_step < GAME_MAX_TIME_STEP
                )
            ) {
                ctx->alg_opts.time_step = min(
                    GAME_MAX_TIME_STEP,
                    ctx->alg_opts.time_step * 2L
                );
                if (!ctx->alg_opts.playing) {
                    ctx->active_window = GAME_UI_WINDOW_PREVIEW;
                }
            }
        }
        {
            Aff2 right_trans;
            aff2_position(
                right_trans,
                (Vec2){ 0.525f, 0.0f },
                (Vec2){ 0.475f, 1.0f }
            );
            aff2_compose(right_trans, button_trans, right_trans);
            if (
                aven_gl_ui_button_maybe(
                    &ctx->ui,
                    right_trans,
                    AVEN_GL_UI_BUTTON_TYPE_FASTFORWARD,
                    ctx->alg_opts.time_step > GAME_MIN_TIME_STEP
                )
            ) {
                ctx->alg_opts.time_step = max(
                    GAME_MIN_TIME_STEP,
                    ctx->alg_opts.time_step / 2L
                );
                if (!ctx->alg_opts.playing) {
                    ctx->active_window = GAME_UI_WINDOW_PREVIEW;
                }
            }
        }

        if (ctx->active_window == GAME_UI_WINDOW_PREVIEW) {
            Aff2 pv_trans;
            aff2_position(
                pv_trans,
                (Vec2){
                    1.5f * padding + ui_width,
                    top_y - (button_count * ui_width),
                },
                (Vec2){ padding + ui_width / 2.0f, padding + ui_width / 2.0f }
            );
            aff2_compose(pv_trans, ui_trans, pv_trans);
            aven_gl_ui_window(&ctx->ui, pv_trans, popup_border);

            float radius = 0.7f;
            float angle = 2.0f * AVEN_MATH_PI_F / (float)GAME_PREVIEW_EDGES;
            for (size_t i = 0; i < GAME_PREVIEW_EDGES; i += 1) {
                Vec2 p1 = {
                    radius * cosf((float)i * angle),
                    radius * sinf((float)i * angle),
                };
                Vec2 p2 = {
                    radius * cosf((float)(i + 1) * angle),
                    radius * sinf((float)(i + 1) * angle),
                };
                Vec2 mp1p2;
                vec2_midpoint(mp1p2, p1, p2);
                Vec2 p1p2;
                vec2_sub(p1p2, p2, p1);
                float mag = vec2_mag(p1p2);
                Vec2 dim = { mag / 2.0f, 0.05f };
                vec2_scale(p1p2, 1.0f / mag, p1p2);

                Aff2 edge_trans;
                aff2_position_rdir(edge_trans, mp1p2, dim, p1p2);
                aff2_compose(edge_trans, pv_trans, edge_trans);
                aven_gl_shape_rounded_geometry_push_square(
                    &ctx->ui.shape.geometry,
                    edge_trans,
                    0.0f,
                    ctx->ui.base_colors.primary
                );

                vec2_scale(p1, 0.5f, p1);
                if (i == ctx->preview.edge_index) {
                    aff2_position_rangle(
                        edge_trans,
                        p1,
                        (Vec2){ radius / 2.0f, 0.125f },
                        (float)i * angle
                    );
                    aff2_compose(edge_trans, pv_trans, edge_trans);
                    aven_gl_shape_rounded_geometry_push_square(
                        &ctx->ui.shape.geometry,
                        edge_trans,
                        0.0f,
                        ctx->ui.base_colors.secondary
                    );
                }
                aff2_position_rangle(
                    edge_trans,
                    p1,
                    (Vec2){ radius / 2.0f, 0.05f },
                    (float)i * angle
                );
                aff2_compose(edge_trans, pv_trans, edge_trans);
                aven_gl_shape_rounded_geometry_push_square(
                    &ctx->ui.shape.geometry,
                    edge_trans,
                    0.0f,
                    ctx->ui.base_colors.primary
                );
            }

            Aff2 vertex_trans;
            aff2_position(
                vertex_trans,
                (Vec2){ 0.0f, 0.0f },
                (Vec2){ 0.225f, 0.225f }
            );
            aff2_compose(vertex_trans, pv_trans, vertex_trans);
            aven_gl_shape_rounded_geometry_push_square(
                &ctx->ui.shape.geometry,
                vertex_trans,
                1.0f,
                ctx->ui.base_colors.secondary
            );
            aff2_position(
                vertex_trans,
                (Vec2){ 0.0f, 0.0f },
                (Vec2){ 0.15f, 0.15f }
            );
            aff2_compose(vertex_trans, pv_trans, vertex_trans);
            aven_gl_shape_rounded_geometry_push_square(
                &ctx->ui.shape.geometry,
                vertex_trans,
                1.0f,
                ctx->ui.base_colors.primary
            );
            for (size_t i = 0; i < GAME_PREVIEW_EDGES; i += 1) {
                aff2_position(
                    vertex_trans,
                    (Vec2){
                        radius * cosf((float)i * angle),
                        radius * sinf((float)i * angle),
                    },
                    (Vec2){ 0.15f, 0.15f }
                );
                aff2_compose(vertex_trans, pv_trans, vertex_trans);
                aven_gl_shape_rounded_geometry_push_square(
                    &ctx->ui.shape.geometry,
                    vertex_trans,
                    1.0f,
                    ctx->ui.base_colors.primary
                );
            }
        }

        button_count += 1;
    }

    {
        Aff2 button_trans;
        aff2_position_rangle(
            button_trans,
            (Vec2){ 0.0f, top_y - (button_count * ui_width) },
            (Vec2){ ui_width / 2.15f, ui_width / 2.15f },
            -draw_angle
        );
        aff2_compose(button_trans, ui_trans, button_trans);
        if (ctx->alg_opts.playing) {
            if (
                aven_gl_ui_button(
                    &ctx->ui,
                    button_trans,
                    AVEN_GL_UI_BUTTON_TYPE_PAUSE
                )
            ) {
                ctx->active_window = GAME_UI_WINDOW_NONE;
                ctx->alg_opts.playing = false;
            }
        } else {
            if (
                aven_gl_ui_button_maybe(
                    &ctx->ui,
                    button_trans,
                    AVEN_GL_UI_BUTTON_TYPE_PLAY,
                    !ctx->info.alg.done
                )
            ) {
                ctx->active_window = GAME_UI_WINDOW_NONE;
                ctx->alg_opts.playing = true;
            }
        }

        button_count += 1;
    }

    if (ctx->ui.empty_click) {
        ctx->active_window = GAME_UI_WINDOW_NONE;
    }

    if (
        !aven_gl_ui_id_eq(last_hot, ctx->ui.hot_id) or
        !aven_gl_ui_id_eq(last_active, ctx->ui.active_id) or
        !aven_gl_ui_id_eq(ctx->ui.hot_id, ctx->ui.next_hot_id) or
        last_window != ctx->active_window
    ) {
        ctx->screen_updates = 0;
        ctx->ui_up_to_date = false;
    }

    if (ctx->graph_up_to_date and ctx->ui_up_to_date) {
        aven_gl_ui_clear(&ctx->ui);
        return false;
    }

    // Generate graph geometry

    gl->Viewport(0, 0, width, height);
    assert(gl->GetError() == 0);

    gl->ClearColor(0.75f, 0.75f, 0.75f, 1.0f);
    assert(gl->GetError() == 0);
    gl->Clear(GL_COLOR_BUFFER_BIT);
    assert(gl->GetError() == 0);

    if (ctx->graph_up_to_date) {
        Aff2 ident;
        aff2_identity(ident);
#ifdef TEXTURE_OPTIMIZATION
        aven_gl_texture_geometry_draw(
            gl,
            &ctx->graph_texture.ctx,
            &ctx->graph_texture.buffer,
            ident
        );
#else // !defined(TEXTURE_OPTIMIZATION)
        aven_gl_shape_draw(gl, &ctx->shapes.ctx, &ctx->shapes.buffer, cam_trans);
        aven_gl_shape_rounded_draw(
            gl,
            &ctx->rounded_shapes.ctx,
            &ctx->rounded_shapes.buffer,
            pixel_size,
            cam_trans
        );
#endif // !defined(TEXTURE_OPTIMIZATION)
    } else {
        aven_gl_shape_geometry_clear(&ctx->shapes.geometry);
        aven_gl_shape_rounded_geometry_clear(&ctx->rounded_shapes.geometry);

        float radius = vertex_radii[
            min(ctx->session_opts.radius, countof(vertex_radii))
        ];

        float border_padding = 1.5f * radius + 5.0f * pixel_size;
        graph_scale *= 1.0f / (1.0f + border_padding);

        Aff2 graph_trans;
        aff2_position_rangle(
            graph_trans,
            (Vec2){ graph_offset, 0.0f },
            (Vec2){ graph_scale, graph_scale },
            -draw_angle
        );
        aff2_rotate(graph_trans, graph_trans, draw_angle);

        switch (ctx->info.alg.type) {
            case GAME_DATA_ALG_TYPE_P3COLOR: {
                GraphPlaneP3ColorGeometryInfo geometry_info = {
                    .outline_color = { 0.1f, 0.1f, 0.1f, 1.0f },
                    .done_color = { 0.6f, 0.6f, 0.6f, 1.0f },
                    .active_color = { 0.5f, 0.1f, 0.5f, 1.0f },
                    .face_color = { 0.15f, 0.6f, 0.6f, 1.0f },
                    .below_color = { 0.55f, 0.65f, 0.15f, 1.0f },
                    .inactive_color = { 0.1f, 0.1f, 0.1f, 1.0f },
                    .edge_thickness = radius / 4.0f,
                    .border_thickness = radius * 0.25f,
                    .radius = radius,
                };
                vec4_copy(
                    geometry_info.colors[0],
                    (Vec4){ 0.9f, 0.9f, 0.9f, 1.0f }
                );
                for (size_t i = 0; i < 3; i += 1) {
                    vec4_copy(geometry_info.colors[i + 1], vertex_colors[i]);
                }
                GameInfoAlgP3Color *alg = &ctx->info.alg.data.p3color;
                graph_plane_p3color_geometry_push_ctx(
                    &ctx->shapes.geometry,
                    &ctx->rounded_shapes.geometry,
                    ctx->info.session.embedding,
                    &alg->ctx,
                    alg->frames,
                    graph_trans,
                    &geometry_info,
                    ctx->info.arena
                );
                break;
            }
            case GAME_DATA_ALG_TYPE_P3COLOR_BFS: {
                GraphPlaneP3ColorBfsGeometryInfo geometry_info = {
                    .outline_color = { 0.1f, 0.1f, 0.1f, 1.0f },
                    .done_color = { 0.6f, 0.6f, 0.6f, 1.0f },
                    .active_color = { 0.5f, 0.1f, 0.5f, 1.0f },
                    .tree_color = { 0.65f, 0.25f, 0.15f, 1.0f },
                    .u_color = { 0.15f, 0.6f, 0.6f, 1.0f },
                    .inactive_color = { 0.1f, 0.1f, 0.1f, 1.0f },
                    .edge_thickness = radius / 4.0f,
                    .border_thickness = radius * 0.25f,
                    .radius = radius,
                };
                vec4_copy(
                    geometry_info.colors[0],
                    (Vec4){ 0.9f, 0.9f, 0.9f, 1.0f }
                );
                for (size_t i = 0; i < 3; i += 1) {
                    vec4_copy(geometry_info.colors[i + 1], vertex_colors[i]);
                }
                GameInfoAlgP3ColorBfs *alg = &ctx->info.alg.data.p3color_bfs;
                graph_plane_p3color_bfs_geometry_push_ctx(
                    &ctx->shapes.geometry,
                    &ctx->rounded_shapes.geometry,
                    ctx->info.session.embedding,
                    &alg->ctx,
                    alg->frames,
                    graph_trans,
                    &geometry_info,
                    ctx->info.arena
                );
                break;
            }
            case GAME_DATA_ALG_TYPE_P3CHOOSE: {
                GraphPlaneP3ChooseGeometryInfo geometry_info = {
                    .colors = {
                        .ptr = ctx->info.session.colors.ptr,
                        .len = ctx->info.session.colors.len,
                    },
                    .outline_color = { 0.1f, 0.1f, 0.1f, 1.0f },
                    .edge_color = { 0.1f, 0.1f, 0.1f, 1.0f },
                    .uncolored_edge_color = { 0.6f, 0.6f, 0.6f, 1.0f },
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
                    .edge_thickness = radius / 4.0f,
                    .border_thickness = radius * 0.25f,
                    .radius = radius,
                };
                GameInfoAlgP3Choose *alg = &ctx->info.alg.data.p3choose;
                graph_plane_p3choose_geometry_push_ctx(
                    &ctx->shapes.geometry,
                    &ctx->rounded_shapes.geometry,
                    ctx->info.session.embedding,
                    &alg->ctx,
                    alg->frames,
                    graph_trans,
                    &geometry_info
                );
            }
            default:
                break;
        }

        // Push geometry to GPU and draw to screen

        aven_gl_shape_buffer_update(
            gl,
            &ctx->shapes.buffer,
            &ctx->shapes.geometry
        );
        aven_gl_shape_rounded_buffer_update(
            gl,
            &ctx->rounded_shapes.buffer,
            &ctx->rounded_shapes.geometry
        );

        aven_gl_shape_draw(gl, &ctx->shapes.ctx, &ctx->shapes.buffer, cam_trans);
        aven_gl_shape_rounded_draw(
            gl,
            &ctx->rounded_shapes.ctx,
            &ctx->rounded_shapes.buffer,
            pixel_size,
            cam_trans
        );

#ifdef TEXTURE_OPTIMIZATION
        if (
            !ctx->alg_opts.playing or
            ctx->alg_opts.time_step > (AVEN_TIME_NSEC_PER_SEC / 30)
        ) {
            ctx->graph_up_to_date = true;

            gl->ColorMask(false, false, false, true);
            assert(gl->GetError() == 0);
            gl->ClearColor(1.0f, 1.0f, 1.0f, 1.0f);
            assert(gl->GetError() == 0);
            gl->Clear(GL_COLOR_BUFFER_BIT);
            assert(gl->GetError() == 0);
            gl->ColorMask(true, true, true, true);
            assert(gl->GetError() == 0);

            aven_gl_texture_ctx_update_framebuffer(
                gl,
                &ctx->graph_texture.ctx,
                0,
                0,
                (size_t)ctx->width,
                (size_t)ctx->height
            );
        }
#else // !defined(TEXTURE_OPTIMIZATION)
        ctx->graph_up_to_date = true;
#endif // !defined(TEXTURE_OPTIMIZATION)
    }

    aven_gl_ui_draw(gl, &ctx->ui, pixel_size, cam_trans);

    gl->ColorMask(false, false, false, true);
    assert(gl->GetError() == 0);
    gl->ClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    assert(gl->GetError() == 0);
    gl->Clear(GL_COLOR_BUFFER_BIT);
    assert(gl->GetError() == 0);
    gl->ColorMask(true, true, true, true);
    assert(gl->GetError() == 0);

    aven_gl_ui_clear(&ctx->ui);

    return true;
}

void game_damage(GameCtx *ctx) {
    ctx->ui_up_to_date = false;
    ctx->screen_updates = 0;
    ctx->graph_up_to_date = false;
}

