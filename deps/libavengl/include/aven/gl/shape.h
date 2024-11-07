#ifndef AVEN_GL_SHAPE_H
#define AVEN_GL_SHAPE_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/math.h>

#include "../gl.h"

typedef struct {
    Vec4 color;
    Vec2 pos;
} AvenGlShapeVertex;

typedef struct {
    List(AvenGlShapeVertex) vertices;
    List(GLushort) indices;
} AvenGlShapeGeometry;

typedef struct {
    GLuint vertex_shader;
    GLuint fragment_shader;
    GLuint program;
    GLuint utrans_location;
    GLuint upos_location;
    GLuint vpos_location;
    GLuint vcolor_location;
} AvenGlShapeCtx;

typedef struct {
    size_t vertex_cap;
    size_t index_cap;
    size_t index_len;
    GLuint vertex;
    GLuint index;
    AvenGlBufferUsage usage;
} AvenGlShapeBuffer;

static inline AvenGlShapeGeometry aven_gl_shape_geometry_init(
    size_t max_vertices,
    size_t max_indices,
    AvenArena *arena
) {
    assert(max_indices >= max_vertices);

    AvenGlShapeGeometry geometry = {
        .vertices = { .cap = max_vertices },
        .indices = { .cap = max_indices },
    };
    geometry.vertices.ptr = aven_arena_create_array(
        AvenGlShapeVertex,
        arena,
        geometry.vertices.cap
    );
    geometry.indices.ptr = aven_arena_create_array(
        GLushort,
        arena,
        geometry.indices.cap
    );

    return geometry;
}

static inline void aven_gl_shape_geometry_clear(AvenGlShapeGeometry *geometry) {
    geometry->vertices.len = 0;
    geometry->indices.len = 0;
}

static inline void aven_gl_shape_geometry_deinit(
    AvenGlShapeGeometry *geometry
) {
    *geometry = (AvenGlShapeGeometry){ 0 };
}

static inline AvenGlShapeCtx aven_gl_shape_ctx_init(AvenGl *gl) {
    AvenGlShapeCtx ctx = { 0 };

    static const char* vertex_shader_text =
        "#version 100\n"
        "precision mediump float;\n"
        "uniform mat2 uTrans;\n"
        "uniform vec2 uPos;\n"
        "attribute vec2 vPos;\n"
        "attribute vec4 vColor;\n"
        "varying vec4 fColor;\n"
        "void main() {\n"
        "    gl_Position = vec4((uTrans * vPos.xy) + uPos, 0.0, 1.0);\n"
        "    fColor = vColor;\n"
        "}\n";

    static const char* fragment_shader_text =
        "#version 100\n"
        "precision mediump float;\n"
        "varying vec4 fColor;\n"
        "void main() {\n"
        "    gl_FragColor = fColor;\n"
        "}\n";

    ctx.vertex_shader = gl->CreateShader(GL_VERTEX_SHADER);
    assert(gl->GetError() == 0);
    gl->ShaderSource(ctx.vertex_shader, 1, &vertex_shader_text, NULL);
    assert(gl->GetError() == 0);
    gl->CompileShader(ctx.vertex_shader);
    assert(gl->GetError() == 0);

    ctx.fragment_shader = gl->CreateShader(GL_FRAGMENT_SHADER);
    assert(gl->GetError() == 0);
    gl->ShaderSource(ctx.fragment_shader, 1, &fragment_shader_text, NULL);
    assert(gl->GetError() == 0);
    gl->CompileShader(ctx.fragment_shader);
    assert(gl->GetError() == 0);

    ctx.program = gl->CreateProgram();
    assert(gl->GetError() == 0);
    gl->AttachShader(ctx.program, ctx.vertex_shader);
    assert(gl->GetError() == 0);
    gl->AttachShader(ctx.program, ctx.fragment_shader);
    assert(gl->GetError() == 0);
    gl->LinkProgram(ctx.program);
    assert(gl->GetError() == 0);

    ctx.utrans_location = (GLuint)gl->GetUniformLocation(
        ctx.program,
        "uTrans"
    );
    assert(gl->GetError() == 0);
    ctx.upos_location = (GLuint)gl->GetUniformLocation(
        ctx.program,
        "uPos"
    );
    assert(gl->GetError() == 0);

    ctx.vpos_location = (GLuint)gl->GetAttribLocation(
        ctx.program,
        "vPos"
    );
    ctx.vcolor_location = (GLuint)gl->GetAttribLocation(
        ctx.program,
        "vColor"
    );
    assert(gl->GetError() == 0);

    return ctx;
}

static inline void aven_gl_shape_ctx_deinit(AvenGl *gl, AvenGlShapeCtx *ctx) {
    gl->DeleteProgram(ctx->program);
    gl->DeleteShader(ctx->fragment_shader);
    gl->DeleteShader(ctx->vertex_shader);
    *ctx = (AvenGlShapeCtx){ 0 };
}

static inline AvenGlShapeBuffer aven_gl_shape_buffer_init(
    AvenGl *gl,
    AvenGlShapeGeometry *geometry,
    AvenGlBufferUsage buffer_usage
) {
    AvenGlShapeBuffer buffer = { .usage = buffer_usage };

    switch (buffer_usage) {
        case AVEN_GL_BUFFER_USAGE_DYNAMIC:
            buffer.vertex_cap = geometry->vertices.cap *
                sizeof(*geometry->vertices.ptr);
            buffer.index_cap = geometry->indices.cap *
                sizeof(*geometry->indices.ptr);
            break;
        case AVEN_GL_BUFFER_USAGE_STATIC:
        case AVEN_GL_BUFFER_USAGE_STREAM:
            buffer.vertex_cap = geometry->vertices.len *
                sizeof(*geometry->vertices.ptr);
            buffer.index_cap = geometry->indices.len *
                sizeof(*geometry->indices.ptr);
            buffer.index_len = buffer.index_len;
            break;
        default:
            assert(false);
    }

    gl->GenBuffers(1, &buffer.vertex);
    assert(gl->GetError() == 0);
    gl->BindBuffer(GL_ARRAY_BUFFER, buffer.vertex);
    assert(gl->GetError() == 0);
    gl->BufferData(
        GL_ARRAY_BUFFER,
        (GLsizeiptr)buffer.vertex_cap,
        geometry->vertices.ptr,
        (GLenum)buffer_usage
    );
    assert(gl->GetError() == 0);

    gl->GenBuffers(1, &buffer.index);
    assert(gl->GetError() == 0);
    gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer.index);
    assert(gl->GetError() == 0);
    gl->BufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        (GLsizeiptr)buffer.index_cap,
        geometry->indices.ptr,
        (GLenum)buffer_usage
    );
    assert(gl->GetError() == 0);

    gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    assert(gl->GetError() == 0);
    gl->BindBuffer(GL_ARRAY_BUFFER, 0);
    assert(gl->GetError() == 0);

    return buffer;
}

static inline void aven_gl_shape_buffer_deinit(
    AvenGl *gl,
    AvenGlShapeBuffer *buffer
) {
    gl->DeleteBuffers(1, &buffer->index);
    gl->DeleteBuffers(1, &buffer->vertex);
    *buffer = (AvenGlShapeBuffer){ 0 };
}

static inline void aven_gl_shape_buffer_update(
    AvenGl *gl,
    AvenGlShapeBuffer *buffer,
    AvenGlShapeGeometry *geometry
) {
    assert(buffer->usage == AVEN_GL_BUFFER_USAGE_DYNAMIC);
    assert(geometry->vertices.len < buffer->vertex_cap);
    assert(geometry->indices.len < buffer->index_cap);

    gl->BindBuffer(GL_ARRAY_BUFFER, buffer->vertex);
    assert(gl->GetError() == 0);
    gl->BufferSubData(
        GL_ARRAY_BUFFER,
        0,
        (GLsizeiptr)(sizeof(*geometry->vertices.ptr) * geometry->vertices.len),
        geometry->vertices.ptr
    );
    assert(gl->GetError() == 0);

    gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->index);
    assert(gl->GetError() == 0);
    gl->BufferSubData(
        GL_ELEMENT_ARRAY_BUFFER,
        0,
        (GLsizeiptr)(sizeof(*geometry->indices.ptr) * geometry->indices.len),
        geometry->indices.ptr
    );
    assert(gl->GetError() == 0);

    buffer->index_len = geometry->indices.len;

    gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    assert(gl->GetError() == 0);
    gl->BindBuffer(GL_ARRAY_BUFFER, 0);
    assert(gl->GetError() == 0);
}

static inline void aven_gl_shape_draw(
    AvenGl *gl,
    AvenGlShapeCtx *ctx,
    AvenGlShapeBuffer *buffer,
    Aff2 cam_trans
) {
    gl->BindBuffer(GL_ARRAY_BUFFER, buffer->vertex);
    assert(gl->GetError() == 0);
    gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->index);
    assert(gl->GetError() == 0);

    gl->UseProgram(ctx->program);
    assert(gl->GetError() == 0);

    gl->EnableVertexAttribArray(ctx->vpos_location);
    assert(gl->GetError() == 0);
    gl->EnableVertexAttribArray(ctx->vcolor_location);
    assert(gl->GetError() == 0);
    gl->VertexAttribPointer(
        ctx->vpos_location,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(AvenGlShapeVertex),
        (void*)offsetof(AvenGlShapeVertex, pos)
    );
    assert(gl->GetError() == 0);
    gl->VertexAttribPointer(
        ctx->vcolor_location,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(AvenGlShapeVertex),
        (void*)offsetof(AvenGlShapeVertex, color)
    );
    assert(gl->GetError() == 0);
    gl->Enable(GL_BLEND);
    assert(gl->GetError() == 0);
    gl->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    assert(gl->GetError() == 0);

    gl->UniformMatrix2fv(
        (GLint)ctx->utrans_location,
        1,
        GL_FALSE,
        (GLfloat*)cam_trans
    );
    assert(gl->GetError() == 0);
    gl->Uniform2fv(
        (GLint)ctx->upos_location,
        1,
        (GLfloat*)cam_trans[2]
    );
    assert(gl->GetError() == 0);

    gl->DrawElements(
        GL_TRIANGLES,
        (GLsizei)buffer->index_len,
        GL_UNSIGNED_SHORT,
        0
    );
    assert(gl->GetError() == 0);

    gl->Disable(GL_BLEND);
    assert(gl->GetError() == 0);
    gl->DisableVertexAttribArray(ctx->vcolor_location);
    assert(gl->GetError() == 0);
    gl->DisableVertexAttribArray(ctx->vpos_location);
    assert(gl->GetError() == 0);

    gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    assert(gl->GetError() == 0);
    gl->BindBuffer(GL_ARRAY_BUFFER, 0);
    assert(gl->GetError() == 0);
}

static inline void aven_gl_shape_geometry_push_triangle(
    AvenGlShapeGeometry *geometry,
    Aff2 trans,
    Vec2 p1,
    Vec2 p2,
    Vec2 p3,
    Vec4 color
) {
    aff2_transform(p1, trans, p1);
    aff2_transform(p2, trans, p2);
    aff2_transform(p3, trans, p3);

    size_t start_index = geometry->vertices.len;

    list_push(geometry->vertices) = (AvenGlShapeVertex){
        .pos = { p1[0], p1[1] },
        .color = { color[0], color[1], color[2], color[3] },
    };
    list_push(geometry->vertices) = (AvenGlShapeVertex){
        .pos = { p2[0], p2[1] },
        .color = { color[0], color[1], color[2], color[3] },
    };
    list_push(geometry->vertices) = (AvenGlShapeVertex){
        .pos = { p3[0], p3[1] },
        .color = { color[0], color[1], color[2], color[3] },
    };

    list_push(geometry->indices) = (GLushort)start_index + 0;
    list_push(geometry->indices) = (GLushort)start_index + 1;
    list_push(geometry->indices) = (GLushort)start_index + 2;
}

static inline void aven_gl_shape_geometry_push_triangle_isoceles(
    AvenGlShapeGeometry *geometry,
    Aff2 trans,
    Vec4 color
) {
    Vec2 p1 = {  0.0f,  1.0f };
    Vec2 p2 = {  1.0f, -1.0f };
    Vec2 p3 = { -1.0f, -1.0f };

    aven_gl_shape_geometry_push_triangle(
        geometry,
        trans,
        p1,
        p2,
        p3,
        color
    );
}

static inline void aven_gl_shape_geometry_push_triangle_right(
    AvenGlShapeGeometry *geometry,
    Aff2 trans,
    Vec4 color
) {
    Vec2 p1 = { -1.0f,  1.0f };
    Vec2 p2 = {  1.0f, -1.0f };
    Vec2 p3 = { -1.0f, -1.0f };

    aven_gl_shape_geometry_push_triangle(
        geometry,
        trans,
        p1,
        p2,
        p3,
        color
    );
}

static inline void aven_gl_shape_geometry_push_square(
    AvenGlShapeGeometry *geometry,
    Aff2 trans,
    Vec4 color
) {
    Vec2 p1 = { -1.0f, -1.0f };
    Vec2 p2 = {  1.0f, -1.0f };
    Vec2 p3 = {  1.0f,  1.0f };
    Vec2 p4 = { -1.0f,  1.0f };

    aff2_transform(p1, trans, p1);
    aff2_transform(p2, trans, p2);
    aff2_transform(p3, trans, p3);
    aff2_transform(p4, trans, p4);

    size_t start_index = geometry->vertices.len;

    list_push(geometry->vertices) = (AvenGlShapeVertex){
        .pos = { p1[0], p1[1] },
        .color = { color[0], color[1], color[2], color[3] },
    };
    list_push(geometry->vertices) = (AvenGlShapeVertex){
        .pos = { p2[0], p2[1] },
        .color = { color[0], color[1], color[2], color[3] },
    };
    list_push(geometry->vertices) = (AvenGlShapeVertex){
        .pos = { p3[0], p3[1] },
        .color = { color[0], color[1], color[2], color[3] },
    };
    list_push(geometry->vertices) = (AvenGlShapeVertex){
        .pos = { p4[0], p4[1] },
        .color = { color[0], color[1], color[2], color[3] },
    };

    list_push(geometry->indices) = (GLushort)start_index + 0;
    list_push(geometry->indices) = (GLushort)start_index + 1;
    list_push(geometry->indices) = (GLushort)start_index + 2;
    list_push(geometry->indices) = (GLushort)start_index + 0;
    list_push(geometry->indices) = (GLushort)start_index + 2;
    list_push(geometry->indices) = (GLushort)start_index + 3;
}

// Rounded

typedef struct {
    Vec4 color;
    Vec4 info;
    Vec2 pos;
} AvenGlShapeRoundedVertex;

typedef struct {
    List(AvenGlShapeRoundedVertex) vertices;
    List(GLushort) indices;
} AvenGlShapeRoundedGeometry;

typedef struct {
    size_t vertex_cap;
    size_t index_cap;
    size_t index_len;
    GLuint vertex;
    GLuint index;
    AvenGlBufferUsage usage;
} AvenGlShapeRoundedBuffer;

typedef struct {
    GLuint vertex_shader;
    GLuint fragment_shader;
    GLuint program;
    GLuint utrans_location;
    GLuint upos_location;
    GLuint upx_location;
    GLuint vpos_location;
    GLuint vinfo_location;
    GLuint vcolor_location;
} AvenGlShapeRoundedCtx;

static inline AvenGlShapeRoundedGeometry aven_gl_shape_rounded_geometry_init(
    size_t max_vertices,
    size_t max_indices,
    AvenArena *arena
) {
    assert(max_indices >= max_vertices);

    AvenGlShapeRoundedGeometry geometry = {
        .vertices = { .cap = max_vertices },
        .indices = { .cap = max_indices },
    };
    geometry.vertices.ptr = aven_arena_create_array(
        AvenGlShapeRoundedVertex,
        arena,
        geometry.vertices.cap
    );
    geometry.indices.ptr = aven_arena_create_array(
        GLushort,
        arena,
        geometry.indices.cap
    );

    return geometry;
}

static inline void aven_gl_shape_rounded_geometry_clear(
    AvenGlShapeRoundedGeometry *geometry
) {
    geometry->vertices.len = 0;
    geometry->indices.len = 0;
}

static inline void aven_gl_shape_rounded_geometry_deinit(
    AvenGlShapeRoundedGeometry *geometry
) {
    *geometry = (AvenGlShapeRoundedGeometry){ 0 };
}

static inline AvenGlShapeRoundedCtx aven_gl_shape_rounded_ctx_init(AvenGl *gl) {
    AvenGlShapeRoundedCtx ctx = { 0 };

    static const char* vertex_shader_text =
        "#version 100\n"
        "precision mediump float;\n"
        "uniform mat2 uTrans;\n"
        "uniform vec2 uPos;\n"
        "attribute vec2 vPos;\n"
        "attribute vec4 vInfo;\n"
        "attribute vec4 vColor;\n"
        "varying vec2 tPos;\n"
        "varying vec2 tDim;\n"
        "varying vec4 fColor;\n"
        "void main() {\n"
        "    gl_Position = vec4((uTrans * vPos.xy) + uPos, 0.0, 1.0);\n"
        "    tPos = vInfo.xy;\n"
        "    tDim = vInfo.zw;\n"
        "    fColor = vColor;\n"
        "}\n";

    static const char* fragment_shader_text =
        "#version 100\n"
        "precision mediump float;\n"
        "uniform float uPx;\n"
        "varying vec2 tPos;\n"
        "varying vec2 tDim;\n"
        "varying vec4 fColor;\n"
        "float oversample(vec2 p) {\n"
        "    if (dot(p, p) > 1.0) {\n"
        "        return 0.0;\n"
        "    }\n"
        "    return 1.0;\n"
        "}\n"
        "void main() {\n"
        "    float magnitude = 0.0;\n"
        "    vec2 offset = vec2(uPx / tDim.x, uPx / tDim.y);\n"
        "    magnitude += oversample(tPos);\n"
        "    magnitude += oversample(tPos + vec2(0, -offset.y));\n"
        "    magnitude += oversample(tPos + vec2(offset.x, 0));\n"
        "    magnitude += oversample(tPos + vec2(0, offset.y));\n"
        "    magnitude += oversample(tPos + vec2(-offset.x, 0));\n"
        "    magnitude += oversample(tPos + vec2(-offset.x, offset.y));\n"
        "    magnitude += oversample(tPos + vec2(-offset.x, -offset.y));\n"
        "    magnitude += oversample(tPos + vec2(offset.x, -offset.y));\n"
        "    magnitude += oversample(tPos + vec2(offset.x, offset.y));\n"
        "    magnitude /= 9.0;\n"
        "    gl_FragColor = vec4(fColor.xyz, fColor.w * magnitude);\n"
        "}\n";

    ctx.vertex_shader = gl->CreateShader(GL_VERTEX_SHADER);
    assert(gl->GetError() == 0);
    gl->ShaderSource(ctx.vertex_shader, 1, &vertex_shader_text, NULL);
    assert(gl->GetError() == 0);
    gl->CompileShader(ctx.vertex_shader);
    assert(gl->GetError() == 0);

    ctx.fragment_shader = gl->CreateShader(GL_FRAGMENT_SHADER);
    assert(gl->GetError() == 0);
    gl->ShaderSource(ctx.fragment_shader, 1, &fragment_shader_text, NULL);
    assert(gl->GetError() == 0);
    gl->CompileShader(ctx.fragment_shader);
    assert(gl->GetError() == 0);

    ctx.program = gl->CreateProgram();
    assert(gl->GetError() == 0);
    gl->AttachShader(ctx.program, ctx.vertex_shader);
    assert(gl->GetError() == 0);
    gl->AttachShader(ctx.program, ctx.fragment_shader);
    assert(gl->GetError() == 0);
    gl->LinkProgram(ctx.program);
    assert(gl->GetError() == 0);

    ctx.utrans_location = (GLuint)gl->GetUniformLocation(
        ctx.program,
        "uTrans"
    );
    assert(gl->GetError() == 0);
    ctx.upos_location = (GLuint)gl->GetUniformLocation(
        ctx.program,
        "uPos"
    );
    assert(gl->GetError() == 0);
    ctx.upx_location = (GLuint)gl->GetUniformLocation(
        ctx.program,
        "uPx"
    );
    assert(gl->GetError() == 0);

    ctx.vpos_location = (GLuint)gl->GetAttribLocation(
        ctx.program,
        "vPos"
    );
    ctx.vinfo_location = (GLuint)gl->GetAttribLocation(
        ctx.program,
        "vInfo"
    );
    assert(gl->GetError() == 0);
    ctx.vcolor_location = (GLuint)gl->GetAttribLocation(
        ctx.program,
        "vColor"
    );
    assert(gl->GetError() == 0);

    return ctx;
}

static inline void aven_gl_shape_rounded_ctx_deinit(AvenGl *gl, AvenGlShapeRoundedCtx *ctx) {
    gl->DeleteProgram(ctx->program);
    gl->DeleteShader(ctx->fragment_shader);
    gl->DeleteShader(ctx->vertex_shader);
    *ctx = (AvenGlShapeRoundedCtx){ 0 };
}

static inline AvenGlShapeRoundedBuffer aven_gl_shape_rounded_buffer_init(
    AvenGl *gl,
    AvenGlShapeRoundedGeometry *geometry,
    AvenGlBufferUsage buffer_usage
) {
    AvenGlShapeRoundedBuffer buffer = { .usage = buffer_usage };

    switch (buffer_usage) {
        case AVEN_GL_BUFFER_USAGE_DYNAMIC:
            buffer.vertex_cap = geometry->vertices.cap *
                sizeof(*geometry->vertices.ptr);
            buffer.index_cap = geometry->indices.cap *
                sizeof(*geometry->indices.ptr);
            break;
        case AVEN_GL_BUFFER_USAGE_STATIC:
        case AVEN_GL_BUFFER_USAGE_STREAM:
            buffer.vertex_cap = geometry->vertices.len *
                sizeof(*geometry->vertices.ptr);
            buffer.index_cap = geometry->indices.len *
                sizeof(*geometry->indices.ptr);
            buffer.index_len = buffer.index_len;
            break;
        default:
            assert(false);
    }

    gl->GenBuffers(1, &buffer.vertex);
    assert(gl->GetError() == 0);
    gl->BindBuffer(GL_ARRAY_BUFFER, buffer.vertex);
    assert(gl->GetError() == 0);
    gl->BufferData(
        GL_ARRAY_BUFFER,
        (GLsizeiptr)buffer.vertex_cap,
        geometry->vertices.ptr,
        (GLenum)buffer_usage
    );
    assert(gl->GetError() == 0);

    gl->GenBuffers(1, &buffer.index);
    assert(gl->GetError() == 0);
    gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer.index);
    assert(gl->GetError() == 0);
    gl->BufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        (GLsizeiptr)buffer.index_cap,
        geometry->indices.ptr,
        (GLenum)buffer_usage
    );
    assert(gl->GetError() == 0);

    gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    assert(gl->GetError() == 0);
    gl->BindBuffer(GL_ARRAY_BUFFER, 0);
    assert(gl->GetError() == 0);

    return buffer;
}

static inline void aven_gl_shape_rounded_buffer_deinit(
    AvenGl *gl,
    AvenGlShapeRoundedBuffer *buffer
) {
    gl->DeleteBuffers(1, &buffer->index);
    gl->DeleteBuffers(1, &buffer->vertex);
    *buffer = (AvenGlShapeRoundedBuffer){ 0 };
}

static inline void aven_gl_shape_rounded_buffer_update(
    AvenGl *gl,
    AvenGlShapeRoundedBuffer *buffer,
    AvenGlShapeRoundedGeometry *geometry
) {
    assert(buffer->usage == AVEN_GL_BUFFER_USAGE_DYNAMIC);
    assert(geometry->vertices.len < buffer->vertex_cap);
    assert(geometry->indices.len < buffer->index_cap);

    gl->BindBuffer(GL_ARRAY_BUFFER, buffer->vertex);
    assert(gl->GetError() == 0);
    gl->BufferSubData(
        GL_ARRAY_BUFFER,
        0,
        (GLsizeiptr)(sizeof(*geometry->vertices.ptr) * geometry->vertices.len),
        geometry->vertices.ptr
    );
    assert(gl->GetError() == 0);

    gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->index);
    assert(gl->GetError() == 0);
    gl->BufferSubData(
        GL_ELEMENT_ARRAY_BUFFER,
        0,
        (GLsizeiptr)(sizeof(*geometry->indices.ptr) * geometry->indices.len),
        geometry->indices.ptr
    );
    assert(gl->GetError() == 0);

    buffer->index_len = geometry->indices.len;

    gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    assert(gl->GetError() == 0);
    gl->BindBuffer(GL_ARRAY_BUFFER, 0);
    assert(gl->GetError() == 0);
}

static inline void aven_gl_shape_rounded_draw(
    AvenGl *gl,
    AvenGlShapeRoundedCtx *ctx,
    AvenGlShapeRoundedBuffer *buffer,
    float pixel_size,
    Aff2 cam_trans
) {
    gl->BindBuffer(GL_ARRAY_BUFFER, buffer->vertex);
    assert(gl->GetError() == 0);
    gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer->index);
    assert(gl->GetError() == 0);

    gl->UseProgram(ctx->program);
    assert(gl->GetError() == 0);

    gl->EnableVertexAttribArray(ctx->vpos_location);
    assert(gl->GetError() == 0);
    gl->EnableVertexAttribArray(ctx->vinfo_location);
    assert(gl->GetError() == 0);
    gl->EnableVertexAttribArray(ctx->vcolor_location);
    assert(gl->GetError() == 0);
    gl->VertexAttribPointer(
        ctx->vpos_location,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(AvenGlShapeRoundedVertex),
        (void*)offsetof(AvenGlShapeRoundedVertex, pos)
    );
    assert(gl->GetError() == 0);
    gl->VertexAttribPointer(
        ctx->vinfo_location,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(AvenGlShapeRoundedVertex),
        (void*)offsetof(AvenGlShapeRoundedVertex, info)
    );
    assert(gl->GetError() == 0);
    gl->VertexAttribPointer(
        ctx->vcolor_location,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(AvenGlShapeRoundedVertex),
        (void*)offsetof(AvenGlShapeRoundedVertex, color)
    );
    assert(gl->GetError() == 0);
    gl->Enable(GL_BLEND);
    assert(gl->GetError() == 0);
    gl->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    assert(gl->GetError() == 0);

    gl->UniformMatrix2fv(
        (GLint)ctx->utrans_location,
        1,
        GL_FALSE,
        (GLfloat*)cam_trans
    );
    assert(gl->GetError() == 0);
    gl->Uniform2fv(
        (GLint)ctx->upos_location,
        1,
        (GLfloat*)cam_trans[2]
    );
    assert(gl->GetError() == 0);
    gl->Uniform1fv(
        (GLint)ctx->upx_location,
        1,
        (GLfloat*)&pixel_size
    );
    assert(gl->GetError() == 0);

    gl->DrawElements(
        GL_TRIANGLES,
        (GLsizei)buffer->index_len,
        GL_UNSIGNED_SHORT,
        0
    );
    assert(gl->GetError() == 0);

    gl->Disable(GL_BLEND);
    assert(gl->GetError() == 0);
    gl->DisableVertexAttribArray(ctx->vcolor_location);
    assert(gl->GetError() == 0);
    gl->DisableVertexAttribArray(ctx->vinfo_location);
    assert(gl->GetError() == 0);
    gl->DisableVertexAttribArray(ctx->vpos_location);
    assert(gl->GetError() == 0);

    gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    assert(gl->GetError() == 0);
    gl->BindBuffer(GL_ARRAY_BUFFER, 0);
    assert(gl->GetError() == 0);
}

static inline void aven_gl_shape_rounded_geometry_push_sector(
    AvenGlShapeRoundedGeometry *geometry,
    Aff2 trans,
    float start_angle,
    float stop_angle,
    Vec4 color
) {
    assert((stop_angle - start_angle) < AVEN_MATH_PI_F);

    float angle = stop_angle - start_angle;

    Vec2 tex_points[3];
    vec2_copy(tex_points[0], (Vec2){ 0.0f, 0.0f });
    vec2_copy(tex_points[1], (Vec2){ 1.0f, 0.0f });
    vec2_copy(tex_points[2], (Vec2){ cosf(angle), sinf(angle) });

    Vec2 height_vec;
    mat2_mul_vec2(height_vec, trans, (Vec2){ 0.0f, tex_points[2][1] });

    Vec2 width_vec;
    mat2_mul_vec2(width_vec, trans, (Vec2){ 1.0f, 0.0f });

    float width = 2.0f * vec2_mag(width_vec);
    float height = 2.0f * vec2_mag(height_vec);

    Vec2 midpoint;
    vec2_add(midpoint, tex_points[1], tex_points[2]);
    vec2_scale(midpoint, 0.5f, midpoint);
    float scale = vec2_mag(midpoint);

    vec2_scale(tex_points[1], 1.0f / scale, tex_points[1]);
    vec2_scale(tex_points[2], 1.0f / scale, tex_points[2]);
    vec2_scale(midpoint, scale, midpoint);

    Vec2 points[3];

    Mat2 rot;
    mat2_identity(rot);
    mat2_rotate(rot, rot, start_angle);

    mat2_mul_vec2(points[0], rot, tex_points[0]);
    mat2_mul_vec2(points[1], rot, tex_points[1]);
    mat2_mul_vec2(points[2], rot, tex_points[2]);

    aff2_transform(points[0], trans, points[0]);
    aff2_transform(points[1], trans, points[1]);
    aff2_transform(points[2], trans, points[2]);

    size_t start_index = geometry->vertices.len;

    for (size_t i = 0; i < countof(points); i += 1) {
        list_push(geometry->vertices) = (AvenGlShapeRoundedVertex){
            .pos = { points[i][0], points[i][1] },
            .info = { tex_points[i][0], tex_points[i][1], width, height },
            .color = { color[0], color[1], color[2], color[3] },
        };
    }

    list_push(geometry->indices) = (GLushort)start_index + 0;
    list_push(geometry->indices) = (GLushort)start_index + 1;
    list_push(geometry->indices) = (GLushort)start_index + 2;
}

static inline void aven_gl_shape_rounded_geometry_push_triangle(
    AvenGlShapeRoundedGeometry *geometry,
    Aff2 trans,
    Vec2 p1,
    Vec2 p2,
    Vec2 p3,
    float roundness,
    Vec4 color
) {
    aff2_transform(p1, trans, p1);
    aff2_transform(p2, trans, p2);
    aff2_transform(p3, trans, p3);

    Vec2 mid;
    vec2_midpoint(mid, p2, p3);

    Vec2 up;
    vec2_sub(up, p1, mid);

    Vec2 base;
    vec2_sub(base, p2, p3);

    float width = vec2_mag(up);
    float height = vec2_mag(base);

    // Roundness calculation done for equilateral triangle
    float rs = 1.0f + roundness;
    float sx = AVEN_MATH_SQRT3_F / 2.0f;

    size_t start_index = geometry->vertices.len;

    list_push(geometry->vertices) = (AvenGlShapeRoundedVertex){
        .pos = { p1[0], p1[1] },
        .info = { 0.0f, rs * 1.0f, height, width },
        .color = { color[0], color[1], color[2], color[3] },
    };
    list_push(geometry->vertices) = (AvenGlShapeRoundedVertex){
        .pos = { p2[0], p2[1] },
        .info = { rs * -sx, rs * -0.5f, height, width },
        .color = { color[0], color[1], color[2], color[3] },
    };
    list_push(geometry->vertices) = (AvenGlShapeRoundedVertex){
        .pos = { p3[0], p3[1] },
        .info = { rs * sx, rs * -0.5f, height, width },
        .color = { color[0], color[1], color[2], color[3] },
    };

    list_push(geometry->indices) = (GLushort)start_index + 0;
    list_push(geometry->indices) = (GLushort)start_index + 1;
    list_push(geometry->indices) = (GLushort)start_index + 2;
}

static inline void aven_gl_shape_rounded_geometry_push_triangle_isoceles(
    AvenGlShapeRoundedGeometry *geometry,
    Aff2 trans,
    float roundness,
    Vec4 color
) {
    Vec2 p1 = {  0.0f,  1.0f };
    Vec2 p2 = {  1.0f, -1.0f };
    Vec2 p3 = { -1.0f, -1.0f };

    aven_gl_shape_rounded_geometry_push_triangle(
        geometry,
        trans,
        p1,
        p2,
        p3,
        roundness,
        color
    );
}

static inline void aven_gl_shape_rounded_geometry_push_triangle_right(
    AvenGlShapeRoundedGeometry *geometry,
    Aff2 trans,
    float roundness,
    Vec4 color
) {
    Vec2 p1 = { -1.0f,  1.0f };
    Vec2 p2 = {  1.0f, -1.0f };
    Vec2 p3 = { -1.0f, -1.0f };

    aven_gl_shape_rounded_geometry_push_triangle(
        geometry,
        trans,
        p1,
        p2,
        p3,
        roundness,
        color
    );
}

static inline void aven_gl_shape_rounded_geometry_push_square(
    AvenGlShapeRoundedGeometry *geometry,
    Aff2 trans,
    float roundness,
    Vec4 color
) {
    Vec2 p1 = { -1.0f, -1.0f };
    Vec2 p2 = {  1.0f, -1.0f };
    Vec2 p3 = {  1.0f,  1.0f };
    Vec2 p4 = { -1.0f,  1.0f };

    aff2_transform(p1, trans, p1);
    aff2_transform(p2, trans, p2);
    aff2_transform(p3, trans, p3);
    aff2_transform(p4, trans, p4);

    float rs = (1.0f / AVEN_MATH_SQRT2_F) +
        roundness * (1.0f - (1.0f / AVEN_MATH_SQRT2_F));

    Vec2 p1p2;
    vec2_sub(p1p2, p2, p1);

    Vec2 p1p4;
    vec2_sub(p1p4, p4, p1);

    float width = vec2_mag(p1p2);
    float height = vec2_mag(p1p4);

    size_t start_index = geometry->vertices.len;

    list_push(geometry->vertices) = (AvenGlShapeRoundedVertex){
        .pos = { p1[0], p1[1] },
        .info = { -1.0f * rs, -1.0f * rs, width, height },
        .color = { color[0], color[1], color[2], color[3] },
    };
    list_push(geometry->vertices) = (AvenGlShapeRoundedVertex){
        .pos = { p2[0], p2[1] },
        .info = { 1.0f * rs, -1.0f * rs, width, height },
        .color = { color[0], color[1], color[2], color[3] },
    };
    list_push(geometry->vertices) = (AvenGlShapeRoundedVertex){
        .pos = { p3[0], p3[1] },
        .info = { 1.0f * rs, 1.0f * rs, width, height },
        .color = { color[0], color[1], color[2], color[3] },
    };
    list_push(geometry->vertices) = (AvenGlShapeRoundedVertex){
        .pos = { p4[0], p4[1] },
        .info = { -1.0f * rs, 1.0f * rs, width, height },
        .color = { color[0], color[1], color[2], color[3] },
    };

    list_push(geometry->indices) = (GLushort)start_index + 0;
    list_push(geometry->indices) = (GLushort)start_index + 1;
    list_push(geometry->indices) = (GLushort)start_index + 2;
    list_push(geometry->indices) = (GLushort)start_index + 0;
    list_push(geometry->indices) = (GLushort)start_index + 2;
    list_push(geometry->indices) = (GLushort)start_index + 3;
}

static inline void aven_gl_shape_rounded_geometry_push_square_half(
    AvenGlShapeRoundedGeometry *geometry,
    Aff2 trans,
    float roundness,
    Vec4 color
) {
    Vec2 p1 = { -1.0f, -1.0f };
    Vec2 p2 = {  1.0f, -1.0f };
    Vec2 p3 = {  1.0f,  1.0f };
    Vec2 p4 = { -1.0f,  1.0f };

    aff2_transform(p1, trans, p1);
    aff2_transform(p2, trans, p2);
    aff2_transform(p3, trans, p3);
    aff2_transform(p4, trans, p4);

    Vec2 p1p2;
    vec2_sub(p1p2, p2, p1);

    Vec2 p1p4;
    vec2_sub(p1p4, p4, p1);

    float width = vec2_mag(p1p2);
    float height = 2.0f * vec2_mag(p1p4);

    float rs = (1.0f / AVEN_MATH_SQRT2_F) +
        roundness * (1.0f - (1.0f / AVEN_MATH_SQRT2_F));

    size_t start_index = geometry->vertices.len;

    list_push(geometry->vertices) = (AvenGlShapeRoundedVertex){
        .pos = { p1[0], p1[1] },
        .info = { -1.0f * rs, 0.0f, width, height },
        .color = { color[0], color[1], color[2], color[3] },
    };
    list_push(geometry->vertices) = (AvenGlShapeRoundedVertex){
        .pos = { p2[0], p2[1] },
        .info = { 1.0f * rs, 0.0f, width, height },
        .color = { color[0], color[1], color[2], color[3] },
    };
    list_push(geometry->vertices) = (AvenGlShapeRoundedVertex){
        .pos = { p3[0], p3[1] },
        .info = { 1.0f * rs, 1.0f * rs, width, height },
        .color = { color[0], color[1], color[2], color[3] },
    };
    list_push(geometry->vertices) = (AvenGlShapeRoundedVertex){
        .pos = { p4[0], p4[1] },
        .info = { -1.0f * rs, 1.0f * rs, width, height },
        .color = { color[0], color[1], color[2], color[3] },
    };

    list_push(geometry->indices) = (GLushort)start_index + 0;
    list_push(geometry->indices) = (GLushort)start_index + 1;
    list_push(geometry->indices) = (GLushort)start_index + 2;
    list_push(geometry->indices) = (GLushort)start_index + 0;
    list_push(geometry->indices) = (GLushort)start_index + 2;
    list_push(geometry->indices) = (GLushort)start_index + 3;
}

#endif // AVEN_GL_SHAPE_H
