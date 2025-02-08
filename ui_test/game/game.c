#include "aven/gl/ui.h"
#if !defined(_WIN32) && !defined(_POSIX_C_SOURCE)
    #define _POSIX_C_SOURCE 200112L
#endif

#ifndef AVEN_IMPLEMENTATION
    #define AVEN_IMPLEMENTATION
#endif
#include <aven.h>
#include <aven/arena.h>
#include <aven/gl/shape.h>
#include <aven/gl/text.h>
#include <aven/math.h>
#include <aven/str.h>
#include <aven/time.h>

#include "../game.h"
#include "font.h"

#include <graph/plane/gen.h>

static GameInfoAlg game_info_alg_init(
    size_t alg_arena_size,
    AvenArena *arena
) {
    GameInfoAlg alg = { 0 };
    alg.init_arena = aven_arena_init(
        aven_arena_alloc(
            arena,
            alg_arena_size,
            AVEN_ARENA_BIGGEST_ALIGNMENT,
            1
        ),
        alg_arena_size
    );

    return alg;
}

static void game_info_alg_setup(
    GameInfoSession *info_session,
    GameInfoAlg *info_alg,
    GameInfoAlgType type,
    size_t nthreads
) {
    info_alg->arena = info_alg->init_arena;
    info_alg->type = type;
    info_alg->steps = 0;
    info_alg->done = false;

    switch (info_alg->type) {
        case GAME_DATA_ALG_TYPE_NONE: {
            break;
        }
        case GAME_DATA_ALG_TYPE_P3COLOR: {
            GameInfoAlgP3Color *alg = &info_alg->data.p3color;

            *alg = (GameInfoAlgP3Color){
                .frames = { .len = nthreads },
            };
            alg->frames.ptr = aven_arena_create_array(
                GraphPlaneP3ColorFrameOptional,
                &info_alg->arena,
                alg->frames.len
            );
            alg->ctx = graph_plane_p3color_init(
                info_session->graph,
                info_session->p1,
                info_session->p2,
                &info_alg->arena
            );
            break;
        }
        case GAME_DATA_ALG_TYPE_P3COLOR_BFS: {
            info_alg->type = GAME_DATA_ALG_TYPE_NONE;
            break;
        }
        case GAME_DATA_ALG_TYPE_P3CHOOSE: {
            info_alg->type = GAME_DATA_ALG_TYPE_NONE;
            break;
        }
    }
}

static bool game_info_alg_step(GameInfoAlg *info_alg) {
    if (!info_alg->done) {
        info_alg->steps += 1;
    }

    switch (info_alg->type) {
        case GAME_DATA_ALG_TYPE_NONE: {
            return true;
        }
        case GAME_DATA_ALG_TYPE_P3COLOR: {
            GameInfoAlgP3Color *alg = &info_alg->data.p3color;
            GraphPlaneP3ColorCtx *ctx = &alg->ctx;

            bool finished = true;
            for (size_t i = 0; i < alg->frames.len; i += 1) {
                GraphPlaneP3ColorFrameOptional *frame = &get(alg->frames, i);
                if (!frame->valid) {
                    *frame = graph_plane_p3color_next_frame(ctx);
                }
                if (!frame->valid) {
                    continue;
                }

                finished = false;

                bool done = graph_plane_p3color_frame_step(ctx, &frame->value);
                if (done) {
                    frame->valid = false;
                }
            }
            if (finished) {
                info_alg->done = true;
            }
            return finished;
        }
        case GAME_DATA_ALG_TYPE_P3COLOR_BFS: {
            return true;
        }
        case GAME_DATA_ALG_TYPE_P3CHOOSE: {
            return true;
        }
    }
}

static GameInfo game_info_init(
    size_t info_arena_size,
    AvenArena *arena
) {
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

static void game_info_setup(
    GameInfo *info,
    uint32_t graph_size,
    uint8_t color_divisions,
    float min_area,
    float min_coeff,
    size_t alg_arena_size,
    AvenRng rng
) {
    info->arena = info->init_arena;

    info->alg = game_info_alg_init(
        alg_arena_size,
        &info->arena
    );

    GameInfoSession session = { 0 };
    Aff2 identity;
    aff2_identity(identity);
    GraphPlaneGenData gen_data = graph_plane_gen_tri(
        graph_size,
        identity,
        min_area,
        min_coeff,
        true,
        rng,
        &info->arena
    );
    session.graph = gen_data.graph;
    session.embedding = gen_data.embedding;
    session.aug_graph = graph_aug(session.graph, &info->arena);

    uint32_t v1 = aven_rng_rand_bounded(rng, 4);
    uint32_t p1_len = 1 + aven_rng_rand_bounded(rng, 3);

    List(uint32_t) p1_list = aven_arena_create_list(uint32_t, &info->arena, 4);
    for (uint32_t i = 0; i < p1_len; i += 1) {
        list_push(p1_list) = (v1 + i) % 4;
    }
    GraphSubset p1 = slice_list(p1_list);
    session.p1 = p1;

    List(uint32_t) p2_list = aven_arena_create_list(uint32_t, &info->arena, 4);
    for (uint32_t i = 0; i < 4 - p1_len; i += 1) {
        list_push(p2_list) = (v1 + p1_len + i) % 4;
    }
    GraphSubset p2 = slice_list(p2_list);
    session.p2 = p2;

    GraphSubset outer_face = aven_arena_create_slice(uint32_t, &info->arena, 4);
    for (uint32_t i = 0; i < 4; i += 1) {
        get(outer_face, i) = (v1 + i) % 4;
    }
    session.outer_cycle = outer_face;

    uint32_t max_color = ((color_divisions + 2) * (color_divisions + 1)) / 2U;
    assert(max_color < 256);

    GraphPlaneP3ChooseListProp color_lists = aven_arena_create_slice(
        GraphPlaneP3ChooseList,
        &info->arena,
        session.graph.len
    );

    for (uint32_t v = 0; v < color_lists.len; v += 1) {
        GraphPlaneP3ChooseList list = { .len = 3 };
        get(list, 0) = (uint8_t)(
            1 + aven_rng_rand_bounded(rng, max_color)
        );
        get(list, 1) = (uint8_t)(
            1 + aven_rng_rand_bounded(rng, max_color)
        );
        get(list, 2) = (uint8_t)(
            1 + aven_rng_rand_bounded(rng, max_color)
        );

        while (get(list, 1) == get(list, 0)) {
            get(list, 1) = (uint8_t)(
                1 + aven_rng_rand_bounded(rng, max_color)
            );
        }
        while (
            get(list, 2) == get(list, 1) or
            get(list, 2) == get(list, 0)
        ) {
            get(list, 2) = (uint8_t)(
                1 + aven_rng_rand_bounded(rng, max_color)
            );
        }

        get(color_lists, v) = list;
    }

    session.colors.len = max_color;
    session.colors.ptr = aven_arena_create_array(
        Vec4,
        &info->arena,
        max_color
    );

    size_t color_index = 0;
    for (uint8_t i = 0; i <= color_divisions; i += 1) {
        for (uint8_t j = i; j <= color_divisions; j += 1) {
            float coeffs[3] = {
                (float)(i) / (float)color_divisions,
                (float)(j - i) / (float)color_divisions,
                (float)(color_divisions - j) /
                    (float)color_divisions,
            };

            Vec4 base_colors[3] = {
                { 0.85f, 0.15f, 0.15f, 1.0f },
                { 0.15f, 0.75f, 0.15f, 1.0f },
                { 0.15f, 0.15f, 0.85f, 1.0f },
            };
            vec4_copy(get(session.colors, color_index), (Vec4){ 0 });
            for (size_t k = 0; k < 3; k += 1) {
                vec4_scale(base_colors[k], coeffs[k], base_colors[k]);
                vec4_add(
                    get(session.colors, color_index),
                    get(session.colors, color_index),
                    base_colors[k]
                );
            }
            color_index += 1;
        }
    }
}

#if !defined(HOT_RELOAD)
static
#elif defined(_MSC_VER)
__declspec(dllexport)
#endif
const GameTable game_table = {
    .init = game_init,
    .reload = game_reload,
    .update = game_update,
    .deinit = game_deinit,
    .mouse_click = game_mouse_click,
    .mouse_move = game_mouse_move,
};

static void game_load(GameCtx *ctx, AvenGl *gl) {
    ctx->arena = ctx->init_arena;

    ByteSlice font_bytes = array_as_bytes(game_font_opensans_ttf);
    ctx->text.font = aven_gl_text_font_init(
        gl,
        font_bytes,
        32.0f,
        ctx->arena
    );

    ctx->text.ctx = aven_gl_text_ctx_init(gl);
    ctx->text.geometry = aven_gl_text_geometry_init(
        128,
        &ctx->arena
    );
    ctx->text.buffer = aven_gl_text_buffer_init(
        gl,
        &ctx->text.geometry,
        AVEN_GL_BUFFER_USAGE_DYNAMIC
    );

    ctx->shapes.ctx = aven_gl_shape_rounded_ctx_init(gl);
    ctx->shapes.geometry = aven_gl_shape_rounded_geometry_init(
        128,
        192,
        &ctx->arena
    );
    ctx->shapes.buffer = aven_gl_shape_rounded_buffer_init(
        gl,
        &ctx->shapes.geometry,
        AVEN_GL_BUFFER_USAGE_DYNAMIC
    );

    AvenGlUiColors base_colors = {
        .background = { 0.45f, 0.45f, 0.45f, 1.0f },
        .primary = { 0.15f, 0.25f, 0.05f, 1.0f },
        .secondary = { 0.25f, 0.05f, 0.15f, 1.0f },
    };
    AvenGlUiColors hovered_colors = {
        .background = { 0.475f, 0.475f, 0.475f, 1.0f },
        .primary = { 0.175f, 0.275f, 0.075f, 1.0f },
        .secondary = { 0.275f, 0.075f, 0.175f, 1.0f },
    };
    AvenGlUiColors clicked_colors = base_colors;
    ctx->ui = aven_gl_ui_init(
        gl,
        128,
        base_colors,
        hovered_colors,
        clicked_colors,
        &ctx->arena
    );
}

static void game_unload(GameCtx *ctx, AvenGl *gl) {
    aven_gl_ui_deinit(gl, &ctx->ui);
    aven_gl_shape_rounded_buffer_deinit(gl, &ctx->shapes.buffer);
    aven_gl_shape_rounded_ctx_deinit(gl, &ctx->shapes.ctx);
    ctx->shapes = (GameShapes){ 0 };

    aven_gl_text_buffer_deinit(gl, &ctx->text.buffer);
    aven_gl_text_ctx_deinit(gl, &ctx->text.ctx);
    ctx->text = (GameText){ 0 };
}

GameCtx game_init(AvenGl *gl, AvenArena *arena) {
    GameCtx ctx = { 0 };

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

    ctx.info = game_info_init(
        GAME_LEVEL_ARENA_SIZE,
        arena
    );

    ctx.last_update = aven_time_now();

    ctx.pcg = aven_rng_pcg_seed(0xdead, 0xbeef);
    ctx.rng = aven_rng_pcg(&ctx.pcg);

    return ctx;
}

void game_deinit(GameCtx *ctx, AvenGl *gl) {
    aven_gl_text_font_deinit(gl, &ctx->text.font);
    game_unload(ctx, gl);
    *ctx = (GameCtx){ 0 };
}

int game_reload(GameCtx *ctx, AvenGl *gl) {
    game_unload(ctx, gl);
    game_load(ctx, gl);

    return 0;
}

void game_mouse_move(
    GameCtx *ctx,
    Vec2 pos
) {
    float side = (float)ctx->height;
    if (ctx->height > ctx->width) {
        side = (float)ctx->width;
    }
    Aff2 screen_trans;
    aff2_camera_position(
        screen_trans,
        (Vec2){
            (float)ctx->width / 2.0f,
            (float)ctx->height / 2.0f,
        },
        (Vec2){ side / 2.0f, -side / 2.0f },
        0.0f
    );
    
    aff2_transform(pos, screen_trans, pos);

    aven_gl_ui_mouse_move(
        &ctx->ui,
        pos
    );
}

void game_mouse_click(
    GameCtx *ctx,
    AvenGlUiMouseEvent event
) {
    aven_gl_ui_mouse_click(&ctx->ui, event);
}

int game_update(
    GameCtx *ctx,
    AvenGl *gl,
    int width,
    int height,
    AvenArena arena
) {
    (void)arena;
    AvenTimeInst now = aven_time_now();
    int64_t elapsed = aven_time_since(now, ctx->last_update);
    ctx->last_update = now;

    float screen_ratio = (float)width / (float)height;
    float pixel_size = 2.0f / (float)height;

    ctx->width = width;
    ctx->height = height;

    bool changed;

    if (ctx->paused) {
    } else {
        while (elapsed >= ctx->time_step) {
            elapsed -= ctx->time_step;

            if (
                ctx->info.alg.type == GAME_DATA_ALG_TYPE_NONE or
                ctx->info.alg.done
            ) {
                GameInfoAlgType last_type = ctx->info.alg.type;
                game_info_setup(
                    &ctx->info,
                    ctx->nvertices,
                    ctx->color_divisions,
                    ctx->min_area,
                    ctx->min_coeff,
                    GAME_ALG_ARENA_SIZE,
                    ctx->rng
                );
                game_info_alg_setup(
                    &ctx->info.session,
                    &ctx->info.alg,
                    last_type, ctx->nthreads
                );
                ctx->paused = true;
            } else {
                bool done = game_info_alg_step(&ctx->info.alg);
                if (done) {
                    ctx->paused = true;
                }
            }
        }
    }

    gl->Viewport(0, 0, width, height);
    assert(gl->GetError() == 0);
    gl->ClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    assert(gl->GetError() == 0);
    gl->Clear(GL_COLOR_BUFFER_BIT);
    assert(gl->GetError() == 0);

    Aff2 cam_trans;
    aff2_camera_position(
        cam_trans,
        (Vec2){ 0.0f, 0.0f },
        (Vec2){ screen_ratio, 1.0f },
        0.0f
    );

    aven_gl_text_geometry_clear(&ctx->text.geometry);
    aven_gl_shape_rounded_geometry_clear(&ctx->shapes.geometry);

    // Draw UI

    Aff2 button_transform;
    aff2_position(
        button_transform,
        (Vec2){ 0.0f, 0.0f },
        (Vec2){ 1.0f / 10.0f, 1.0f / 10.0f },
        0.0f
    );
    if (ctx->info.threaded) {
        if (
            aven_gl_ui_button(
                &ctx->ui,
                button_transform,
                AVEN_GL_UI_BUTTON_TYPE_MULTITHREAD
            )
        ) {
            ctx->info.threaded = false;
        }
    } else {
        if (
            aven_gl_ui_button(
                &ctx->ui,
                button_transform,
                AVEN_GL_UI_BUTTON_TYPE_THREAD
            )
        ) {
            ctx->info.threaded = true;
        }
    }

    // Draw graphs

    aven_gl_shape_rounded_buffer_update(
        gl,
        &ctx->shapes.buffer,
        &ctx->shapes.geometry
    );
    aven_gl_text_buffer_update(gl, &ctx->text.buffer, &ctx->text.geometry);

    aven_gl_shape_rounded_draw(
        gl,
        &ctx->shapes.ctx,
        &ctx->shapes.buffer,
        pixel_size,
        cam_trans
    );
    aven_gl_text_geometry_draw(
        gl,
        &ctx->text.ctx,
        &ctx->text.buffer,
        &ctx->text.font,
        cam_trans
    );

    aven_gl_ui_draw(
        gl,
        &ctx->ui,
        pixel_size,
        cam_trans
    );

    return 0;
}

