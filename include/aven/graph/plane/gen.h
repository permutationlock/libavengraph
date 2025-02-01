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
        AvenGraphAdjList *adj = &get(graph, v);
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
            get(*adj, i) = (x - 1) + y * width;
            i += 1;
        }
        if (y > 0) {
            get(*adj, i) = x + (y - 1) * width;
            i += 1;
        }
        if (x < width - 1) {
            get(*adj, i) = (x + 1) + y * width;
            i += 1;
        }
        if (y < height - 1) {
            get(*adj, i) = x + (y + 1) * width;
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

        get(embedding, v)[0] = -1.0f + (float)x * x_scale;
        get(embedding, v)[1] = -1.0f + (float)y * y_scale;
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
            get(ctx.embedding, 0),
            get(ctx.embedding, 1),
            get(ctx.embedding, 2)
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
            get(ctx.embedding, 0),
            get(ctx.embedding, 1),
            get(ctx.embedding, 2)
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
    vec2_copy(coords[0], get(ctx->embedding, v1));
    vec2_copy(coords[1], get(ctx->embedding, v2));
    vec2_copy(coords[2], get(ctx->embedding, v3));

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
        vec2_copy(face_coords[i], get(ctx->embedding, face.vertices[i]));
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

        float existing_area = get(ctx->faces, new_face_indices[i]).area;
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
            if (get(data->graph, v).len != 0) {
                continue;
            }

            uint32_t adj_start = (uint32_t)data->master_adj.len;
            get(data->graph, v).ptr = &data->master_adj.ptr[adj_start];

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

            get(data->graph, v).len = (uint32_t)(
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

typedef struct {
    uint32_t vertices[3];
    uint32_t neighbors[3];
} AvenGraphPlaneGenAbsFace;

static inline AvenGraph aven_graph_plane_gen_tri_abs(
    uint32_t size,
    AvenRng rng,
    AvenArena *arena
) {
    assert(size >= 3);

    List(uint32_t) master_adj = aven_arena_create_list(
        uint32_t,
        arena,
        6 * size - 12
    );
    AvenGraph graph = aven_arena_create_slice(
        AvenGraphAdjList,
        arena,
        size
    );

    for (uint32_t v = 0; v < graph.len; v += 1) {
        get(graph, v) = (AvenGraphAdjList){ .len = 0, .ptr = NULL };
    }

    AvenArena temp_arena = *arena;

    List(AvenGraphPlaneGenAbsFace) faces = aven_arena_create_list(
        AvenGraphPlaneGenAbsFace,
        &temp_arena,
        2 * size - 4
    );

    list_push(faces) = (AvenGraphPlaneGenAbsFace){
        .vertices = { 0, 2, 1 },
        .neighbors = { 1, 1, 1 },
    };
    list_push(faces) = (AvenGraphPlaneGenAbsFace){
        .vertices = { 0, 1, 2 },
        .neighbors = { 0, 0, 0 },
    };

    for (uint32_t v = 3; v < size; v += 1) {
        uint32_t face_index = 1 + aven_rng_rand_bounded(
            rng,
            (uint32_t)(faces.len - 1)
        );
        uint32_t edge_flips = aven_rng_rand_bounded(rng, 3);
        uint32_t flip_start = aven_rng_rand_bounded(rng, 3);

        AvenGraphPlaneGenAbsFace og_face = get(faces, face_index);

        uint32_t face_indices[3] = {
            face_index,
            (uint32_t)faces.len,
            (uint32_t)(faces.len + 1)
        };
        faces.len += 2;

        AvenGraphPlaneGenAbsFace *new_faces[3];
        for (size_t i = 0; i < 3; i += 1) {
            new_faces[i] = &get(faces, face_indices[i]);
            *(new_faces[i]) = (AvenGraphPlaneGenAbsFace){
                .vertices = {
                    v,
                    og_face.vertices[i],
                    og_face.vertices[(i + 1) % 3],
                },
                .neighbors = {
                    face_indices[(i + 2) % 3],
                    og_face.neighbors[i],
                    face_indices[(i + 1) % 3],
                },
            };
        }

        AvenGraphPlaneGenAbsFace *neighbor_faces[3];
        uint32_t neighbor_edge_indices[3];
        uint32_t neighbor_opposite_vertices[3];
        for (size_t i = 0; i < 3; i += 1) {
            uint32_t u = og_face.vertices[(i + 1) % 3];
            neighbor_faces[i] = &get(faces, og_face.neighbors[i]);
            uint32_t j = 0;
            for (; j < 3; j += 1) {
                if (neighbor_faces[i]->vertices[j] == u) {
                    neighbor_edge_indices[i] = j;
                    neighbor_opposite_vertices[i] =
                        neighbor_faces[i]->vertices[(j + 2) % 3];
                    neighbor_faces[i]->neighbors[j] = face_indices[i];
                    break;
                }
            }
            assert(j < 3);
        }

        if (
            edge_flips == 2 and
            neighbor_opposite_vertices[flip_start] ==
                neighbor_opposite_vertices[(flip_start + 1) % 3]
        ) {
            if (
                neighbor_opposite_vertices[flip_start] ==
                    neighbor_opposite_vertices[(flip_start + 2) % 3]
            ) {
                edge_flips -= 1;
            } else {
                flip_start += 1 + aven_rng_rand_bounded(rng, 1);
            }
        }

        for (uint32_t i = 0; i < edge_flips; i += 1) {
            uint32_t flip_index = (flip_start + i) % 3;
            if (og_face.neighbors[flip_index] == 0) {
                // never flip an edge of the outer triangle
                continue;
            }

            uint32_t nflip_index = neighbor_edge_indices[flip_index];

            AvenGraphPlaneGenAbsFace *face = new_faces[flip_index];
            AvenGraphPlaneGenAbsFace *neighbor = neighbor_faces[flip_index];

            {
                AvenGraphPlaneGenAbsFace *face_next_neighbor =
                    new_faces[(flip_index + 1) % 3];
                uint32_t j = 0;
                for (; j < 3; j += 1) {
                    if (face_next_neighbor->vertices[j] == v) {
                        break;
                    }
                }
                assert(j < 3);
                face_next_neighbor->neighbors[j] =
                    og_face.neighbors[flip_index];
            }
            {
                AvenGraphPlaneGenAbsFace *neighbor_prev_neighbor = &get(
                    faces,
                    neighbor->neighbors[(nflip_index + 1) % 3]
                );
                uint32_t j = 0;
                for (; j < 3; j += 1) {
                    if (
                        neighbor_prev_neighbor->vertices[j] ==
                            neighbor_opposite_vertices[flip_index]
                    ) {
                        break;
                    }
                }
                assert(j < 3);
                neighbor_prev_neighbor->neighbors[j] = face_indices[flip_index];
            }

            face->vertices[2] = neighbor->vertices[(nflip_index + 2) % 3];
            neighbor->vertices[(nflip_index + 1) % 3] = v;

            face->neighbors[1] = neighbor->neighbors[(nflip_index + 1) % 3];
            face->neighbors[2] = og_face.neighbors[flip_index];

            neighbor->neighbors[nflip_index] =
                face_indices[(flip_index + 1) % 3];
            neighbor->neighbors[(nflip_index + 1) % 3] =
                face_indices[flip_index];
        }
    }

    Slice(uint32_t) labels = aven_arena_create_slice(
        uint32_t,
        &temp_arena,
        size
    );

    for (uint32_t v = 0; v < labels.len; v += 1) {
        get(labels, v) = v;
    }

    for (uint32_t i = (uint32_t)labels.len; i > 4; i -= 1) {
        uint32_t j = 3 + aven_rng_rand_bounded(rng, i - 4);
        uint32_t tmp = get(labels, i - 1);
        get(labels, i - 1) = get(labels, j);
        get(labels, j) = tmp;
    }

    for (uint32_t i = 0; i < faces.len; i += 1) {
        AvenGraphPlaneGenAbsFace *face = &get(faces, i);

        for (uint32_t j = 0; j < 3; j += 1) {
            uint32_t v = face->vertices[j];
            uint32_t vl = get(labels, v);
            if (get(graph, vl).len != 0) {
                continue;
            }

            size_t adj_start = master_adj.len;

            list_push(master_adj) = get(
                labels,
                face->vertices[(j + 1) % 3]
            );
            get(graph, vl).ptr = &list_back(master_adj);

            uint32_t face_index = face->neighbors[j];
            while (face_index != i) {
                AvenGraphPlaneGenAbsFace *cur_face = &get(faces, face_index);

                uint32_t k = 0;
                for (; k < 3; k += 1) {
                    if (cur_face->vertices[k] == v) {
                        break;
                    }
                }
                assert(k < 3);

                list_push(master_adj) = get(
                    labels,
                    cur_face->vertices[(k + 1) % 3]
                );
                face_index = cur_face->neighbors[k];
            }

            get(graph, vl).len = master_adj.len - adj_start;
        }
    }

    assert(master_adj.len == master_adj.cap);

    return graph;
}

static uint32_t aven_graph_plane_gen_pyramid_coord(
    uint32_t k,
    uint32_t x,
    uint32_t y
) {
    assert(x < k - y);
    assert(y < k);
//    return ((2 * k - (y - 1)) * y) / 2 + x + 3;
    return k * y - ((y * (y - 1)) / 2) + x + 3;
}

static inline AvenGraph aven_graph_plane_gen_pyramid_abs(
    uint32_t k,
    AvenArena *arena
) {
    assert(k > 0);

    AvenGraph graph = { .len = ((k * (k + 1)) / 2) + 3 };
    graph.ptr = aven_arena_create_array(AvenGraphAdjList, arena, graph.len);

    {
        List(uint32_t) adj_list = aven_arena_create_list(
            uint32_t,
            arena,
            2 + 2 * k - 1
        );

        list_push(adj_list) = 2;

        for (uint32_t y = 0; y < k; y += 1) {
            uint32_t u = aven_graph_plane_gen_pyramid_coord(k, 0, y);
            list_push(adj_list) = u;
        }

        for (uint32_t x = 1; x < k; x += 1) {
            uint32_t y = (k - x) - 1;
            uint32_t u = aven_graph_plane_gen_pyramid_coord(k, x, y);
            list_push(adj_list) = u;
        }

        list_push(adj_list) = 1;

        assert(adj_list.len == adj_list.cap);
        get(graph, 0) = (AvenGraphAdjList)slice_list(adj_list);
    }

    {
        List(uint32_t) adj_list = aven_arena_create_list(
            uint32_t,
            arena,
            3
        );

        list_push(adj_list) = 0;

        {
            uint32_t u = aven_graph_plane_gen_pyramid_coord(k, k - 1, 0);
            list_push(adj_list) = u;
        }

        list_push(adj_list) = 2;

        assert(adj_list.len == adj_list.cap);
        get(graph, 1) = (AvenGraphAdjList)slice_list(adj_list);
    }

    {
        List(uint32_t) adj_list = aven_arena_create_list(
            uint32_t,
            arena,
            2 + k
        );

        list_push(adj_list) = 1;

        for (uint32_t x = k; x > 0; x -= 1) {
            uint32_t u = aven_graph_plane_gen_pyramid_coord(k, x - 1, 0);
            list_push(adj_list) = u;
        }

        list_push(adj_list) = 0;

        assert(adj_list.len == adj_list.cap);
        get(graph, 2) = (AvenGraphAdjList)slice_list(adj_list);
    }

    for (uint32_t y = 0; y < k; y += 1) {
        uint32_t width = k - y;
        for (uint32_t x = 0; x < width; x += 1) {
            uint32_t v = aven_graph_plane_gen_pyramid_coord(k, x, y);

            uint32_t adj_list_data[6];
            List(uint32_t) adj_list = list_array(adj_list_data);

            if (x > 0) {
                uint32_t u = aven_graph_plane_gen_pyramid_coord(k, x - 1, y);
                list_push(adj_list) = u;
            }
            if (y > 0) {
                list_push(adj_list) = aven_graph_plane_gen_pyramid_coord(k, x, y - 1);
                list_push(adj_list) = aven_graph_plane_gen_pyramid_coord(k, x + 1, y - 1);
            } else {
                list_push(adj_list) = 2;
                if (x == width - 1) {
                    list_push(adj_list) = 1;
                }
            }
            if (x < (width - 1)) {
                uint32_t u = aven_graph_plane_gen_pyramid_coord(k, x + 1, y);
                list_push(adj_list) = u;
            } else if ((width - 1) != 0) {
                list_push(adj_list) = 0;
            }
            if (y < (k - 1) and x < width - 1) {
                uint32_t u = aven_graph_plane_gen_pyramid_coord(k, x, y + 1);
                list_push(adj_list) = u;
            }
            if (x == 0) {
                list_push(adj_list) = 0;
            } else if (y < (k - 1)) {
                uint32_t u = aven_graph_plane_gen_pyramid_coord(k, x - 1, y + 1);
                list_push(adj_list) = u;
            }

            AvenGraphAdjList *v_adj = &get(graph, v);
            v_adj->len = adj_list.len;
            v_adj->ptr = aven_arena_create_array(uint32_t, arena, v_adj->len);
            for (size_t i = 0; i < adj_list.len; i += 1) {
                get(*v_adj, i) = get(adj_list, i);
            }
        }
    }

    return graph;
}

#endif // AVEN_GRAPH_PLANE_GEN_H
