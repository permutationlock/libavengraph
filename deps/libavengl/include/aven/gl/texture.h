#ifndef AVEN_GL_TEXTURE_H
#define AVEN_GL_TEXTURE_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/str.h>
#include <aven/math.h>

#include "../gl.h"

typedef struct {
    GLuint texture_id;
    GLuint vertex_shader;
    GLuint fragment_shader;
    GLuint program;
    GLuint utrans_location;
    GLuint upos_location;
    GLuint vpos_location;
} AvenGlTextureCtx;

typedef struct {
    Vec4 pos;
} AvenGlTextureVertex;

typedef struct {
    size_t vertex_cap;
    size_t index_cap;
    size_t index_len;
    GLuint vertex;
    GLuint index;
    AvenGlBufferUsage usage;
} AvenGlTextureBuffer;

typedef struct {
    List(AvenGlTextureVertex) vertices;
    List(GLushort) indices;
} AvenGlTextureGeometry;

typedef Optional(ByteSlice) AvenGlTextureBytesOptional;

static inline AvenGlTextureCtx aven_gl_texture_ctx_init(
    AvenGl *gl,
    size_t width,
    size_t height,
    AvenGlTextureBytesOptional maybe_bytes
) {
    if (maybe_bytes.valid) {
        assert((width * height) == (maybe_bytes.value.len / 4));
    }

    AvenGlTextureCtx ctx = { 0 };

    static const char* vertex_shader_text =
        "#version 100\n"
        "precision mediump float;\n"
        "uniform mat2 uTrans;\n"
        "uniform vec2 uPos;\n"
        "attribute vec4 vPos;\n"
        "varying vec2 tCoord;\n"
        "void main() {\n"
        "    gl_Position = vec4((uTrans * vPos.xy) + uPos, 0.0, 1.0);\n"
        "    tCoord = vPos.zw;\n"
        "}\n";

    static const char* fragment_shader_text =
        "#version 100\n"
        "precision mediump float;\n"
        "uniform sampler2D texSampler;\n"
        "varying vec2 tCoord;\n"
        "void main() {\n"
        "    gl_FragColor = texture2D(texSampler, tCoord);\n"
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
    assert(gl->GetError() == 0);

    gl->GenTextures(1, &ctx.texture_id);
    assert(gl->GetError() == 0);
    gl->BindTexture(GL_TEXTURE_2D, ctx.texture_id);
    assert(gl->GetError() == 0);
    gl->TexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA, 
        (GLsizei)width,
        (GLsizei)height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        maybe_bytes.valid ? maybe_bytes.value.ptr : NULL
    );
    assert(gl->GetError() == 0);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    assert(gl->GetError() == 0);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    assert(gl->GetError() == 0);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    assert(gl->GetError() == 0);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    assert(gl->GetError() == 0);

    gl->BindTexture(GL_TEXTURE_2D, 0);
    assert(gl->GetError() == 0);

    return ctx;
}

static inline void aven_gl_texture_ctx_update(
    AvenGl *gl,
    AvenGlTextureCtx *ctx,
    size_t width,
    size_t height,
    AvenGlTextureBytesOptional maybe_bytes
) {
    gl->BindTexture(GL_TEXTURE_2D, ctx->texture_id);
    assert(gl->GetError() == 0);
    gl->TexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA, 
        (GLsizei)width,
        (GLsizei)height,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        maybe_bytes.valid ? maybe_bytes.value.ptr : NULL
    );
    assert(gl->GetError() == 0);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    assert(gl->GetError() == 0);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    assert(gl->GetError() == 0);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    assert(gl->GetError() == 0);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    assert(gl->GetError() == 0);

    gl->BindTexture(GL_TEXTURE_2D, 0);
    assert(gl->GetError() == 0);
}

static inline void aven_gl_texture_ctx_update_framebuffer(
    AvenGl *gl,
    AvenGlTextureCtx *ctx,
    size_t x,
    size_t y,
    size_t width,
    size_t height    
) {
    gl->BindTexture(GL_TEXTURE_2D, ctx->texture_id);
    assert(gl->GetError() == 0);
    gl->CopyTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        (GLsizei)x,
        (GLsizei)y,
        (GLsizei)width,
        (GLsizei)height,
        0
    );
    assert(gl->GetError() == 0);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    assert(gl->GetError() == 0);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    assert(gl->GetError() == 0);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    assert(gl->GetError() == 0);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    assert(gl->GetError() == 0);

    gl->BindTexture(GL_TEXTURE_2D, 0);
    assert(gl->GetError() == 0);
}

static inline void aven_gl_texture_ctx_deinit(
    AvenGl *gl,
    AvenGlTextureCtx *ctx
) {
    gl->DeleteProgram(ctx->program);
    gl->DeleteShader(ctx->fragment_shader);
    gl->DeleteShader(ctx->vertex_shader);
}

static inline AvenGlTextureGeometry aven_gl_texture_geometry_init(
    size_t max_quads,
    AvenArena *arena
) {
    AvenGlTextureGeometry geometry = {
        .vertices = { .cap = max_quads * 4 },
        .indices = { .cap = max_quads * 6 },
    };

    geometry.vertices.ptr = aven_arena_create_array(
        AvenGlTextureVertex,
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

static inline void aven_gl_texture_geometry_push_square(
    AvenGlTextureGeometry *geometry,
    Aff2 trans,
    Aff2 texture_trans
) {
    Vec2 p1 = { -1.0f, -1.0f };
    Vec2 p2 = {  1.0f, -1.0f };
    Vec2 p3 = {  1.0f,  1.0f };
    Vec2 p4 = { -1.0f,  1.0f };

    aff2_transform(p1, trans, p1);
    aff2_transform(p2, trans, p2);
    aff2_transform(p3, trans, p3);
    aff2_transform(p4, trans, p4);

    Vec2 t1 = { 0.0f, 0.0f };
    Vec2 t2 = {  1.0f, 0.0f };
    Vec2 t3 = {  1.0f,  1.0f };
    Vec2 t4 = { 0.0f,  1.0f };

    aff2_transform(t1, texture_trans, t1);
    aff2_transform(t2, texture_trans, t2);
    aff2_transform(t3, texture_trans, t3);
    aff2_transform(t4, texture_trans, t4);

    size_t start_index = geometry->vertices.len;

    list_push(geometry->vertices) = (AvenGlTextureVertex){
        .pos = { p1[0], p1[1], t1[0], t1[1] },
    };
    list_push(geometry->vertices) = (AvenGlTextureVertex){
        .pos = { p2[0], p2[1], t2[0], t2[1] },
    };
    list_push(geometry->vertices) = (AvenGlTextureVertex){
        .pos = { p3[0], p3[1], t3[0], t3[1] },
    };
    list_push(geometry->vertices) = (AvenGlTextureVertex){
        .pos = { p4[0], p4[1], t4[0], t4[1] },
    };

    list_push(geometry->indices) = (GLushort)start_index + 0;
    list_push(geometry->indices) = (GLushort)start_index + 1;
    list_push(geometry->indices) = (GLushort)start_index + 2;
    list_push(geometry->indices) = (GLushort)start_index + 0;
    list_push(geometry->indices) = (GLushort)start_index + 2;
    list_push(geometry->indices) = (GLushort)start_index + 3;
}

static inline void aven_gl_texture_geometry_clear(
    AvenGlTextureGeometry *geometry
) {
    geometry->vertices.len = 0;
    geometry->indices.len = 0;
}

static inline AvenGlTextureBuffer aven_gl_texture_buffer_init(
    AvenGl *gl,
    AvenGlTextureGeometry *geometry,
    AvenGlBufferUsage buffer_usage
) {
    AvenGlTextureBuffer buffer = { .usage = buffer_usage };

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
            buffer.index_len = geometry->indices.len;
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

static inline void aven_gl_texture_buffer_deinit(
    AvenGl *gl,
    AvenGlTextureBuffer *buffer
) {
    gl->DeleteBuffers(1, &buffer->index);
    gl->DeleteBuffers(1, &buffer->vertex);
    *buffer = (AvenGlTextureBuffer){ 0 };
}

static inline void aven_gl_texture_buffer_update(
    AvenGl *gl,
    AvenGlTextureBuffer *buffer,
    AvenGlTextureGeometry *geometry
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

static inline void aven_gl_texture_geometry_draw(
    AvenGl *gl,
    AvenGlTextureCtx *ctx,
    AvenGlTextureBuffer *buffer,
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
    gl->VertexAttribPointer(
        ctx->vpos_location,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(AvenGlTextureVertex),
        (void*)offsetof(AvenGlTextureVertex, pos)
    );
    assert(gl->GetError() == 0);
    gl->Enable(GL_BLEND);
    assert(gl->GetError() == 0);
    gl->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    assert(gl->GetError() == 0);
    gl->BindTexture(GL_TEXTURE_2D, ctx->texture_id);
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

    gl->BindTexture(GL_TEXTURE_2D, 0);
    assert(gl->GetError() == 0);

    gl->Disable(GL_BLEND);
    assert(gl->GetError() == 0);
    gl->DisableVertexAttribArray(ctx->vpos_location);
    assert(gl->GetError() == 0);

    gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    assert(gl->GetError() == 0);
    gl->BindBuffer(GL_ARRAY_BUFFER, 0);
    assert(gl->GetError() == 0);
}

#endif // AVEN_GL_TEXTURE_H
