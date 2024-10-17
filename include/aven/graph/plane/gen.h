#ifndef AVEN_GRAPH_PLANE_GEN_H
#define AVEN_GRAPH_PLANE_GEN_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/rng.h>

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
#define AVEN_GRAPH_PLANE_GEN_VERTEX_TM 0xffffffffUL
#define AVEN_GRAPH_PLANE_GEN_VERTEX_BR (AVEN_GRAPH_PLANE_GEN_VERTEX_TM - 1UL)
#define AVEN_GRAPH_PLANE_GEN_VERTEX_BL (AVEN_GRAPH_PLANE_GEN_VERTEX_TM - 2UL)

typedef struct {
    uint32_t vertices[3];
    uint32_t neighbors[3];
} AvenGraphPlaneGenFace;

typedef Slice(AvenGraphPlaneGenFace) AvenGraphPlaneGenFaceSlice;

typedef struct {
    AvenGraph graph;
    List(Vec2) embedding;
    List(AvenGraphPlaneGenFace) faces;
    uint32_t active_face;
    float vertex_padding;
} AvenGraphPlaneGenTriCtx;

static inline AvenGraphPlaneGenTriCtx aven_graph_plane_gen_tri_init(
    uint32_t size,
    float vertex_padding,
    AvenArena *arena
) {
    AvenGraphPlaneGenTriCtx ctx = {
        .embedding = { .cap = size },
        .faces = { .cap = 2 * size - 4 },
        .active_face = 1,
        .vertex_padding = vertex_padding,
    };

    ctx.embedding.ptr = aven_arena_create_array(
        Vec2,
        arena,
        ctx.embedding.cap
    );

    ctx.faces.ptr = aven_arena_create_array(
        AvenGraphPlaneGenFace,
        arena,
        ctx.faces.len
    );

    list_push(ctx.faces) = (AvenGraphPlaneGenFace){
        .vertices = {
            AVEN_GRAPH_PLANE_GEN_VERTEX_TM,
            AVEN_GRAPH_PLANE_GEN_VERTEX_BL,
            AVEN_GRAPH_PLANE_GEN_VERTEX_BR,
        },
        .neighbors = { 1, 1, 1 },
    };
    list_push(ctx.faces) = (AvenGraphPlaneGenFace){
        .vertices = {
            AVEN_GRAPH_PLANE_GEN_VERTEX_TM,
            AVEN_GRAPH_PLANE_GEN_VERTEX_BR,
            AVEN_GRAPH_PLANE_GEN_VERTEX_BL,
        },
        .neighbors = { 0, 0, 0 },
    };

    return ctx;
}

static void aven_graph_plane_gen_tri_coords(
    Vec2 coords,
    AvenGraphPlaneGenTriCtx *ctx,
    uint32_t v
) {
    switch(v) {
        case AVEN_GRAPH_PLANE_GEN_VERTEX_TM:
            vec2_copy(coords, (Vec2){ 0.0f, 1.0f });
            break;
        case AVEN_GRAPH_PLANE_GEN_VERTEX_BR:
            vec2_copy(coords, (Vec2){ 1.0f, -1.0f });
            break;
        case AVEN_GRAPH_PLANE_GEN_VERTEX_BL:
            vec2_copy(coords, (Vec2){ -1.0f, -1.0f });
            break;
        default:
            vec2_copy(coords, slice_get(ctx->embedding, v));
            break;
    }
}

static inline bool aven_graph_plane_gen_tri_step(
    AvenGraphPlaneGenTriCtx *ctx,
    AvenRng rng
) {
    if (ctx->embedding.len == ctx->embedding.cap) {
        return true;
    }

    if (ctx->active_face == AVEN_GRAPH_PLANE_GEN_FACE_INVALID) {
        // Randomly pick a new active face (excluding the outer face)
        ctx->active_face = 1U + aven_rng_rand_bounded(
            rng,
            (uint32_t)ctx->faces.len - 1
        );
        return false;
    }

    AvenGraphPlaneGenFace face = list_get(ctx->faces, ctx->active_face);

    // Generate a random vertex contained within the active face satisfying
    // the vertex_spacing requirement

    Vec2 face_coords[3];
    for (uint32_t i = 0; i < 3; i += 1) {
        aven_graph_plane_gen_tri_coords(
            face_coords[i],
            ctx,
            face.vertices[i]
        );
    }

    Vec2 neighbor_coords[3];
    for (uint32_t i = 0; i < 3; i += 1) {
        AvenGraphPlaneGenFace *neighbor = &slice_get(
            ctx->faces,
            face.neighbors[i]
        );
        uint32_t u = face.vertices[i];
        uint32_t v = face.vertices[(i + 1) % 3];
        for (uint32_t j = 0; j < 3; j += 1) {
            uint32_t w = neighbor->vertices[j];
            if (w != v and w != u) {
                aven_graph_plane_gen_tri_coords(
                    neighbor_coords[i],
                    ctx,
                    w
                );
                break;
            }
        }
    }

    float min_mags[3] = { 0 };
    for (uint32_t i = 0; i < 3; i += 1) {
        uint32_t j = (i + 1) % 3;
        Vec2 edge;
        vec2_sub(edge, face_coords[i], face_coords[j]);
        float edge_len = vec2_mag(edge);

        if (edge_len < 2.0f * ctx->vertex_padding) {
            ctx->active_face = AVEN_GRAPH_PLANE_GEN_FACE_INVALID;
            return false;
        }

        float min_edge_mag = ctx->vertex_padding / edge_len;
        min_mags[i] = max(
            min_edge_mag,
            min_mags[i]
        );
        min_mags[j] = max(
            min_edge_mag,
            min_mags[j]
        );
    }

    float coeff[3];
    for (uint32_t i = 0; i < 2; i += 1) {
        coeff[i] = min_mags[i] +
            aven_rng_randf(rng) * (1.0f - 2.0f * min_mags[i]);
    }
    coeff[2] = 2.0f - (coeff[0] + coeff[1]);

    Vec2 vpos = { 0.0f, 0.0f };
    for (uint32_t i = 0; i < 3; i += 1) {
        Vec2 scaled_part;
        vec2_scale(scaled_part, coeff[i], face_coords[i]);
        vec2_add(vpos, vpos, scaled_part);
    }

    uint32_t v = (uint32_t)ctx->embedding.len;
    vec2_copy(list_push(ctx->embedding), (Vec2){ vpos[0], vpos[1] });

    // Determine if any faces neighboring the active face are valid for edge
    // flips with v

    typedef struct {
        uint32_t neighbor_index;
        uint32_t vertex_index;
    } AvenGraphPlaneGenNeighbor;
    AvenGraphPlaneGenNeighbor neighbor_data[4];
    List(AvenGraphPlaneGenNeighbor) valid_neighbors = list_array(neighbor_data);

    for (uint32_t i = 0; i < 3; i += 1) {
        AvenGraphPlaneGenFace *neighbor = &list_get(
            ctx->faces,
            face.neighbors[i]
        );

        uint32_t j = (i + 1) % 3;

        uint32_t f1 = face.vertices[i];
        uint32_t f2 = face.vertices[j];

        Vec2 f1pos;
        vec2_copy(f1pos, list_get(ctx->embedding, f1));

        Vec2 f2pos;
        vec2_copy(f2pos, list_get(ctx->embedding, f1));

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
        vec2_cwise_perp(perp1, vf1);

        Vec2 vf2;
        vec2_sub(vf2, f2pos, vpos);

        Vec2 perp2;
        vec2_ccwise_perp(perp2, vf2);

        float test1 = vec2_dot(vn, perp1);
        float test2 = vec2_dot(vn, perp2);

        if (test1 > 0.0f and test2 > 0.0f) {
            list_push(valid_neighbors) = (AvenGraphPlaneGenNeighbor){
                .neighbor_index = i,
                .vertex_index = k,
            };
        }
    }

    // Split the active face into three faces around the new vertex

    uint32_t new_face_indices[3] = {
        ctx->active_face,
        (uint32_t)ctx->faces.len,
        (uint32_t)ctx->faces.len + 1,
    };

    list_push(ctx->faces) = (AvenGraphPlaneGenFace){ 0 };
    list_push(ctx->faces) = (AvenGraphPlaneGenFace){ 0 };

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

    // Randomly flip valid edges around the newly split active face

    uint32_t flips = aven_rng_rand(rng);

    for (uint32_t i = 0; i < valid_neighbors.len; i += 1) {
        if ((flips & (1U << i)) == 0) {
            continue;
        }

        AvenGraphPlaneGenNeighbor *neighbor = &list_get(valid_neighbors, i);

        uint32_t new_face_index = new_face_indices[neighbor->neighbor_index];
        AvenGraphPlaneGenFace *new_face = &list_get(ctx->faces, new_face_index);

        AvenGraphPlaneGenFace *neighbor_face = &list_get(
            ctx->faces,
            new_face->neighbors[1]
        );

        AvenGraphPlaneGenFace old_neighbor_face = *neighbor_face;

        uint32_t ncur = neighbor->vertex_index;
        uint32_t nnext = (ncur + 1) % 3;
        uint32_t nprev = (ncur + 2) % 3;

        neighbor_face->neighbors[nprev] = new_face_index;
        neighbor_face->neighbors[nnext] = new_face->neighbors[2];
        neighbor_face->vertices[nprev] = new_face->vertices[0];

        new_face->neighbors[2] = new_face->neighbors[1];
        new_face->neighbors[1] = old_neighbor_face.neighbors[nprev];
        new_face->vertices[2] = old_neighbor_face.vertices[ncur];
    }

    ctx->active_face = AVEN_GRAPH_PLANE_GEN_FACE_INVALID;

    return false;
}

static inline AvenGraphPlaneGenData aven_graph_plane_gen_tri_data(
    AvenGraphPlaneGenTriCtx *ctx,
    AvenArena *arena
) {
    AvenGraph graph = { .len = ctx->embedding.len };
    graph.ptr = aven_arena_create_array(AvenGraphAdjList, arena, graph.len);

    for (uint32_t i = 0; i < graph.len; i += 1) {
        slice_get(graph, i) = (AvenGraphAdjList){ 0 };
    }

    List(uint32_t) master_adj = { .cap = 2 * (graph.len * 3 - 6) };
    master_adj.ptr = aven_arena_create_array(uint32_t, arena, master_adj.cap);

    for (uint32_t i = 0; i < ctx->faces.len; i += 1) {
        AvenGraphPlaneGenFace *face = &list_get(ctx->faces, i);

        for (uint32_t j = 0; j < 3; j += 1) {
            uint32_t v = face->vertices[j];
            if (slice_get(graph, v).len != 0) {
                continue;
            }

            uint32_t adj_start = (uint32_t)master_adj.len;
            slice_get(graph, v).ptr = &master_adj.ptr[adj_start];

            list_push(master_adj) = face->vertices[(j + 1) % 3];

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
                
                list_push(master_adj) = cur_face->neighbors[(k + 1) % 3];
            }

            slice_get(graph, v).len = (uint32_t)(master_adj.len - adj_start);
        }
    }

    AvenGraphPlaneEmbedding embedding = { .len = ctx->embedding.len };
    embedding.ptr = aven_arena_create_array(Vec2, arena, embedding.len);
    
    AvenGraphPlaneEmbedding ctx_embedding = slice_list(ctx->embedding);
    slice_copy(embedding, ctx_embedding);

    return (AvenGraphPlaneGenData){
        .graph = graph,
        .embedding = embedding,
    };
}

#endif // AVEN_GRAPH_PLANE_GEN_H
