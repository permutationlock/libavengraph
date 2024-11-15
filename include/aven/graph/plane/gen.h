#ifndef AVEN_GRAPH_PLANE_GEN_H
#define AVEN_GRAPH_PLANE_GEN_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/rng.h>

#include <float.h>

#include "../../graph.h"
#include "../plane.h"

typedef struct {
    AvenGraph graph;
    AvenGraphPlaneEmbedding embedding;
} AvenGraphPlaneGenData;

static inline AvenGraphPlaneGenData aven_graph_plane_gen_grid(
    uint32_t width,
    uint32_t height,
    AvenArena *arena
) {
    if (width == 0 or height == 0) {
        return (AvenGraphPlaneGenData){ 0 };
    }

    AvenGraph graph = { .len = width * height };
    graph.ptr = aven_arena_create_array(AvenGraphAdjList, arena, graph.len);

    for (uint32_t v = 0; v < graph.len; v += 1) {
        AvenGraphAdjList *adj = &slice_get(graph, v);
        adj->len = 4;
        adj->ptr = aven_arena_create_array(
            uint32_t,
            arena,
            adj->len
        );

        uint32_t x = v % width;
        uint32_t y = v / width;

        uint32_t i = 0;
        if (x > 0) {
            slice_get(*adj, i) = (x - 1) + y * width;
            i += 1;
        }
        if (y > 0) {
            slice_get(*adj, i) = x + (y - 1) * width;
            i += 1;
        }
        if (x < width - 1) {
            slice_get(*adj, i) = (x + 1) + y * width;
            i += 1;
        }
        if (y < height - 1) {
            slice_get(*adj, i) = x + (y + 1) * width;
            i += 1;
        }

        adj->len = i;
    }

    AvenGraphPlaneEmbedding embedding = { .len = graph.len };
    embedding.ptr = aven_arena_create_array(Vec2, arena, embedding.len);

    float x_scale = 2.0f;
    if (width > 1) {
        x_scale /= (float)(width - 1);
    }
    float y_scale = 2.0f;
    if (height > 1) {
        y_scale /= (float)(height - 1);
    }

    for (uint32_t v = 0; v < graph.len; v += 1) {
        uint32_t x = v % width;
        uint32_t y = v / width;

        slice_get(embedding, v)[0] = -1.0f + (float)x * x_scale;
        slice_get(embedding, v)[1] = -1.0f + (float)y * y_scale;
    }

    return (AvenGraphPlaneGenData){
        .graph = graph,
        .embedding = embedding,
    };
}

#define AVEN_GRAPH_PLANE_GEN_FACE_INVALID 0xffffffffUL

typedef struct {
    uint32_t vertices[3];
    uint32_t neighbors[3];
    float area;
    bool invalid;
} AvenGraphPlaneGenFace;

typedef Slice(AvenGraphPlaneGenFace) AvenGraphPlaneGenFaceSlice;

typedef struct {
    List(Vec2) embedding;
    List(AvenGraphPlaneGenFace) faces;
    List(uint32_t) valid_faces;
    uint32_t active_face;
    float min_coeff;
    float min_area;
    bool square;
} AvenGraphPlaneGenTriCtx;

static inline AvenGraphPlaneGenTriCtx aven_graph_plane_gen_tri_init(
    AvenGraphPlaneEmbedding embedding,
    Aff2 trans,
    float min_area,
    float min_coeff,
    bool square,
    AvenArena *arena
) {
    AvenGraphPlaneGenTriCtx ctx = {
        .embedding = { .ptr = embedding.ptr, .cap = embedding.len },
        .faces = { .cap = 2 * embedding.len - 4 },
        .valid_faces = { .cap = 2 * embedding.len - 4 },
        .active_face = AVEN_GRAPH_PLANE_GEN_FACE_INVALID,
        .min_area = 2.0f * min_area,
        .min_coeff = min_coeff,
        .square = square,
    };

    ctx.faces.ptr = aven_arena_create_array(
        AvenGraphPlaneGenFace,
        arena,
        ctx.faces.cap
    );

    ctx.valid_faces.ptr = aven_arena_create_array(
        uint32_t,
        arena,
        ctx.valid_faces.cap
    );

    if (!ctx.square) {
        Vec2 points[3] = {
            {  0.0f, 1.0f },
            {  1.0f, -1.0f },
            { -1.0f, -1.0f },
        };

        for (size_t i = 0; i < countof(points); i += 1) {
            aff2_transform(points[i], trans, points[i]);
            vec2_copy(list_push(ctx.embedding), points[i]);
        }

        float area = vec2_triangle_area(
            slice_get(ctx.embedding, 0),
            slice_get(ctx.embedding, 1),
            slice_get(ctx.embedding, 2)
        );

        list_push(ctx.faces) = (AvenGraphPlaneGenFace){
            .vertices = { 0, 2, 1 },
            .neighbors = { 1, 1, 1 },
            .area = -1.0f,
        };
        list_push(ctx.faces) = (AvenGraphPlaneGenFace){
            .vertices = { 0, 1, 2 },
            .neighbors = { 0, 0, 0 },
            .area = area,
        };

        list_push(ctx.valid_faces) = 1;
    } else {
        Vec2 points[4] = {
            { -1.0f,  1.0f },
            {  1.0f,  1.0f },
            {  1.0f, -1.0f },
            { -1.0f, -1.0f },
        };

        for (size_t i = 0; i < countof(points); i += 1) {
            aff2_transform(points[i], trans, points[i]);
            vec2_copy(list_push(ctx.embedding), points[i]);
        }

        float area = vec2_triangle_area(
            slice_get(ctx.embedding, 0),
            slice_get(ctx.embedding, 1),
            slice_get(ctx.embedding, 2)
        );

        list_push(ctx.faces) = (AvenGraphPlaneGenFace){
            .vertices = { 2, 1, 3 },
            .neighbors = { 2, 1, 3 },
            .area = -1.0f,
        };
        list_push(ctx.faces) = (AvenGraphPlaneGenFace){
            .vertices = { 1, 0, 3 },
            .neighbors = { 2, 3, 0 },
            .area = -1.0f,
        };
        list_push(ctx.faces) = (AvenGraphPlaneGenFace){
            .vertices = { 0, 1, 2 },
            .neighbors = { 1, 0, 3 },
            .area = area,
        };
        list_push(ctx.faces) = (AvenGraphPlaneGenFace){
            .vertices = { 0, 2, 3 },
            .neighbors = { 2, 0, 1 },
            .area = area,
        };

        list_push(ctx.valid_faces) = 2;
        list_push(ctx.valid_faces) = 3;
    }

    return ctx;
}

static float aven_graph_plane_gen_tri_face_area_internal(
    AvenGraphPlaneGenTriCtx *ctx,
    uint32_t v1,
    uint32_t v2,
    uint32_t v3
) {
    Vec2 coords[3];
    vec2_copy(coords[0], slice_get(ctx->embedding, v1));
    vec2_copy(coords[1], slice_get(ctx->embedding, v2));
    vec2_copy(coords[2], slice_get(ctx->embedding, v3));

    float max_side2 = 0.0f;
    for (uint32_t i = 0; i < 3; i += 1) {
        uint32_t j = (i + 1) % 3;

        Vec2 side;
        vec2_sub(side, coords[j], coords[i]);
        max_side2 = max(max_side2, vec2_dot(side, side));
    }

    float area = vec2_triangle_area(coords[0], coords[1], coords[2]);

    float bh_ratio = 2.0f * area / max_side2;

    return area * bh_ratio;
}

static inline bool aven_graph_plane_gen_tri_step(
    AvenGraphPlaneGenTriCtx *ctx,
    AvenRng rng
) {
    if (ctx->embedding.len == ctx->embedding.cap) {
        return true;
    }

    if (ctx->active_face == AVEN_GRAPH_PLANE_GEN_FACE_INVALID) {
        uint32_t tries = (uint32_t)ctx->valid_faces.len;
        while (tries != 0) {
            uint32_t valid_face_index = aven_rng_rand_bounded(
                rng,
                (uint32_t)ctx->valid_faces.len
            );
            ctx->active_face = list_get(ctx->valid_faces, valid_face_index);

            AvenGraphPlaneGenFace *face = &list_get(
                ctx->faces,
                ctx->active_face
            );

            if (face->area > 3.0f * ctx->min_area) {
                return false;
            }

            list_get(ctx->valid_faces, valid_face_index) = list_get(
                ctx->valid_faces,
                ctx->valid_faces.len - 1
            );

            ctx->valid_faces.len -= 1;
            face->invalid = true;

            tries -= 1;
        }

        ctx->active_face = AVEN_GRAPH_PLANE_GEN_FACE_INVALID;
        return true;
    }

    AvenGraphPlaneGenFace face = list_get(ctx->faces, ctx->active_face);

    // Generate a random vertex contained within the active face

    Vec2 face_coords[3];
    for (uint32_t i = 0; i < 3; i += 1) {
        vec2_copy(face_coords[i], slice_get(ctx->embedding, face.vertices[i]));
    }

    float min_coeff = max(
        0.33f * sqrtf(3.0f * ctx->min_area / face.area),
        ctx->min_coeff
    );
    float coeff[3] = {
        min_coeff,
        min_coeff,
        min_coeff,
    };

    float remaining = 1.0f;
    for (uint32_t i = 0; i < 3; i += 1) {
        remaining -= coeff[i];
    }

    for (uint32_t i = 0; i < 2; i += 1) {
        float extra = aven_rng_randf(rng) * remaining;
        remaining -= extra;
        coeff[i] += extra;
    }
    coeff[2] += remaining;

    Vec2 vpos = { 0.0f, 0.0f };
    for (uint32_t i = 0; i < 3; i += 1) {
        Vec2 scaled;
        vec2_scale(scaled, coeff[i], face_coords[i]);
        vec2_add(vpos, vpos, scaled);
    }

    uint32_t v = (uint32_t)ctx->embedding.len;
    vec2_copy(list_push(ctx->embedding), (Vec2){ vpos[0], vpos[1] });

    // Split the active face into three faces around the new vertex

    uint32_t new_face_indices[3] = {
        ctx->active_face,
        (uint32_t)ctx->faces.len,
        (uint32_t)ctx->faces.len + 1,
    };

    list_push(ctx->faces) = (AvenGraphPlaneGenFace){ 0 };
    list_push(ctx->faces) = (AvenGraphPlaneGenFace){ 0 };

    list_push(ctx->valid_faces) = new_face_indices[1];
    list_push(ctx->valid_faces) = new_face_indices[2];

    for (uint32_t i = 0; i < 3; i += 1) {
        uint32_t nexti = (i + 1) % 3;
        uint32_t previ = (i + 2) % 3;

        AvenGraphPlaneGenFace *new_face = &list_get(
            ctx->faces,
            new_face_indices[i]
        );

        new_face->vertices[0] = v;
        new_face->vertices[1] = face.vertices[i];
        new_face->vertices[2] = face.vertices[nexti];

        new_face->neighbors[0] = new_face_indices[previ];
        new_face->neighbors[1] = face.neighbors[i];
        new_face->neighbors[2] = new_face_indices[nexti];

        new_face->area = aven_graph_plane_gen_tri_face_area_internal(
            ctx,
            new_face->vertices[0],
            new_face->vertices[1],
            new_face->vertices[2]
        );
    }

    for (uint32_t i = 0; i < 3; i += 1) {
        AvenGraphPlaneGenFace *neighbor = &list_get(
            ctx->faces,
            face.neighbors[i]
        );
        uint32_t end_vertex = face.vertices[(i + 1) % 3];

        uint32_t j = 0;
        for (; j < 3; j += 1) {
            if (neighbor->vertices[j] == end_vertex) {
                break;
            }
        }
        assert(j < 3);

        neighbor->neighbors[j] = new_face_indices[i];
    }

    // Determine if any faces neighboring the active face are valid for edge
    // flips with v

    typedef struct {
        uint32_t neighbor_index;
        uint32_t vertex_index;
        float new_area;
        float new_neighbor_area;
    } AvenGraphPlaneGenNeighbor;
    AvenGraphPlaneGenNeighbor neighbor_data[3];
    List(AvenGraphPlaneGenNeighbor) valid_neighbors = list_array(neighbor_data);

    // uint32_t flips = aven_rng_rand(rng);

    for (uint32_t i = 0; i < 3; i += 1) {
        AvenGraphPlaneGenFace *neighbor = &list_get(
            ctx->faces,
            face.neighbors[i]
        );

        if (neighbor->area < 0.0f) {
            continue;
        }

        uint32_t j = (i + 1) % 3;

        uint32_t f1 = face.vertices[i];
        uint32_t f2 = face.vertices[j];

        Vec2 f1pos;
        vec2_copy(f1pos, list_get(ctx->embedding, f1));

        Vec2 f2pos;
        vec2_copy(f2pos, list_get(ctx->embedding, f2));

        uint32_t k = 0;
        for (; k < 3; k += 1) {
            if (neighbor->vertices[k] != f1 and neighbor->vertices[k] != f2) {
                break;
            }
        }
        assert(k < 3);

        uint32_t n = neighbor->vertices[k];

        float existing_area = slice_get(ctx->faces, new_face_indices[i]).area;
        float existing_neighbor_area = neighbor->area;

        float min_existing = min(existing_area, existing_neighbor_area);
        
        float new_area = aven_graph_plane_gen_tri_face_area_internal(
            ctx,
            v,
            f1,
            n
        );
        float new_neighbor_area = aven_graph_plane_gen_tri_face_area_internal(
            ctx,
            n,
            f2,
            v
        );

        float min_new = min(new_area, new_neighbor_area);

        if (min_existing >= min_new) {
            continue;
        }

        Vec2 npos;
        vec2_copy(npos, list_get(ctx->embedding, n));

        Vec2 vn;
        vec2_sub(vn, npos, vpos);

        Vec2 vf1;
        vec2_sub(vf1, f1pos, vpos);
        Vec2 perp1;
        vec2_copy(perp1, (Vec2){ vf1[1], -vf1[0] });

        Vec2 vf2;
        vec2_sub(vf2, f2pos, vpos);

        Vec2 perp2;
        vec2_copy(perp2, (Vec2){ -vf2[1], vf2[0] });

        float test1 = vec2_dot(vn, perp1);
        float test2 = vec2_dot(vn, perp2);

        if (test1 > 0.0f and test2 > 0.0f) {
            list_push(valid_neighbors) = (AvenGraphPlaneGenNeighbor){
                .neighbor_index = i,
                .vertex_index = k,
                .new_area = new_area,
                .new_neighbor_area = new_neighbor_area,
            };
        }
    }

    // Flip selected valid edges around the newly split active face

    for (uint32_t i = 0; i < valid_neighbors.len; i += 1) {
        AvenGraphPlaneGenNeighbor *neighbor = &list_get(valid_neighbors, i);

        uint32_t new_face_index = new_face_indices[neighbor->neighbor_index];
        AvenGraphPlaneGenFace *new_face = &list_get(ctx->faces, new_face_index);

        uint32_t neighbor_face_index = new_face->neighbors[1];
        AvenGraphPlaneGenFace *neighbor_face = &list_get(
            ctx->faces,
            neighbor_face_index
        );

        uint32_t ncur = neighbor->vertex_index;
        uint32_t nnext = (ncur + 1) % 3;
        uint32_t nprev = (ncur + 2) % 3;

        AvenGraphPlaneGenFace old_neighbor_face = *neighbor_face;

        neighbor_face->neighbors[nprev] = new_face_index;
        neighbor_face->neighbors[nnext] = new_face->neighbors[2];
        neighbor_face->vertices[nprev] = new_face->vertices[0];
        neighbor_face->area = neighbor->new_neighbor_area;

        new_face->neighbors[2] = new_face->neighbors[1];
        new_face->neighbors[1] = old_neighbor_face.neighbors[nprev];
        new_face->vertices[2] = old_neighbor_face.vertices[ncur];
        new_face->area = neighbor->new_area;

        AvenGraphPlaneGenFace *neighbor_prev_neighbor = &list_get(
            ctx->faces,
            new_face->neighbors[1]
        );

        uint32_t j = 0;
        for (; j < 3; j += 1) {
            if (neighbor_prev_neighbor->vertices[j] == new_face->vertices[2]) {
                break;
            }
        }
        assert(j < 3);

        neighbor_prev_neighbor->neighbors[j] = new_face_index;

        AvenGraphPlaneGenFace *new_face_next_neighbor = &list_get(
            ctx->faces,
            neighbor_face->neighbors[nnext]
        );

        for (j = 0; j < 3; j += 1) {
            if (new_face_next_neighbor->vertices[j] == new_face->vertices[0]) {
                break;
            }
        }
        assert(j < 3);

        new_face_next_neighbor->neighbors[j] = neighbor_face_index;

        // neighbor face changed shape so re-add it to the pool if necessary
        if (neighbor_face->invalid) {
            neighbor_face->invalid = false;
            list_push(ctx->valid_faces) = neighbor_face_index;
        }
    }

    ctx->active_face = AVEN_GRAPH_PLANE_GEN_FACE_INVALID;

    return false;
}

static inline bool aven_graph_plane_gen_tri_step_unrestricted(
    AvenGraphPlaneGenTriCtx *ctx,
    AvenRng rng
) {
    if (ctx->embedding.len == ctx->embedding.cap) {
        return true;
    }

    if (ctx->active_face == AVEN_GRAPH_PLANE_GEN_FACE_INVALID) {
        uint32_t valid_face_index = aven_rng_rand_bounded(
            rng,
            (uint32_t)ctx->valid_faces.len
        );
        ctx->active_face = list_get(ctx->valid_faces, valid_face_index);
        return false;
    }

    AvenGraphPlaneGenFace face = list_get(ctx->faces, ctx->active_face);

    // Generate a random vertex contained within the active face

    Vec2 face_coords[3];
    for (uint32_t i = 0; i < 3; i += 1) {
        vec2_copy(face_coords[i], slice_get(ctx->embedding, face.vertices[i]));
    }

    float coeff[3] = {
        FLT_MIN,
        FLT_MIN,
        FLT_MIN,
    };

    float remaining = 1.0f;

    for (uint32_t i = 0; i < 2; i += 1) {
        float extra = aven_rng_randf(rng) * remaining;
        remaining -= extra;
        coeff[i] += extra;
    }
    coeff[2] += remaining;

    Vec2 vpos = { 0.0f, 0.0f };
    for (uint32_t i = 0; i < 3; i += 1) {
        Vec2 scaled;
        vec2_scale(scaled, coeff[i], face_coords[i]);
        vec2_add(vpos, vpos, scaled);
    }

    uint32_t v = (uint32_t)ctx->embedding.len;
    vec2_copy(list_push(ctx->embedding), (Vec2){ vpos[0], vpos[1] });

    // Split the active face into three faces around the new vertex

    uint32_t new_face_indices[3] = {
        ctx->active_face,
        (uint32_t)ctx->faces.len,
        (uint32_t)ctx->faces.len + 1,
    };

    list_push(ctx->faces) = (AvenGraphPlaneGenFace){ 0 };
    list_push(ctx->faces) = (AvenGraphPlaneGenFace){ 0 };

    list_push(ctx->valid_faces) = new_face_indices[1];
    list_push(ctx->valid_faces) = new_face_indices[2];

    for (uint32_t i = 0; i < 3; i += 1) {
        uint32_t nexti = (i + 1) % 3;
        uint32_t previ = (i + 2) % 3;

        AvenGraphPlaneGenFace *new_face = &list_get(
            ctx->faces,
            new_face_indices[i]
        );

        new_face->vertices[0] = v;
        new_face->vertices[1] = face.vertices[i];
        new_face->vertices[2] = face.vertices[nexti];

        new_face->neighbors[0] = new_face_indices[previ];
        new_face->neighbors[1] = face.neighbors[i];
        new_face->neighbors[2] = new_face_indices[nexti];
    }

    for (uint32_t i = 0; i < 3; i += 1) {
        AvenGraphPlaneGenFace *neighbor = &list_get(
            ctx->faces,
            face.neighbors[i]
        );
        uint32_t end_vertex = face.vertices[(i + 1) % 3];

        uint32_t j = 0;
        for (; j < 3; j += 1) {
            if (neighbor->vertices[j] == end_vertex) {
                break;
            }
        }
        assert(j < 3);

        neighbor->neighbors[j] = new_face_indices[i];
    }

    // Determine if any faces neighboring the active face are valid for edge
    // flips with v

    typedef struct {
        uint32_t neighbor_index;
        uint32_t vertex_index;
    } AvenGraphPlaneGenNeighbor;
    AvenGraphPlaneGenNeighbor neighbor_data[3];
    List(AvenGraphPlaneGenNeighbor) valid_neighbors = list_array(neighbor_data);

    uint32_t flips = aven_rng_rand_bounded(rng, 3);
    uint32_t offset = aven_rng_rand_bounded(rng, 3);

    for (uint32_t n_index = 0; n_index < 3; n_index += 1) {
        uint32_t i = (n_index + offset) % 3;
        uint32_t j = (i + 1) % 3;

        AvenGraphPlaneGenFace *neighbor = &list_get(
            ctx->faces,
            face.neighbors[i]
        );

        uint32_t f1 = face.vertices[i];
        uint32_t f2 = face.vertices[j];

        Vec2 f1pos;
        vec2_copy(f1pos, list_get(ctx->embedding, f1));

        Vec2 f2pos;
        vec2_copy(f2pos, list_get(ctx->embedding, f2));

        uint32_t k = 0;
        for (; k < 3; k += 1) {
            if (neighbor->vertices[k] != f1 and neighbor->vertices[k] != f2) {
                break;
            }
        }
        assert(k < 3);

        uint32_t n = neighbor->vertices[k];

        Vec2 npos;
        vec2_copy(npos, list_get(ctx->embedding, n));

        Vec2 vn;
        vec2_sub(vn, npos, vpos);

        Vec2 vf1;
        vec2_sub(vf1, f1pos, vpos);
        Vec2 perp1;
        vec2_copy(perp1, (Vec2){ vf1[1], -vf1[0] });

        Vec2 vf2;
        vec2_sub(vf2, f2pos, vpos);

        Vec2 perp2;
        vec2_copy(perp2, (Vec2){ -vf2[1], vf2[0] });

        float test1 = vec2_dot(vn, perp1);
        float test2 = vec2_dot(vn, perp2);

        if (test1 > 0.0f and test2 > 0.0f and flips > 0) {
            list_push(valid_neighbors) = (AvenGraphPlaneGenNeighbor){
                .neighbor_index = i,
                .vertex_index = k,
            };
            flips -= 1;
        }
    }

    // Flip selected valid edges around the newly split active face

    for (uint32_t i = 0; i < valid_neighbors.len; i += 1) {
        AvenGraphPlaneGenNeighbor *neighbor = &list_get(valid_neighbors, i);

        uint32_t new_face_index = new_face_indices[neighbor->neighbor_index];
        AvenGraphPlaneGenFace *new_face = &list_get(ctx->faces, new_face_index);

        uint32_t neighbor_face_index = new_face->neighbors[1];
        AvenGraphPlaneGenFace *neighbor_face = &list_get(
            ctx->faces,
            neighbor_face_index
        );

        uint32_t ncur = neighbor->vertex_index;
        uint32_t nnext = (ncur + 1) % 3;
        uint32_t nprev = (ncur + 2) % 3;

        AvenGraphPlaneGenFace old_neighbor_face = *neighbor_face;

        neighbor_face->neighbors[nprev] = new_face_index;
        neighbor_face->neighbors[nnext] = new_face->neighbors[2];
        neighbor_face->vertices[nprev] = new_face->vertices[0];

        new_face->neighbors[2] = new_face->neighbors[1];
        new_face->neighbors[1] = old_neighbor_face.neighbors[nprev];
        new_face->vertices[2] = old_neighbor_face.vertices[ncur];

        AvenGraphPlaneGenFace *neighbor_prev_neighbor = &list_get(
            ctx->faces,
            new_face->neighbors[1]
        );

        uint32_t j = 0;
        for (; j < 3; j += 1) {
            if (neighbor_prev_neighbor->vertices[j] == new_face->vertices[2]) {
                break;
            }
        }
        assert(j < 3);

        neighbor_prev_neighbor->neighbors[j] = new_face_index;

        AvenGraphPlaneGenFace *new_face_next_neighbor = &list_get(
            ctx->faces,
            neighbor_face->neighbors[nnext]
        );

        for (j = 0; j < 3; j += 1) {
            if (new_face_next_neighbor->vertices[j] == new_face->vertices[0]) {
                break;
            }
        }
        assert(j < 3);

        new_face_next_neighbor->neighbors[j] = neighbor_face_index;
    }

    ctx->active_face = AVEN_GRAPH_PLANE_GEN_FACE_INVALID;

    return false;
}

typedef struct {
    List(AvenGraphAdjList) graph;
    List(uint32_t) master_adj;
} AvenGraphPlaneGenTriData;

static inline AvenGraphPlaneGenTriData aven_graph_plane_gen_tri_data_alloc(
    uint32_t size,
    AvenArena *arena
) {
    assert(size >= 3);

    AvenGraphPlaneGenTriData data = {
        .graph = { .cap = size },
        .master_adj = { .cap = 6 * size - 12 },
    };

    data.graph.ptr = aven_arena_create_array(
        AvenGraphAdjList,
        arena,
        data.graph.cap
    );
    data.master_adj.ptr = aven_arena_create_array(
        uint32_t,
        arena,
        data.master_adj.cap
    );

    return data;
}

static inline AvenGraphPlaneGenData aven_graph_plane_gen_tri_data(
    AvenGraphPlaneGenTriCtx *ctx,
    AvenGraphPlaneGenTriData *data
) {
    data->graph.len = 0;
    data->master_adj.len = 0;

    for (uint32_t i = 0; i < ctx->embedding.len; i += 1) {
        list_push(data->graph) = (AvenGraphAdjList){ 0 };
    }

    for (uint32_t i = 0; i < ctx->faces.len; i += 1) {
        AvenGraphPlaneGenFace *face = &list_get(ctx->faces, i);

        for (uint32_t j = 0; j < 3; j += 1) {
            uint32_t v = face->vertices[j];
            if (slice_get(data->graph, v).len != 0) {
                continue;
            }

            uint32_t adj_start = (uint32_t)data->master_adj.len;
            slice_get(data->graph, v).ptr = &data->master_adj.ptr[adj_start];

            {
                uint32_t u = face->vertices[(j + 1) % 3];
                if (!ctx->square) {
                    list_push(data->master_adj) = u;
                } else {
                    if ((u != 1 or v != 3) and (u != 3 or v != 1)) {
                        list_push(data->master_adj) = u;
                    }
                }
            }

            uint32_t face_index = face->neighbors[j];

            while (face_index != i) {
                AvenGraphPlaneGenFace *cur_face = &list_get(
                    ctx->faces,
                    face_index
                );

                uint32_t k = 0;
                for (; k < 3; k += 1) {
                    if (cur_face->vertices[k] == v) {
                        break;
                    }
                }
                assert(k < 3);

                uint32_t u = cur_face->vertices[(k + 1) % 3];
                if (!ctx->square) {
                    list_push(data->master_adj) = u;
                } else {
                    if ((u != 1 or v != 3) and (u != 3 or v != 1)) {
                        list_push(data->master_adj) = u;
                    }
                }
                face_index = cur_face->neighbors[k];
            }

            slice_get(data->graph, v).len = (uint32_t)(
                data->master_adj.len - adj_start
            );
        }
    }

    return (AvenGraphPlaneGenData){
        .graph = (AvenGraph){ .ptr = data->graph.ptr, .len = data->graph.len },
        .embedding = (AvenGraphPlaneEmbedding){
            .ptr = ctx->embedding.ptr,
            .len = ctx->embedding.len,
        },
    };
}

static inline AvenGraphPlaneGenData aven_graph_plane_gen_tri(
    uint32_t size,
    Aff2 trans,
    float min_area,
    float min_coeff,
    bool square,
    AvenRng rng,
    AvenArena *arena    
) {
    assert(size >= 3);

    AvenGraphPlaneGenTriData data = aven_graph_plane_gen_tri_data_alloc(
        size,
        arena
    );
    AvenGraphPlaneEmbedding embedding = { .len = size };
    embedding.ptr = aven_arena_create_array(
        Vec2,
        arena,
        embedding.len
    );

    AvenArena temp_arena = *arena;
    AvenGraphPlaneGenTriCtx ctx = aven_graph_plane_gen_tri_init(
        embedding,
        trans,
        min_area,
        min_coeff,
        square,
        &temp_arena
    );
    while (!aven_graph_plane_gen_tri_step(&ctx, rng)) {}

    return aven_graph_plane_gen_tri_data(&ctx, &data);
}

static inline AvenGraphPlaneGenData aven_graph_plane_gen_tri_unrestricted(
    uint32_t size,
    Aff2 trans,
    bool square,
    AvenRng rng,
    AvenArena *arena    
) {
    assert(size >= 3);

    AvenGraphPlaneGenTriData data = aven_graph_plane_gen_tri_data_alloc(
        size,
        arena
    );
    AvenGraphPlaneEmbedding embedding = { .len = size };
    embedding.ptr = aven_arena_create_array(
        Vec2,
        arena,
        embedding.len
    );

    AvenArena temp_arena = *arena;
    AvenGraphPlaneGenTriCtx ctx = aven_graph_plane_gen_tri_init(
        embedding,
        trans,
        0.0f,
        0.0f,
        square,
        &temp_arena
    );
    while (!aven_graph_plane_gen_tri_step_unrestricted(&ctx, rng)) {}

    return aven_graph_plane_gen_tri_data(&ctx, &data);
}

#endif // AVEN_GRAPH_PLANE_GEN_H
