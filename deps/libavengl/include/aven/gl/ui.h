#ifndef AVEN_GL_UI_H
#define AVEN_GL_UI_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/math.h>

#include "../gl.h"
#include "shape.h"

typedef struct {
    uintptr_t parent;
    uintptr_t unique;
    size_t index;
} AvenGlUiId;

#define aven_gl_ui_id_internal(p, u, i) (AvenGlUiId){ \
        .parent = (uintptr_t)(p), \
        .unique = (uintptr_t)(u), \
        .index = (size_t)(i), \
    }

static bool aven_gl_ui_id_eq(AvenGlUiId id1, AvenGlUiId id2) {
    return (id1.parent == id2.parent) and
        (id1.unique == id2.unique) and
        (id1.index == id2.index);
}

// typedef enum {
//     AVEN_GL_UI_ELEMENT_TYPE_BUTTON,
// } AvenGlUiElementType;

// typedef union {
//     struct {
//         AvenStr text;
//     } button;
// } AvenGlUiElementData;

// typedef struct {
//     AvenGlUiElementType type;
//     AvenGlUiElementData data;
// } AvenGlUiElement;

typedef struct {
    Vec4 background;
    Vec4 primary;
    Vec4 secondary;
} AvenGlUiColors;

typedef struct {
    AvenGlShapeRoundedCtx ctx;
    AvenGlShapeRoundedGeometry geometry;
    AvenGlShapeRoundedBuffer buffer;
} AvenGlUiShape;

// typedef struct {
//     ByteSlice font_bytes;
//     AvenArena font_arena;
//     AvenGlTextFont font;
//     AvenGlTextCtx ctx;
//     AvenGlTextGeometry geometry;
//     AvenGlTextBuffer buffer;
//     Vec4 color;
//     float norm_height;
// } AvenGlUiText;

typedef enum {
    AVEN_GL_UI_MOUSE_EVENT_NONE = 0,
    AVEN_GL_UI_MOUSE_EVENT_DOWN,
    AVEN_GL_UI_MOUSE_EVENT_UP,
} AvenGlUiMouseEvent;

typedef struct {
    Vec2 pos;
    AvenGlUiMouseEvent event;
} AvenGlUiMouse;


// #define AVEN_GL_UI_KEYBOARD_MAX (348)
// 
// typedef struct {
//     enum {
//         AVEN_GL_UI_KEYBOARD_NONE = 0,
//         AVEN_GL_UI_KEYBOARD_DOWN,
//         AVEN_GL_UI_KEYBOARD_UP,
//     } keys[AVEN_GL_UI_KEYBOARD_MAX];
//     bool captured;
// } AvenGlUiKeyboard;

typedef struct {
    AvenGlUiShape shape;
    AvenGlUiColors base_colors;
    AvenGlUiColors hovered_colors;
    AvenGlUiColors clicked_colors;
    AvenGlUiMouse mouse;
    // AvenGlUiKeyboard keyboard;
    AvenGlUiId active_id;
    // AvenGlUiId hot_id;
    float pixel_size;
} AvenGlUi;

static inline AvenGlUi aven_gl_ui_init(
    AvenGl *gl,
    size_t max_elems,
    AvenGlUiColors base_colors,
    AvenGlUiColors hovered_colors,
    AvenGlUiColors clicked_colors,
    AvenArena *arena
) {
    AvenGlUi ctx = {
        .base_colors = base_colors,
        .clicked_colors = clicked_colors,
        .hovered_colors = hovered_colors,
        .mouse = { .pos = { -1e10f, -1e10f } },
    };

    ctx.shape.ctx = aven_gl_shape_rounded_ctx_init(gl);
    ctx.shape.geometry = aven_gl_shape_rounded_geometry_init(
        4 * max_elems,
        6 * max_elems,
        arena
    );
    ctx.shape.buffer = aven_gl_shape_rounded_buffer_init(
        gl,
        &ctx.shape.geometry,
        AVEN_GL_BUFFER_USAGE_DYNAMIC
    );

    return ctx;
}

static inline void aven_gl_ui_deinit(AvenGl *gl, AvenGlUi *ctx) {
    aven_gl_shape_rounded_buffer_deinit(gl, &ctx->shape.buffer);
    aven_gl_shape_rounded_geometry_deinit(&ctx->shape.geometry);
    aven_gl_shape_rounded_ctx_deinit(gl, &ctx->shape.ctx);
}

static inline void aven_gl_ui_mouse_move(
    AvenGlUi *ctx,
    Vec2 pos
) {
    vec2_copy(ctx->mouse.pos, pos);
}

static inline void aven_gl_ui_mouse_click(
    AvenGlUi *ctx,
    AvenGlUiMouseEvent event
) {
    ctx->mouse.event = event;
}

static inline void aven_gl_ui_draw(
    AvenGl *gl,
    AvenGlUi *ctx,
    float pixel_size,
    Aff2 cam_trans
) {
    aven_gl_shape_rounded_buffer_update(
        gl,
        &ctx->shape.buffer,
        &ctx->shape.geometry
    );
    aven_gl_shape_rounded_geometry_clear(&ctx->shape.geometry);
    aven_gl_shape_rounded_draw(
        gl,
        &ctx->shape.ctx,
        &ctx->shape.buffer,
        pixel_size,
        cam_trans
    );
    ctx->mouse.event = AVEN_GL_UI_MOUSE_EVENT_NONE;
}

static inline void aven_gl_ui_push_thread(
    AvenGlUi *ctx,
    Aff2 trans,
    AvenGlUiColors* colors
) {
    Aff2 circle_trans;
    aff2_position(
        circle_trans,
        (Vec2){ 0.0f, -0.5f },
        (Vec2){ 0.66f, 0.22f },
        0.0f
    );
    aff2_compose(circle_trans, trans, circle_trans);
    aven_gl_shape_rounded_geometry_push_square(
        &ctx->shape.geometry,
        circle_trans,
        1.0f,
        colors->primary
    );
    aff2_position(
        circle_trans,
        (Vec2){ 0.0f, -0.5f },
        (Vec2){ 0.33f, 0.11f },
        0.0f
    );
    aff2_compose(circle_trans, trans, circle_trans);
    aven_gl_shape_rounded_geometry_push_square(
        &ctx->shape.geometry,
        circle_trans,
        1.0f,
        colors->secondary
    );
    aff2_position(
        circle_trans,
        (Vec2){ 0.0f, 0.0f },
        (Vec2){ 0.33f, 0.5f },
        0.0f
    );
    aff2_compose(circle_trans, trans, circle_trans);
    aven_gl_shape_rounded_geometry_push_square(
        &ctx->shape.geometry,
        circle_trans,
        0.0f,
        colors->secondary
    );
    aff2_position(
        circle_trans,
        (Vec2){ 0.0f, 0.5f },
        (Vec2){ 0.66f, 0.22f },
        0.0f
    );
    aff2_compose(circle_trans, trans, circle_trans);
    aven_gl_shape_rounded_geometry_push_square(
        &ctx->shape.geometry,
        circle_trans,
        1.0f,
        colors->primary
    );
}

static inline void aven_gl_ui_push_magnifier(
    AvenGlUi *ctx,
    Aff2 trans,
    AvenGlUiColors* colors
) {
    Aff2 circle_trans;
    aff2_position(
        circle_trans,
        (Vec2){ -0.33f, -0.33f },
        (Vec2){ 0.475f, 0.18f },
        AVEN_MATH_PI_F / 4.0f
    );
    aff2_compose(circle_trans, trans, circle_trans);
    aven_gl_shape_rounded_geometry_push_square(
        &ctx->shape.geometry,
        circle_trans,
        0.1f,
        colors->primary
    );

    aff2_position(
        circle_trans,
        (Vec2){ 0.15f, 0.15f },
        (Vec2){ 0.7f, 0.7f },
        0.0f
    );
    aff2_compose(circle_trans, trans, circle_trans);
    aven_gl_shape_rounded_geometry_push_square(
        &ctx->shape.geometry,
        circle_trans,
        1.0f,
        colors->primary
    );

    aff2_position(
        circle_trans,
        (Vec2){ 0.15f, 0.15f },
        (Vec2){ 0.5f, 0.5f },
        0.0f
    );
    aff2_compose(circle_trans, trans, circle_trans);
    aven_gl_shape_rounded_geometry_push_square(
        &ctx->shape.geometry,
        circle_trans,
        1.0f,
        colors->background
    );
}

static inline void aven_gl_ui_push_plus(
    AvenGlUi *ctx,
    Aff2 trans,
    AvenGlUiColors* colors
) {
    Aff2 plus_trans;
    aff2_position(
        plus_trans,
        (Vec2){ 0.0f, 0.0f },
        (Vec2){ 0.25f, 0.75f },
        0.0f
    );
    aff2_compose(plus_trans, trans, plus_trans);
    aven_gl_shape_rounded_geometry_push_square(
        &ctx->shape.geometry,
        plus_trans,
        0.1f,
        colors->primary
    );
    aff2_position(
        plus_trans,
        (Vec2){ 0.0f, 0.0f },
        (Vec2){ 0.75f, 0.25f },
        0.0f
    );
    aff2_compose(plus_trans, trans, plus_trans);
    aven_gl_shape_rounded_geometry_push_square(
        &ctx->shape.geometry,
        plus_trans,
        0.1f,
        colors->primary
    );
}

static inline void aven_gl_ui_push_minus(
    AvenGlUi *ctx,
    Aff2 trans,
    AvenGlUiColors* colors
) {
    Aff2 shaft_trans;
    aff2_position(
        shaft_trans,
        (Vec2){ 0.0f, 0.0f },
        (Vec2){ 0.66f, 0.25f },
        0.0f
    );
    aff2_compose(shaft_trans, trans, shaft_trans);
    aven_gl_shape_rounded_geometry_push_square(
        &ctx->shape.geometry,
        shaft_trans,
        0.1f,
        colors->primary
    );
}

typedef enum {
    AVEN_GL_UI_BUTTON_TYPE_PLAY,
    AVEN_GL_UI_BUTTON_TYPE_PAUSE,
    AVEN_GL_UI_BUTTON_TYPE_RIGHT,
    AVEN_GL_UI_BUTTON_TYPE_LEFT,
    AVEN_GL_UI_BUTTON_TYPE_FASTFORWARD,
    AVEN_GL_UI_BUTTON_TYPE_REWIND,
    AVEN_GL_UI_BUTTON_TYPE_PLUS,
    AVEN_GL_UI_BUTTON_TYPE_MINUS,
    AVEN_GL_UI_BUTTON_TYPE_THREAD,
    AVEN_GL_UI_BUTTON_TYPE_MULTITHREAD,
    AVEN_GL_UI_BUTTON_TYPE_ZOOMIN,
    AVEN_GL_UI_BUTTON_TYPE_ZOOMOUT,
} AvenGlUiButtonType;

static inline bool aven_gl_ui_button_internal(
    AvenGlUi *ctx,
    AvenGlUiId id,
    Aff2 trans,
    AvenGlUiButtonType type
) {
    Vec2 pos = { 0.0f, 0.0f };
    Vec2 dim = { 1.0f, 1.0f };

    aff2_transform(pos, trans, pos);
    aff2_transform(dim, trans, dim);

    bool hovered = vec2_point_in_rect(pos, dim, ctx->mouse.pos);

    if (hovered and ctx->mouse.event == AVEN_GL_UI_MOUSE_EVENT_DOWN) {
        ctx->active_id = id;
    }

    bool clicked = aven_gl_ui_id_eq(id, ctx->active_id);

    bool value = false;

    if (clicked and ctx->mouse.event == AVEN_GL_UI_MOUSE_EVENT_UP) {
        if (hovered) {
            value = true;
        }
        ctx->active_id = (AvenGlUiId){ 0 };
    }

    AvenGlUiColors *colors = &ctx->base_colors;
    if (clicked) {
        colors = &ctx->clicked_colors;
    } else if (hovered) {
        colors = &ctx->hovered_colors;
    }

    aven_gl_shape_rounded_geometry_push_square(
        &ctx->shape.geometry,
        trans,
        0.15f,
        colors->primary
    );

    aff2_stretch(trans, (Vec2){ 0.85f, 0.85f }, trans);
    aven_gl_shape_rounded_geometry_push_square(
        &ctx->shape.geometry,
        trans,
        0.15f,
        colors->background
    );

    switch (type) {
        case AVEN_GL_UI_BUTTON_TYPE_PLAY: {
            aven_gl_shape_rounded_geometry_push_triangle(
                &ctx->shape.geometry,
                trans,
                (Vec2){ 0.9f, 0.0f },
                (Vec2){ -0.6f, -0.75f },
                (Vec2){ -0.6f, 0.75f },
                0.25f,
                colors->primary
            );
            break;
        }
        case AVEN_GL_UI_BUTTON_TYPE_PAUSE: {
            Aff2 pause_trans;
            aff2_position(
                pause_trans,
                (Vec2){ -0.33f, 0.0f },
                (Vec2){ 0.25f, 0.66f },
                0.0f
            );
            aff2_compose(pause_trans, trans, pause_trans);
            aven_gl_shape_rounded_geometry_push_square(
                &ctx->shape.geometry,
                pause_trans,
                0.1f,
                colors->primary
            );
            aff2_position(
                pause_trans,
                (Vec2){ 0.33f, 0.0f },
                (Vec2){ 0.25f, 0.66f },
                0.0f
            );
            aff2_compose(pause_trans, trans, pause_trans);
            aven_gl_shape_rounded_geometry_push_square(
                &ctx->shape.geometry,
                pause_trans,
                0.1f,
                colors->primary
            );
            break;
        }
        case AVEN_GL_UI_BUTTON_TYPE_RIGHT: {
            Aff2 shaft_trans;
            aff2_position(
                shaft_trans,
                (Vec2){ -0.2f, 0.0f },
                (Vec2){ 0.5f, 0.33f },
                0.0f
            );
            aff2_compose(shaft_trans, trans, shaft_trans);
            aven_gl_shape_rounded_geometry_push_square(
                &ctx->shape.geometry,
                shaft_trans,
                0.1f,
                colors->primary
            );
            aven_gl_shape_rounded_geometry_push_triangle(
                &ctx->shape.geometry,
                trans,
                (Vec2){ 0.9f, 0.0f },
                (Vec2){ -0.1f, -0.75f },
                (Vec2){ -0.1f, 0.75f },
                0.25f,
                colors->primary
            );
            break;
        }
        case AVEN_GL_UI_BUTTON_TYPE_LEFT: {
            Aff2 shaft_trans;
            aff2_position(
                shaft_trans,
                (Vec2){ 0.2f, 0.0f },
                (Vec2){ 0.5f, 0.33f },
                0.0f
            );
            aff2_compose(shaft_trans, trans, shaft_trans);
            aven_gl_shape_rounded_geometry_push_square(
                &ctx->shape.geometry,
                shaft_trans,
                0.1f,
                colors->primary
            );
            aven_gl_shape_rounded_geometry_push_triangle(
                &ctx->shape.geometry,
                trans,
                (Vec2){ -0.9f, 0.0f },
                (Vec2){ 0.1f, -0.75f },
                (Vec2){ 0.1f, 0.75f },
                0.25f,
                colors->primary
            );
            break;
        }
        case AVEN_GL_UI_BUTTON_TYPE_FASTFORWARD: {
            aven_gl_shape_rounded_geometry_push_triangle(
                &ctx->shape.geometry,
                trans,
                (Vec2){ 0.35f, 0.0f },
                (Vec2){ -0.75f, -0.75f },
                (Vec2){ -0.75f, 0.75f },
                0.25f,
                colors->primary
            );
            aven_gl_shape_rounded_geometry_push_triangle(
                &ctx->shape.geometry,
                trans,
                (Vec2){ 1.0f, 0.0f },
                (Vec2){ -0.1f, -0.75f },
                (Vec2){ -0.1f, 0.75f },
                0.25f,
                colors->primary
            );
            break;
        }
        case AVEN_GL_UI_BUTTON_TYPE_REWIND: {
            aven_gl_shape_rounded_geometry_push_triangle(
                &ctx->shape.geometry,
                trans,
                (Vec2){ -0.35f, 0.0f },
                (Vec2){ 0.75f, -0.75f },
                (Vec2){ 0.75f, 0.75f },
                0.25f,
                colors->primary
            );
            aven_gl_shape_rounded_geometry_push_triangle(
                &ctx->shape.geometry,
                trans,
                (Vec2){ -1.0f, 0.0f },
                (Vec2){ 0.1f, -0.75f },
                (Vec2){ 0.1f, 0.75f },
                0.25f,
                colors->primary
            );
            break;
        }
        case AVEN_GL_UI_BUTTON_TYPE_PLUS: {
            aven_gl_ui_push_plus(ctx, trans, colors);
            break;
        }
        case AVEN_GL_UI_BUTTON_TYPE_MINUS: {
            aven_gl_ui_push_minus(ctx, trans, colors);
            break;
        }
        case AVEN_GL_UI_BUTTON_TYPE_THREAD: {
            aven_gl_ui_push_thread(ctx, trans, colors);
            break;
        }
        case AVEN_GL_UI_BUTTON_TYPE_MULTITHREAD: {
            Aff2 thread_trans;
            aff2_position(
                thread_trans,
                (Vec2){ 0.0f, 0.4f },
                (Vec2){ 0.55f, 0.55f },
                0.0f
            );
            aff2_compose(thread_trans, trans, thread_trans);
            aven_gl_ui_push_thread(ctx, thread_trans, colors);

            aff2_position(
                thread_trans,
                (Vec2){ -0.4f, -0.4f },
                (Vec2){ 0.55f, 0.55f },
                0.0f
            );
            aff2_compose(thread_trans, trans, thread_trans);
            aven_gl_ui_push_thread(ctx, thread_trans, colors);

            aff2_position(
                thread_trans,
                (Vec2){ 0.4f, -0.4f },
                (Vec2){ 0.55f, 0.55f },
                0.0f
            );
            aff2_compose(thread_trans, trans, thread_trans);
            aven_gl_ui_push_thread(ctx, thread_trans, colors);
            break;
        }
        case AVEN_GL_UI_BUTTON_TYPE_ZOOMIN: {
            aven_gl_ui_push_magnifier(ctx, trans, colors);

            Aff2 plus_trans;
            aff2_position(
                plus_trans,
                (Vec2){ 0.15f, 0.15f },
                (Vec2){ 0.5f, 0.5f },
                0.0f
            );
            aff2_compose(plus_trans, trans, plus_trans);
            aven_gl_ui_push_plus(ctx, plus_trans, colors);
            break;
        }
        case AVEN_GL_UI_BUTTON_TYPE_ZOOMOUT: {
            aven_gl_ui_push_magnifier(ctx, trans, colors);

            Aff2 plus_trans;
            aff2_position(
                plus_trans,
                (Vec2){ 0.15f, 0.15f },
                (Vec2){ 0.5f, 0.5f },
                0.0f
            );
            aff2_compose(plus_trans, trans, plus_trans);
            aven_gl_ui_push_minus(ctx, plus_trans, colors);
            break;
        }
    }

    return value;
}

#define aven_gl_ui_button(ctx, trans, type) aven_gl_ui_button_internal( \
        ctx, \
        aven_gl_ui_id_internal(__FILE__, type, __LINE__), \
        trans, \
        type \
    )

#endif
