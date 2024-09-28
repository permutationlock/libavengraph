#ifndef AVEN_GL_TEXT_H
#define AVEN_GL_TEXT_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/str.h>

#include "../gl.h"
#include "../glm.h"

typedef struct {
   unsigned short x0;
   unsigned short y0;
   unsigned short x1;
   unsigned short y1;
   float xoff;
   float yoff;
   float xadvance;
   float xoff2;
   float yoff2;
} AvenGlTextChar;

typedef struct {
    AvenGlTextChar packed_chars[96];
    float height;
    GLuint texture_id;
    int texture_width;
    int texture_height;
} AvenGlTextFont;

static inline AvenGlTextFont aven_gl_text_font_init(
    AvenGl *gl,
    ByteSlice font_bytes,
    float font_height,
    AvenArena arena
) {
    struct stbtt_pack_context {
        void *user_allocator_context;
        void *pack_info;
        int width;
        int height;
        int stride_in_bytes;
        int padding;
        int skip_missing;
        unsigned int h_oversample, v_oversample;
        unsigned char *pixels;
        void *nodes;
    };
    int stbtt_PackBegin(
        struct stbtt_pack_context *spc,
        unsigned char *pixels,
        int width,
        int height,
        int stride_in_bytes,
        int padding,
        void *alloc_context
    );
    void stbtt_PackEnd(struct stbtt_pack_context *spc);
    void stbtt_PackSetOversampling(
    struct stbtt_pack_context *spc,
        unsigned int h_oversample,
        unsigned int v_oversample
    );
    int stbtt_PackFontRange(
        struct stbtt_pack_context *spc,
        const unsigned char *fontdata,
        int font_index,
        float font_size,
        int first_unicode_char_in_range,
        int num_chars_in_range,
        AvenGlTextChar *chardata_for_range
    );

    AvenArena temp_arena;
    int ncols = 4;
    int nrows = 8;
    int char_dim = (int)((font_height + 2.0f) * 2.0f);

    int success = 0;
    AvenGlTextFont font = { .height = font_height };
    ByteSlice texture_bytes = { 0 };
    do {
        temp_arena = arena;

        font.texture_width = char_dim * ncols;
        font.texture_height = char_dim * nrows;
        texture_bytes.len = (size_t)font.texture_width *
            (size_t)font.texture_height;
        texture_bytes.ptr = aven_arena_alloc(
            &temp_arena,
            1,
            1,
            texture_bytes.len
        );

        struct stbtt_pack_context ctx = { 0 };
        success = stbtt_PackBegin(
            &ctx,
            texture_bytes.ptr,
            font.texture_width,
            font.texture_height,
            0,
            1,
            &temp_arena
        );
        assert(success != 0);

        stbtt_PackSetOversampling(&ctx, 2, 2);

        success = stbtt_PackFontRange(
            &ctx,
            font_bytes.ptr,
            0,
            font_height,
            32,
            countof(font.packed_chars),
            font.packed_chars
        );

        stbtt_PackEnd(&ctx);

        nrows += 1;
    } while (success == 0);

    arena = temp_arena;

    assert(gl->GetError() == 0);
    gl->GenTextures(1, &font.texture_id);
    assert(gl->GetError() == 0);
    gl->BindTexture(GL_TEXTURE_2D, font.texture_id);
    assert(gl->GetError() == 0);
    gl->TexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_ALPHA, 
        (GLsizei)font.texture_width,
        (GLsizei)font.texture_height,
        0,
        GL_ALPHA,
        GL_UNSIGNED_BYTE,
        texture_bytes.ptr
    );
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    assert(gl->GetError() == 0);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    assert(gl->GetError() == 0);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    assert(gl->GetError() == 0);
    gl->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    assert(gl->GetError() == 0);

    gl->BindTexture(GL_TEXTURE_2D, 0);
    assert(gl->GetError() == 0);
    
    return font;
}

static inline void aven_gl_text_font_deinit(AvenGl *gl, AvenGlTextFont *font) {
    gl->DeleteTextures(1, &font->texture_id);
}

typedef struct {
    Vec2 pos;
    Vec2 tex;
    Vec4 color;
} AvenGlTextVertex;

typedef struct {
    GLuint vertex_shader;
    GLuint fragment_shader;
    GLuint program;
    GLuint utrans_location;
    GLuint upos_location;
    GLuint vpos_location;
    GLuint vtex_location;
    GLuint vcolor_location;
} AvenGlTextCtx;

typedef struct {
    size_t vertex_cap;
    size_t index_cap;
    size_t index_len;
    GLuint vertex;
    GLuint index;
    AvenGlBufferUsage usage;
} AvenGlTextBuffer;

typedef struct {
    List(AvenGlTextVertex) vertices;
    List(GLushort) indices;
} AvenGlTextGeometry;

static inline AvenGlTextGeometry aven_gl_text_geometry_init(
    size_t max_chars,
    AvenArena *arena
) {
    AvenGlTextGeometry geometry = {
        .vertices = { .cap = max_chars * 4 },
        .indices = { .cap = max_chars * 6 },
    };

    geometry.vertices.ptr = aven_arena_create_array(
        AvenGlTextVertex,
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

static inline void aven_gl_text_geometry_clear(AvenGlTextGeometry *geometry) {
    geometry->vertices.len = 0;
    geometry->indices.len = 0;
}

static inline AvenGlTextCtx aven_gl_text_ctx_init(AvenGl *gl) {
    AvenGlTextCtx ctx = { 0 };

    static const char* vertex_shader_text =
        "#version 100\n"
        "precision mediump float;\n"
        "uniform mat2 uTrans;\n"
        "uniform vec2 uPos;\n"
        "attribute vec2 vTex;\n"
        "attribute vec2 vPos;\n"
        "attribute vec4 vColor;\n"
        "varying vec4 fColor;\n"
        "varying vec2 tCoord;\n"
        "void main() {\n"
        "    gl_Position = vec4(uTrans * (vPos.xy - uPos), 0.0, 1.0);\n"
        "    tCoord = vTex;\n"
        "    fColor = vColor;\n"
        "}\n";

    static const char* fragment_shader_text =
        "#version 100\n"
        "precision mediump float;\n"
        "uniform sampler2D texSampler;\n"
        "varying vec4 fColor;\n"
        "varying vec2 tCoord;\n"
        "void main() {\n"
        "    vec4 tColor = texture2D(texSampler, tCoord);\n"
        "    float alpha = tColor.w * fColor.w;\n"
        "    gl_FragColor = vec4(fColor.xyz, alpha);\n"
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
    ctx.vtex_location = (GLuint)gl->GetAttribLocation(
        ctx.program,
        "vTex"
    );
    assert(gl->GetError() == 0);
    ctx.vcolor_location = (GLuint)gl->GetAttribLocation(
        ctx.program,
        "vColor"
    );
    assert(gl->GetError() == 0);

    return ctx;
}

static inline void aven_gl_text_ctx_deinit(AvenGl *gl, AvenGlTextCtx *ctx) {
    gl->DeleteProgram(ctx->program);
    gl->DeleteShader(ctx->fragment_shader);
    gl->DeleteShader(ctx->vertex_shader);
}

static inline AvenGlTextBuffer aven_gl_text_buffer_init(
    AvenGl *gl,
    AvenGlTextGeometry *geometry,
    AvenGlBufferUsage buffer_usage
) {
    AvenGlTextBuffer buffer = { .usage = buffer_usage };

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

static inline void aven_gl_text_buffer_deinit(
    AvenGl *gl,
    AvenGlTextBuffer *buffer
) {
    gl->DeleteBuffers(1, &buffer->index);
    gl->DeleteBuffers(1, &buffer->vertex);
    *buffer = (AvenGlTextBuffer){ 0 };
}

typedef struct {
    float x0;
    float y0;
    float s0;
    float t0;
    float x1;
    float y1;
    float s1;
    float t1;
} AvenGlTextQuad;

static inline float aven_gl_text_geometry_push(
    AvenGlTextGeometry *geometry,
    AvenGlTextFont *font,
    Vec2 pos,
    float pixel_size,
    Vec4 color,
    AvenStr text
) {
    void stbtt_GetPackedQuad(
        const AvenGlTextChar *chardata,
        int pw,
        int ph,
        int char_index,
        float *xpos,
        float *ypos,
        AvenGlTextQuad *q,
        int align_to_integer
    );

    float cur_x = 0.0f;
    float cur_y = 0.0f;

    for (size_t i = 0; i < text.len; i += 1) {
        int pc_index = slice_get(text, i) - 32;

        AvenGlTextQuad q;
        stbtt_GetPackedQuad(
            font->packed_chars,
            font->texture_width,
            font->texture_height,
            pc_index,
            &cur_x,
            &cur_y,
            &q,
            0
        );

        size_t start_index = geometry->vertices.len;

        list_push(geometry->vertices) = (AvenGlTextVertex){
            .pos = { q.x0 * pixel_size + pos[0], q.y0 * pixel_size + pos[1] },
            .tex = { q.s0, q.t0, },
            .color = { color[0], color[1], color[2], color[3] },
        };
        list_push(geometry->vertices) = (AvenGlTextVertex){
            .pos = { q.x1 * pixel_size + pos[0], q.y0 * pixel_size + pos[1] },
            .tex = { q.s1, q.t0, },
            .color = { color[0], color[1], color[2], color[3] },
        };
        list_push(geometry->vertices) = (AvenGlTextVertex){
            .pos = { q.x1 * pixel_size + pos[0], q.y1 * pixel_size + pos[1] },
            .tex = { q.s1, q.t1, },
            .color = { color[0], color[1], color[2], color[3] },
        };
        list_push(geometry->vertices) = (AvenGlTextVertex){
            .pos = { q.x0 * pixel_size + pos[0], q.y1 * pixel_size + pos[1] },
            .tex = { q.s0, q.t1, },
            .color = { color[0], color[1], color[2], color[3] },
        };

        list_push(geometry->indices) = (GLushort)start_index + 0;
        list_push(geometry->indices) = (GLushort)start_index + 1;
        list_push(geometry->indices) = (GLushort)start_index + 2;
        list_push(geometry->indices) = (GLushort)start_index + 0;
        list_push(geometry->indices) = (GLushort)start_index + 2;
        list_push(geometry->indices) = (GLushort)start_index + 3;
    }

    return cur_x;
}

static inline float aven_gl_text_geometry_width(
    AvenGlTextFont *font,
    float pixel_size,
    AvenStr text
) {
    void stbtt_GetPackedQuad(
        const AvenGlTextChar *chardata,
        int pw,
        int ph,
        int char_index,
        float *xpos,
        float *ypos,
        AvenGlTextQuad *q,
        int align_to_integer
    );

    float cur_x = 0.0f;
    float cur_y = 0.0f;

    for (size_t i = 0; i < text.len; i += 1) {
        int pc_index = slice_get(text, i) - 32;

        AvenGlTextQuad q;
        stbtt_GetPackedQuad(
            font->packed_chars,
            font->texture_width,
            font->texture_height,
            pc_index,
            &cur_x,
            &cur_y,
            &q,
            0
        );
    }

    return cur_x * pixel_size;
}

static inline void aven_gl_text_buffer_update(
    AvenGl *gl,
    AvenGlTextBuffer *buffer,
    AvenGlTextGeometry *geometry
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

static inline void aven_gl_text_geometry_draw(
    AvenGl *gl,
    AvenGlTextCtx *ctx,
    AvenGlTextBuffer *buffer,
    AvenGlTextFont *font,
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
    gl->EnableVertexAttribArray(ctx->vtex_location);
    assert(gl->GetError() == 0);
    gl->EnableVertexAttribArray(ctx->vcolor_location);
    assert(gl->GetError() == 0);
    gl->VertexAttribPointer(
        ctx->vpos_location,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(AvenGlTextVertex),
        (void*)offsetof(AvenGlTextVertex, pos)
    );
    assert(gl->GetError() == 0);
    gl->VertexAttribPointer(
        ctx->vtex_location,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(AvenGlTextVertex),
        (void*)offsetof(AvenGlTextVertex, tex)
    );
    assert(gl->GetError() == 0);
    gl->VertexAttribPointer(
        ctx->vcolor_location,
        4,
        GL_FLOAT,
        GL_FALSE,
        sizeof(AvenGlTextVertex),
        (void*)offsetof(AvenGlTextVertex, color)
    );
    assert(gl->GetError() == 0);
    gl->Enable(GL_BLEND);
    assert(gl->GetError() == 0);
    gl->BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    assert(gl->GetError() == 0);
    gl->BindTexture(GL_TEXTURE_2D, font->texture_id);
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

    gl->BindTexture(GL_TEXTURE_2D, 0);
    assert(gl->GetError() == 0);

    gl->Disable(GL_BLEND);
    assert(gl->GetError() == 0);
    gl->DisableVertexAttribArray(ctx->vcolor_location);
    assert(gl->GetError() == 0);
    gl->DisableVertexAttribArray(ctx->vtex_location);
    assert(gl->GetError() == 0);
    gl->DisableVertexAttribArray(ctx->vpos_location);
    assert(gl->GetError() == 0);

    gl->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    assert(gl->GetError() == 0);
    gl->BindBuffer(GL_ARRAY_BUFFER, 0);
    assert(gl->GetError() == 0);
}

#endif // AVEN_GL_TEXT_H
