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
    AvenGlUiColors disabled_colors;
    AvenGlUiMouse mouse;
    // AvenGlUiKeyboard keyboard;
    AvenGlUiId active_id;
    AvenGlUiId hot_id;
    AvenGlUiId next_hot_id;
    float pixel_size;
    bool empty_click;
} AvenGlUi;

static inline AvenGlUi aven_gl_ui_init(
    AvenGl *gl,
    size_t max_elems,
    AvenGlUiColors base_colors,
    AvenGlUiColors hovered_colors,
    AvenGlUiColors clicked_colors,
    AvenGlUiColors disabled_colors,
    AvenArena *arena
) {
    AvenGlUi ctx = {
        .base_colors = base_colors,
        .clicked_colors = clicked_colors,
        .hovered_colors = hovered_colors,
        .disabled_colors = disabled_colors,
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
    if (event == AVEN_GL_UI_MOUSE_EVENT_UP) {
        ctx->empty_click = true;
    }
}

static inline void aven_gl_ui_draw(
    AvenGl *gl,
    AvenGlUi *ctx,
    float pixel_size,
    Aff2 cam_trans
) {
    // draw the geometry of the current ui state
    aven_gl_shape_rounded_buffer_update(
        gl,
        &ctx->shape.buffer,
        &ctx->shape.geometry
    );
    aven_gl_shape_rounded_draw(
        gl,
        &ctx->shape.ctx,
        &ctx->shape.buffer,
        pixel_size,
        cam_trans
    );

    // clear ui geometry and set up the next ui state
    aven_gl_shape_rounded_geometry_clear(&ctx->shape.geometry);
    ctx->mouse.event = AVEN_GL_UI_MOUSE_EVENT_NONE;
    ctx->hot_id = ctx->next_hot_id;
    ctx->next_hot_id = (AvenGlUiId){ 0 };
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

static inline void aven_gl_ui_push_double_thread(
    AvenGlUi *ctx,
    Aff2 trans,
    AvenGlUiColors* colors
) {
    Aff2 thread_trans;
    aff2_position(
        thread_trans,
        (Vec2){ -0.4f, 0.15f },
        (Vec2){ 0.75f, 0.75f },
        0.0f
    );
    aff2_compose(thread_trans, trans, thread_trans);
    aven_gl_ui_push_thread(ctx, thread_trans, colors);

    aff2_position(
        thread_trans,
        (Vec2){ 0.4f, -0.15f },
        (Vec2){ 0.75f, 0.75f },
        0.0f
    );
    aff2_compose(thread_trans, trans, thread_trans);
    aven_gl_ui_push_thread(ctx, thread_trans, colors);
}

static inline void aven_gl_ui_push_triple_thread(
    AvenGlUi *ctx,
    Aff2 trans,
    AvenGlUiColors* colors
) {
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
}

static inline void aven_gl_ui_push_quadra_thread(
    AvenGlUi *ctx,
    Aff2 trans,
    AvenGlUiColors* colors
) {
    Aff2 thread_trans;
    aff2_position(
        thread_trans,
        (Vec2){ -0.45f, -0.45f },
        (Vec2){ 0.55f, 0.55f },
        0.0f
    );
    aff2_compose(thread_trans, trans, thread_trans);
    aven_gl_ui_push_thread(ctx, thread_trans, colors);

    aff2_position(
        thread_trans,
        (Vec2){ 0.45f, -0.45f },
        (Vec2){ 0.55f, 0.55f },
        0.0f
    );
    aff2_compose(thread_trans, trans, thread_trans);
    aven_gl_ui_push_thread(ctx, thread_trans, colors);

    aff2_position(
        thread_trans,
        (Vec2){ 0.45f, 0.45f },
        (Vec2){ 0.55f, 0.55f },
        0.0f
    );
    aff2_compose(thread_trans, trans, thread_trans);
    aven_gl_ui_push_thread(ctx, thread_trans, colors);

    aff2_position(
        thread_trans,
        (Vec2){ -0.45f, 0.45f },
        (Vec2){ 0.55f, 0.55f },
        0.0f
    );
    aff2_compose(thread_trans, trans, thread_trans);
    aven_gl_ui_push_thread(ctx, thread_trans, colors);
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

static inline void aven_gl_ui_push_zoomin(
    AvenGlUi *ctx,
    Aff2 trans,
    AvenGlUiColors* colors
) {
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
}

static inline void aven_gl_ui_push_zoomout(
    AvenGlUi *ctx,
    Aff2 trans,
    AvenGlUiColors* colors
) {
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
}

static inline void aven_gl_ui_push_play(
    AvenGlUi *ctx,
    Aff2 trans,
    AvenGlUiColors* colors
) {
    aven_gl_shape_rounded_geometry_push_triangle(
        &ctx->shape.geometry,
        trans,
        (Vec2){ 0.9f, 0.0f },
        (Vec2){ -0.6f, -0.75f },
        (Vec2){ -0.6f, 0.75f },
        0.25f,
        colors->primary
    );
}

static inline void aven_gl_ui_push_pause(
    AvenGlUi *ctx,
    Aff2 trans,
    AvenGlUiColors* colors
) {
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
}

static inline void aven_gl_ui_push_right(
    AvenGlUi *ctx,
    Aff2 trans,
    AvenGlUiColors* colors
) {
    aven_gl_shape_rounded_geometry_push_triangle(
        &ctx->shape.geometry,
        trans,
        (Vec2){ 0.75f, 0.0f },
        (Vec2){ -0.5f, -0.75f },
        (Vec2){ -0.5f, 0.75f },
        0.1f,
        colors->primary
    );
}

static inline void aven_gl_ui_push_left(
    AvenGlUi *ctx,
    Aff2 trans,
    AvenGlUiColors* colors
) {
    Aff2 rtrans;
    aff2_identity(rtrans);
    aff2_stretch(rtrans, (Vec2){ -1.0f, 1.0f }, rtrans);
    aff2_compose(rtrans, trans, rtrans);
    aven_gl_ui_push_right(ctx, rtrans, colors);
}

static inline void aven_gl_ui_push_right_arrow(
    AvenGlUi *ctx,
    Aff2 trans,
    AvenGlUiColors* colors
) {
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
}

static inline void aven_gl_ui_push_left_arrow(
    AvenGlUi *ctx,
    Aff2 trans,
    AvenGlUiColors* colors
) {
    Aff2 rtrans;
    aff2_identity(rtrans);
    aff2_stretch(rtrans, (Vec2){ -1.0f, 1.0f }, rtrans);
    aff2_compose(rtrans, trans, rtrans);
    aven_gl_ui_push_right_arrow(ctx, rtrans, colors);
}

static inline void aven_gl_ui_push_fastforward(
    AvenGlUi *ctx,
    Aff2 trans,
    AvenGlUiColors* colors
) {
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
}

static inline void aven_gl_ui_push_rewind(
    AvenGlUi *ctx,
    Aff2 trans,
    AvenGlUiColors* colors
) {
    Aff2 rtrans;
    aff2_identity(rtrans);
    aff2_stretch(rtrans, (Vec2){ -1.0f, 1.0f }, rtrans);
    aff2_compose(rtrans, trans, rtrans);
    aven_gl_ui_push_fastforward(ctx, rtrans, colors);
}

static inline void aven_gl_ui_push_cross(
    AvenGlUi *ctx,
    Aff2 trans,
    AvenGlUiColors* colors
) {
    Aff2 shaft_trans;
    aff2_position(
        shaft_trans,
        (Vec2){ 0.0f, 0.0f },
        (Vec2){ 0.75f, 0.25f },
        AVEN_MATH_PI_F / 4.0f
    );
    aff2_compose(shaft_trans, trans, shaft_trans);
    aven_gl_shape_rounded_geometry_push_square(
        &ctx->shape.geometry,
        shaft_trans,
        0.1f,
        colors->primary
    );
    aff2_position(
        shaft_trans,
        (Vec2){ 0.0f, 0.0f },
        (Vec2){ 0.75f, 0.25f },
        -AVEN_MATH_PI_F / 4.0f
    );
    aff2_compose(shaft_trans, trans, shaft_trans);
    aven_gl_shape_rounded_geometry_push_square(
        &ctx->shape.geometry,
        shaft_trans,
        0.1f,
        colors->primary
    );
}

static inline void aven_gl_ui_push_check(
    AvenGlUi *ctx,
    Aff2 trans,
    AvenGlUiColors* colors
) {
    Aff2 shaft_trans;
    aff2_position(
        shaft_trans,
        (Vec2){ 0.1f, 0.0f },
        (Vec2){ 0.66f, 0.25f },
        AVEN_MATH_PI_F / 4.0f
    );
    aff2_compose(shaft_trans, trans, shaft_trans);
    aven_gl_shape_rounded_geometry_push_square(
        &ctx->shape.geometry,
        shaft_trans,
        0.1f,
        colors->primary
    );
    aff2_position(
        shaft_trans,
        (Vec2){ -0.32426f, -0.14142f },
        (Vec2){ 0.33f, 0.25f },
        -AVEN_MATH_PI_F / 4.0f
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
    AVEN_GL_UI_BUTTON_TYPE_NONE = 0,
    AVEN_GL_UI_BUTTON_TYPE_PLAY,
    AVEN_GL_UI_BUTTON_TYPE_PAUSE,
    AVEN_GL_UI_BUTTON_TYPE_RIGHT,
    AVEN_GL_UI_BUTTON_TYPE_LEFT,
    AVEN_GL_UI_BUTTON_TYPE_RIGHT_ARROW,
    AVEN_GL_UI_BUTTON_TYPE_LEFT_ARROW,
    AVEN_GL_UI_BUTTON_TYPE_FASTFORWARD,
    AVEN_GL_UI_BUTTON_TYPE_REWIND,
    AVEN_GL_UI_BUTTON_TYPE_PLUS,
    AVEN_GL_UI_BUTTON_TYPE_MINUS,
    AVEN_GL_UI_BUTTON_TYPE_THREAD,
    AVEN_GL_UI_BUTTON_TYPE_DOUBLE_THREAD,
    AVEN_GL_UI_BUTTON_TYPE_TRIPLE_THREAD,
    AVEN_GL_UI_BUTTON_TYPE_QUADRA_THREAD,
    AVEN_GL_UI_BUTTON_TYPE_ZOOMIN,
    AVEN_GL_UI_BUTTON_TYPE_ZOOMOUT,
    AVEN_GL_UI_BUTTON_TYPE_CROSS,
    AVEN_GL_UI_BUTTON_TYPE_CHECK,
    AVEN_GL_UI_BUTTON_TYPE_MAX,
} AvenGlUiButtonType;

#define AVEN_GL_UI_WINDOW_ID (-1)

static inline void aven_gl_ui_push_button(
    AvenGlUi *ctx,
    Aff2 trans,
    AvenGlUiButtonType type,
    AvenGlUiColors* colors
) {
    aven_gl_shape_rounded_geometry_push_square(
        &ctx->shape.geometry,
        trans,
        0.15f,
        colors->primary
    );

    Vec2 up = { 0.0f, 1.0f };
    mat2_mul_vec2(up, trans, up);
    Vec2 right = { 1.0f, 0.0f };
    mat2_mul_vec2(right, trans, right);

    float y_padding = 0.01f / vec2_mag(up);
    float x_padding = 0.01f / vec2_mag(right);

    // float aspect_ratio = vec2_mag(right) / vec2_mag(up);
    // float y_scale = 0.85f;
    // float x_scale = 0.85f / aspect_ratio;
    // if (aspect_ratio < 1.0f) {
    //     x_scale = y_scale;
    //     y_scale = x_scale / aspect_ratio;
    // }

    Aff2 inner_trans;
    aff2_identity(inner_trans);
    aff2_stretch(
        inner_trans,
        (Vec2){ 1.0f / (1.0f + x_padding), 1.0f / (1.0f + y_padding) },
        inner_trans
    );
    aff2_compose(inner_trans, trans, inner_trans);
    aven_gl_shape_rounded_geometry_push_square(
        &ctx->shape.geometry,
        inner_trans,
        0.15f,
        colors->background
    );

    switch (type) {
        case AVEN_GL_UI_BUTTON_TYPE_NONE: {
            break;
        }
        case AVEN_GL_UI_BUTTON_TYPE_MAX: {
            assert(false);
            break;
        }
        case AVEN_GL_UI_BUTTON_TYPE_PLAY: {
            aven_gl_ui_push_play(ctx, inner_trans, colors);
            break;
        }
        case AVEN_GL_UI_BUTTON_TYPE_PAUSE: {
            aven_gl_ui_push_pause(ctx, inner_trans, colors);
            break;
        }
        case AVEN_GL_UI_BUTTON_TYPE_RIGHT: {
            aven_gl_ui_push_right(ctx, inner_trans, colors);
            break;
        }
        case AVEN_GL_UI_BUTTON_TYPE_LEFT: {
            aven_gl_ui_push_left(ctx, inner_trans, colors);
            break;
        }
        case AVEN_GL_UI_BUTTON_TYPE_RIGHT_ARROW: {
            aven_gl_ui_push_right_arrow(ctx, inner_trans, colors);
            break;
        }
        case AVEN_GL_UI_BUTTON_TYPE_LEFT_ARROW: {
            aven_gl_ui_push_left_arrow(ctx, inner_trans, colors);
            break;
        }
        case AVEN_GL_UI_BUTTON_TYPE_FASTFORWARD: {
            aven_gl_ui_push_fastforward(ctx, inner_trans, colors);
            break;
        }
        case AVEN_GL_UI_BUTTON_TYPE_REWIND: {
            aven_gl_ui_push_rewind(ctx, inner_trans, colors);
            break;
        }
        case AVEN_GL_UI_BUTTON_TYPE_PLUS: {
            aven_gl_ui_push_plus(ctx, inner_trans, colors);
            break;
        }
        case AVEN_GL_UI_BUTTON_TYPE_MINUS: {
            aven_gl_ui_push_minus(ctx, inner_trans, colors);
            break;
        }
        case AVEN_GL_UI_BUTTON_TYPE_THREAD: {
            aven_gl_ui_push_thread(ctx, inner_trans, colors);
            break;
        }
        case AVEN_GL_UI_BUTTON_TYPE_DOUBLE_THREAD: {
            aven_gl_ui_push_double_thread(ctx, inner_trans, colors);
            break;
        }
        case AVEN_GL_UI_BUTTON_TYPE_TRIPLE_THREAD: {
            aven_gl_ui_push_triple_thread(ctx, inner_trans, colors);
            break;
        }
        case AVEN_GL_UI_BUTTON_TYPE_QUADRA_THREAD: {
            aven_gl_ui_push_quadra_thread(ctx, inner_trans, colors);
            break;
        }
        case AVEN_GL_UI_BUTTON_TYPE_ZOOMIN: {
            aven_gl_ui_push_zoomin(ctx, inner_trans, colors);
            break;
        }
        case AVEN_GL_UI_BUTTON_TYPE_ZOOMOUT: {
            aven_gl_ui_push_zoomout(ctx, inner_trans, colors);
            break;
        }
        case AVEN_GL_UI_BUTTON_TYPE_CROSS: {
            aven_gl_ui_push_cross(ctx, inner_trans, colors);
            break;
        }
        case AVEN_GL_UI_BUTTON_TYPE_CHECK: {
            aven_gl_ui_push_check(ctx, inner_trans, colors);
            break;
        }
    }
}

static inline void aven_gl_ui_button_disabled(
    AvenGlUi *ctx,
    Aff2 trans,
    AvenGlUiButtonType type
) {
    aven_gl_ui_push_button(
        ctx,
        trans,
        type,
        &ctx->disabled_colors
    );
}

static inline bool aven_gl_ui_button_internal(
    AvenGlUi *ctx,
    AvenGlUiId id,
    Aff2 trans,
    AvenGlUiButtonType type
) {
    Vec2 p1 = { -1.0f, -1.0f };
    Vec2 p2 = { 1.0f, 1.0f };

    aff2_transform(p1, trans, p1);
    aff2_transform(p2, trans, p2);

    bool hot = aven_gl_ui_id_eq(id, ctx->hot_id);

    bool clicked = aven_gl_ui_id_eq(id, ctx->active_id);

    if (vec2_point_in_rect(p1, p2, ctx->mouse.pos)) {
        ctx->next_hot_id = id;
    }

    if (hot and ctx->mouse.event == AVEN_GL_UI_MOUSE_EVENT_DOWN) {
        ctx->active_id = id;
    }

    bool value = false;

    if (clicked and ctx->mouse.event == AVEN_GL_UI_MOUSE_EVENT_UP) {
        if (hot) {
            value = true;
            ctx->empty_click = false;
        }
        ctx->active_id = (AvenGlUiId){ 0 };
    }

    AvenGlUiColors *colors = &ctx->base_colors;
    if (clicked) {
        colors = &ctx->clicked_colors;
    } else if (hot) {
        colors = &ctx->hovered_colors;
    }

    aven_gl_ui_push_button(ctx, trans, type, colors);

    return value;
}

#define aven_gl_ui_button(ctx, trans, type) aven_gl_ui_button_internal( \
        ctx, \
        aven_gl_ui_id_internal(__FILE__, type, __LINE__), \
        trans, \
        type \
    )

#define aven_gl_ui_button_uid(ctx, trans, type, uid) \
    aven_gl_ui_button_internal( \
        ctx, \
        aven_gl_ui_id_internal( \
            __FILE__, \
            AVEN_GL_UI_BUTTON_TYPE_MAX + uid, \
            __LINE__ \
        ), \
        trans, \
        type \
    )

static inline bool aven_gl_ui_button_maybe_internal(
    AvenGlUi *ctx,
    AvenGlUiId id,
    Aff2 trans,
    AvenGlUiButtonType type,
    bool enabled
) {
    if (enabled) {
        return aven_gl_ui_button_internal(ctx, id, trans, type);
    }
    aven_gl_ui_button_disabled(ctx, trans, type);
    return false;
}

#define aven_gl_ui_button_maybe(ctx, trans, type, enabled) \
    aven_gl_ui_button_maybe_internal( \
        ctx, \
        aven_gl_ui_id_internal(__FILE__, type, __LINE__), \
        trans, \
        type, \
        enabled \
    )
#define aven_gl_ui_button_maybe_uid(ctx, trans, type, uid) \
    aven_gl_ui_button_internal( \
        ctx, \
        aven_gl_ui_id_internal( \
            __FILE__, \
            AVEN_GL_UI_BUTTON_TYPE_MAX + uid, \
            __LINE__ \
        ), \
        trans, \
        type, \
        enabled \
    )

static inline bool aven_gl_ui_window_internal(
    AvenGlUi *ctx,
    AvenGlUiId id,
    Aff2 trans
) {
    Vec2 p1 = { -1.0f, -1.0f };
    Vec2 p2 = { 1.0f, 1.0f };

    aff2_transform(p1, trans, p1);
    aff2_transform(p2, trans, p2);

    bool hot = aven_gl_ui_id_eq(id, ctx->hot_id);

    bool clicked = aven_gl_ui_id_eq(id, ctx->active_id);

    if (vec2_point_in_rect(p1, p2, ctx->mouse.pos)) {
        ctx->next_hot_id = id;
    }

    if (hot and ctx->mouse.event == AVEN_GL_UI_MOUSE_EVENT_DOWN) {
        ctx->active_id = id;
    }

    bool value = false;

    if (clicked and ctx->mouse.event == AVEN_GL_UI_MOUSE_EVENT_UP) {
        if (hot) {
            value = true;
            ctx->empty_click = false;
        }
        ctx->active_id = (AvenGlUiId){ 0 };
    }

    aven_gl_shape_rounded_geometry_push_square(
        &ctx->shape.geometry,
        trans,
        0.0f,
        ctx->disabled_colors.background
    );

    return value;    
}

#define aven_gl_ui_window(ctx, trans) aven_gl_ui_window_internal( \
        ctx, \
        aven_gl_ui_id_internal(__FILE__, AVEN_GL_UI_WINDOW_ID, __LINE__), \
        trans \
    )

#endif
