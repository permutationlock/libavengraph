#ifndef AVEN_GL_UI_H
#define AVEN_GL_UI_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/math.h>

#include "../gl.h"
#include "shape.h"
#include "text.h"

#define AVEN_GL_UI_KEYBOARD_MAX (348)

typedef void (*AvenGlUiCallbackFn)(void *);

typedef struct {
    uintptr_t parent;
    uintptr_t unique;
    size_t index;
} AvenGlUiId;

#define aven_gl_ui_id_internal(p, u, i) (AvenGlUiId){ \
        .file = (uintptr_t)(p), \
        .unique = (uintptr_t)(u), \
        .line = (size_t)(i), \
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
    AvenGlShapeCtx ctx;
    AvenGlShapeGeometry geometry;
    AvenGlShapeBuffer buffer;
} AvenGlUiShape;

typedef struct {
    AvenGlTextFont *font;
    AvenGlTextCtx ctx;
    AvenGlTextGeometry geometry;
    AvenGlTextBuffer buffer;
} AvenGlUiText;

typedef struct {
    float x;
    float y;
    enum {
        AVEN_GL_UI_IO_MOUSE_NONE = 0,
        AVEN_GL_UI_IO_MOUSE_DOWN,
        AVEN_GL_UI_IO_MOUSE_UP,
    } event;
} AvenGlUiIoMouse;

typedef struct {
    enum {
        AVEN_GL_UI_IO_KEYBOARD_NONE = 0,
        AVEN_GL_UI_IO_KEYBOARD_DOWN,
        AVEN_GL_UI_IO_KEYBOARD_UP,
    } keys[AVEN_GL_UI_KEYBOARD_MAX];
} AvenGlUiIoKeyboard;

typedef struct {
    AvenGlUiShape shape;
    AvenGlUiText text;
    AvenGlUiIoMouse mouse;
    AvenGlUiIoKeyboard keyboard;
    AvenGlUiId active_id;
    AvenGlUiId hot_id;
    uint32_t count;
} AvenGlUiCtx;

static inline aven_gl_ui_ctx_init(
    AvenGl *gl,
    AvenGlTextFont *font,
    size_t max_elems,
    size_t max_chars,
    AvenArena *arena
) {
    AvenGlUiCtx ctx = { 0 };

    ctx.shape.ctx = aven_gl_shape_ctx_init(gl);
    ctx.shape.geometry = aven_gl_shape_geometry_init(
        4 * max_elems,
        6 * max_elems,
        arena
    );
    ctx.shape.buffer = aven_gl_shape_buffer_init(
        gl,
        &ctx.shape.geometry,
        AVEN_GL_BUFFER_USAGE_DYNAMIC
    );

    ctx.text.ctx = aven_gl_text_ctx_init(gl);
    ctx.text.geometry = aven_gl_text_geometry_init(
        max_chars,
        arena
    );
    ctx.text.buffer = aven_gl_text_buffer_init(
        gl,
        &ctx.text.geometry,
        AVEN_GL_BUFFER_USAGE_DYNAMIC
    );
    ctx.text.font = *font;

    return ctx;
}

static inline void aven_gl_ui_ctx_deinit(AvenGl *gl, AvenGlUiCtx *ctx) {
    aven_gl_text_buffer_deinit(gl, &ctx.text.buffer);
    aven_gl_text_geometry_deinit(&ctx.text.geometry);
    aven_gl_text_ctx_deinit(gl, &ctx.text.ctx);

    aven_gl_shape_buffer_deinit(gl, &ctx.shape.buffer);
    aven_gl_shape_geometry_deinit(&ctx.shape.geometry);
    aven_gl_shape_ctx_deinit(gl, &ctx.shape.ctx);
}

static inline bool aven_gl_ui_button_internal(
    AvenGlUiCtx *ctx,
    AvenGlUiId id,
    AvenStr name,
    Vec2 pos
) {
    if (aven_gl_ui_id_eq(id, ctx->active_id)) {
        if (ctx->mouse.pos)
    }
}

#define aven_gl_ui_button(ctx, name) aven_gl_ui_button_internal( \
        ctx, \
        aven_gl_ui_id_internal(__FILE__, (name).ptr, __LINE__), \
        name, \
        pos \
    )

