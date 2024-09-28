#ifndef AVEN_GL_SHAPE_H
#define AVEN_GL_SHAPE_H

#include <aven.h>
#include <aven/arena.h>

#include "../gl.h"
#include "../glm.h"

typedef struct {
    Vec4 color;
    Vec4 pos;
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
        "attribute vec4 vPos;\n"
        "attribute vec4 vColor;\n"
        "varying vec2 fPos;\n"
        "varying vec4 fColor;\n"
        "void main() {\n"
        "    gl_Position = vec4(uTrans * (vPos.xy - uPos), 0.0, 1.0);\n"
        "    fPos = vPos.zw;\n"
        "    fColor = vColor;\n"
        "}\n";

    static const char* fragment_shader_text =
        "#version 100\n"
        "precision mediump float;\n"
        "varying vec2 fPos;\n"
        "varying vec4 fColor;\n"
        "float oversample(vec2 p) {\n"
        "    if (dot(p, p) > 1.0) {\n"
        "        return 0.0;\n"
        "    }\n"
        "    return 1.0;\n"
        "}\n"
        "void main() {\n"
        "    float offset = 0.004;\n"
        "    float magnitude = 0.0;\n"
        "    magnitude += oversample(fPos);\n"
        "    magnitude += oversample(fPos + vec2(-offset, 0.0));\n"
        "    magnitude += oversample(fPos + vec2(offset, 0.0));\n"
        "    magnitude += oversample(fPos + vec2(0.0, -offset));\n"
        "    magnitude += oversample(fPos + vec2(0.0, offset));\n"
        "    magnitude /= 5.0;\n"
        "    gl_FragColor = fColor * magnitude;\n"
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
    Mat2 cam_trans,
    Vec2 cam_pos
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
        4,
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
        (GLfloat*)cam_pos
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

static void aven_gl_shape_geometry_push_triangle(
    AvenGlShapeGeometry *geometry,
    Vec4 p1,
    Vec4 p2,
    Vec4 p3,
    Vec4 color
) {
    size_t start_index = geometry->vertices.len;

    list_push(geometry->vertices) = (AvenGlShapeVertex){
        .pos = { p1[0], p1[1], p1[2], p1[3] },
        .color = { color[0], color[1], color[2], color[3] },
    };
    list_push(geometry->vertices) = (AvenGlShapeVertex){
        .pos = { p2[0], p2[1], p2[2], p2[3] },
        .color = { color[0], color[1], color[2], color[3] },
    };
    list_push(geometry->vertices) = (AvenGlShapeVertex){
        .pos = { p3[0], p3[1], p3[2], p3[3] },
        .color = { color[0], color[1], color[2], color[3] },
    };

    list_push(geometry->indices) = (GLushort)start_index + 0;
    list_push(geometry->indices) = (GLushort)start_index + 1;
    list_push(geometry->indices) = (GLushort)start_index + 2;
}

static void aven_gl_shape_geometry_push_quad(
    AvenGlShapeGeometry *geometry,
    Vec4 p1,
    Vec4 p2,
    Vec4 p3,
    Vec4 p4,
    Vec4 color
) {
    size_t start_index = geometry->vertices.len;

    list_push(geometry->vertices) = (AvenGlShapeVertex){
        .pos = { p1[0], p1[1], p1[2], p1[3] },
        .color = { color[0], color[1], color[2], color[3] },
    };
    list_push(geometry->vertices) = (AvenGlShapeVertex){
        .pos = { p2[0], p2[1], p2[2], p2[3] },
        .color = { color[0], color[1], color[2], color[3] },
    };
    list_push(geometry->vertices) = (AvenGlShapeVertex){
        .pos = { p3[0], p3[1], p3[2], p3[3] },
        .color = { color[0], color[1], color[2], color[3] },
    };
    list_push(geometry->vertices) = (AvenGlShapeVertex){
        .pos = { p4[0], p4[1], p4[2], p4[3] },
        .color = { color[0], color[1], color[2], color[3] },
    };

    list_push(geometry->indices) = (GLushort)start_index + 0;
    list_push(geometry->indices) = (GLushort)start_index + 1;
    list_push(geometry->indices) = (GLushort)start_index + 2;
    list_push(geometry->indices) = (GLushort)start_index + 0;
    list_push(geometry->indices) = (GLushort)start_index + 2;
    list_push(geometry->indices) = (GLushort)start_index + 3;
}

static inline void aven_gl_shape_geometry_push_triangle_isoceles(
    AvenGlShapeGeometry *geometry,
    Vec2 pos,
    Vec2 dim,
    float rotation_angle,
    float roundness,
    Vec4 color
) {
    Vec2 p1 = {           0.0f, 2.0f * dim[1] / 3.0f };
    Vec2 p2 = { -dim[0] / 2.0f,       -dim[1] / 3.0f };
    Vec2 p3 = {  dim[0] / 2.0f,       -dim[1] / 3.0f };

    Mat2 id;
    mat2_identity(id);
    Mat2 rot;
    mat2_rotate(rot, id, rotation_angle);

    mat2_mul_vec2(p1, rot, p1);
    mat2_mul_vec2(p2, rot, p2);
    mat2_mul_vec2(p3, rot, p3);

    vec2_add(p1, pos, p1);
    vec2_add(p2, pos, p2);
    vec2_add(p3, pos, p3);

    float rs = 1.0f + roundness;
    float sx = AVEN_GLM_SQRT3_F / 2.0f;

    aven_gl_shape_geometry_push_triangle(
        geometry,
        (Vec4){ p1[0], p1[1],     0.0f,  rs * 1.0f },
        (Vec4){ p2[0], p2[1], rs * -sx, rs * -0.5f },
        (Vec4){ p3[0], p3[1],  rs * sx, rs * -0.5f },
        color
    );
}

static inline void aven_gl_shape_geometry_push_triangle_equilateral(
    AvenGlShapeGeometry *geometry,
    Vec2 pos,
    float height,
    float rotation_angle,
    float roundness,
    Vec4 color
) {
    aven_gl_shape_geometry_push_triangle_isoceles(
        geometry,
        pos,
        (Vec2){ (2.0f / AVEN_GLM_SQRT3_F) * height, height },
        rotation_angle,
        roundness,
        color
    );
}

static inline void aven_gl_shape_geometry_push_rectangle(
    AvenGlShapeGeometry *geometry,
    Vec2 pos,
    Vec2 dim,
    float rotation_angle,
    float roundness,
    Vec4 color
) {
    Vec2 p1 = { -dim[0] / 2.0f, -dim[1] / 2.0f };
    Vec2 p2 = {  dim[0] / 2.0f, -dim[1] / 2.0f };
    Vec2 p3 = {  dim[0] / 2.0f,  dim[1] / 2.0f };
    Vec2 p4 = { -dim[0] / 2.0f,  dim[1] / 2.0f };

    Mat2 id;
    mat2_identity(id);
    Mat2 rot;
    mat2_rotate(rot, id, rotation_angle);

    mat2_mul_vec2(p1, rot, p1);
    mat2_mul_vec2(p2, rot, p2);
    mat2_mul_vec2(p3, rot, p3);
    mat2_mul_vec2(p4, rot, p4);

    vec2_add(p1, pos, p1);
    vec2_add(p2, pos, p2);
    vec2_add(p3, pos, p3);
    vec2_add(p4, pos, p4);

    float rs = (1.0f / AVEN_GLM_SQRT2_F) +
        roundness * (1.0f - (1.0f / AVEN_GLM_SQRT2_F));

    aven_gl_shape_geometry_push_quad(
        geometry,
        (Vec4){ p1[0], p1[1], -1.0f * rs, -1.0f * rs },
        (Vec4){ p2[0], p2[1],  1.0f * rs, -1.0f * rs },
        (Vec4){ p3[0], p3[1],  1.0f * rs,  1.0f * rs },
        (Vec4){ p4[0], p4[1], -1.0f * rs,  1.0f * rs },
        color
    );
}

static inline void aven_gl_shape_geometry_push_line(
    AvenGlShapeGeometry *geometry,
    Vec2 p1,
    Vec2 p2,
    float thickness,
    float roundness,
    Vec4 color
) {
    // Below is not efficient, but it is simple given that rectangle is
    // already implemented. If drawing lots of lines each frame, consider
    // doing something different.

    Vec2 center;
    vec2_add(center, p2, p1);
    vec2_scale(center, 0.5f);

    Vec2 diff;
    vec2_diff(diff, p2, p1);
    float dist = sqrtf(vec2_dot(diff, diff));
    vec2_scale(diff, 1.0f / dist, diff);

    Vec2 dim = { (dist / 2.0f) + thickness, thickness };
    float angle = acosf(vec2_dot(diff, (Vec2){ 1.0f, 0.0f }));
    if (diff[1] < 0.0f) {
        angle = 2.0f * AVEN_GLM_PI_F - angle;
    }

    aven_gl_shape_geometry_push_rectangle(
        geometry,
        center,
        dim,
        angle,
        roundness,
        color
    );
}

#endif // AVEN_GL_SHAPE_H
