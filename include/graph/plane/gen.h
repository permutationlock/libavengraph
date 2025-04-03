#ifndef GRAPH_PLANE_GEN_H
#define GRAPH_PLANE_GEN_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/rng.h>

#include <float.h>

#include "../../graph.h"
#include "../plane.h"
#include "../gen.h"

typedef struct {
    Graph graph;
    GraphPlaneEmbedding embedding;
} GraphPlaneGenData;

static inline GraphPlaneGenData graph_plane_gen_grid(
    uint32_t width,
    uint32_t height,
    Aff2 trans,
    AvenArena *arena
) {
    Graph graph = graph_gen_grid(width, height, arena);

    GraphPlaneEmbedding embedding = { .len = graph.adj.len };
    embedding.ptr = aven_arena_create_array(Vec2, arena, embedding.len);

    float x_scale = 2.0f;
    if (width > 1) {
        x_scale /= (float)(width - 1);
    }
    float y_scale = 2.0f;
    if (height > 1) {
        y_scale /= (float)(height - 1);
    }

    for (uint32_t v = 0; v < graph.adj.len; v += 1) {
        uint32_t x = v % width;
        uint32_t y = v / width;

        get(embedding, v)[0] = -1.0f + (float)x * x_scale;
        get(embedding, v)[1] = -1.0f + (float)y * y_scale;
        aff2_transform(get(embedding, v), trans, get(embedding, v));
    }

    return (GraphPlaneGenData){
        .graph = graph,
        .embedding = embedding,
    };
}

static inline GraphPlaneGenData graph_plane_gen_pyramid(
    uint32_t k,
    Aff2 trans,
    float min_area,
    AvenArena *arena
) {
    AvenArena temp_arena;
    Graph graph;
    GraphPlaneEmbedding embedding;
    float area;
    do {
        temp_arena = *arena;
        graph = graph_gen_pyramid(k, &temp_arena);
        embedding = (GraphPlaneEmbedding)aven_arena_create_slice(
            Vec2,
            &temp_arena,
            graph.adj.len
        );
        area = 1.0f;

        aff2_transform(get(embedding, 0), trans, (Vec2){ 0.0f, 1.0f });
        aff2_transform(get(embedding, 1), trans, (Vec2){ 1.0f, -1.0f });
        aff2_transform(get(embedding, 2), trans, (Vec2){ -1.0f, -1.0f });

        float y_top_space = 0.5f;
        float y_bot_space = 0.1f;
        float side_space = 0.1f;
        // float side_scale = 1.0f / (float)(k + 2);
        float y_scale = (2.0f - y_top_space - y_bot_space) / (float)(k + 1);
        // float x_start_scale = 2.0f / (float)(k + 1);
        for (uint32_t y = 0; y < k; y += 1) {
            uint32_t width = k - y;
            float y_coord = -1.0f + y_bot_space + (float)(y + 1) * y_scale;
            float x_start = (y_coord - 1.0f) / 2.0f;
            float x_end = -(y_coord - 1.0f) / 2.0f;
            float x_scale = (x_end - x_start - 2.0f * side_space) /
                (float)(width + 1);
            for (uint32_t x = 0; x < width; x += 1) {
                uint32_t v = graph_gen_pyramid_coord(k, x, y);
                float x_coord = x_start + side_space +
                    (float)(x + 1) * x_scale;
                aff2_transform(
                    get(embedding, v),
                    trans,
                    (Vec2){ x_coord, y_coord }
                );
            }
        }
        for (uint32_t y = 0; y < min(2, k); y += 1) {
            uint32_t width = k - y;
            uint32_t v = graph_gen_pyramid_coord(k, 0, y);
            if (y == 0) {
                uint32_t u = (width == 1) ?
                    1 :
                    graph_gen_pyramid_coord(k, 1, y);
                area = min(
                    area,
                    vec2_triangle_area(
                        get(embedding, 2),
                        get(embedding, v),
                        get(embedding, u)
                    )
                );
            } else {
                area = min(
                    area,
                    vec2_triangle_area(
                        get(embedding, 2),
                        get(embedding, v),
                        get(embedding, 0)
                    )
                );
            }
        }

        k -= 1;
    } while(area < min_area and k > 0);

    *arena = temp_arena;

    return (GraphPlaneGenData){
        .graph = graph,
        .embedding = embedding,
    };
}

typedef struct {
    uint32_t vertices[3];
    uint32_t neighbors[3];
    float area;
    bool invalid;
} GraphPlaneGenFace;

typedef Slice(GraphPlaneGenFace) GraphPlaneGenFaceSlice;

typedef struct {
    List(Vec2) embedding;
    List(GraphPlaneGenFace) faces;
    List(uint32_t) valid_faces;
    uint32_t active_face;
    float min_coeff;
    float min_area;
    bool square;
} GraphPlaneGenTriangulationCtx;

static inline GraphPlaneGenTriangulationCtx graph_plane_gen_triangulation_init(
    GraphPlaneEmbedding embedding,
    Aff2 trans,
    float min_area,
    float min_coeff,
    bool square,
    AvenArena *arena
) {
    GraphPlaneGenTriangulationCtx ctx = {
        .embedding = { .ptr = embedding.ptr, .cap = embedding.len },
        .faces = { .cap = 2 * embedding.len - 4 },
        .valid_faces = { .cap = 2 * embedding.len - 4 },
        .active_face = 0,
        .min_area = 2.0f * min_area,
        .min_coeff = min_coeff,
        .square = square,
    };

    ctx.faces.ptr = aven_arena_create_array(
        GraphPlaneGenFace,
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

        list_push(ctx.faces) = (GraphPlaneGenFace){
            .vertices = { 0, 2, 1 },
            .neighbors = { 1, 1, 1 },
            .area = -1.0f,
        };
        list_push(ctx.faces) = (GraphPlaneGenFace){
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

        list_push(ctx.faces) = (GraphPlaneGenFace){
            .vertices = { 2, 1, 3 },
            .neighbors = { 2, 1, 3 },
            .area = -1.0f,
        };
        list_push(ctx.faces) = (GraphPlaneGenFace){
            .vertices = { 1, 0, 3 },
            .neighbors = { 2, 3, 0 },
            .area = -1.0f,
        };
        list_push(ctx.faces) = (GraphPlaneGenFace){
            .vertices = { 0, 1, 2 },
            .neighbors = { 1, 0, 3 },
            .area = area,
        };
        list_push(ctx.faces) = (GraphPlaneGenFace){
            .vertices = { 0, 2, 3 },
            .neighbors = { 2, 0, 1 },
            .area = area,
        };

        list_push(ctx.valid_faces) = 2;
        list_push(ctx.valid_faces) = 3;
    }

    return ctx;
}

static float graph_plane_gen_triangulation_face_area(
    GraphPlaneGenTriangulationCtx *ctx,
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

static inline bool graph_plane_gen_triangulation_step(
    GraphPlaneGenTriangulationCtx *ctx,
    AvenRng rng
) {
    if (ctx->embedding.len == ctx->embedding.cap) {
        return true;
    }

    if (ctx->active_face == 0) {
        uint32_t tries = (uint32_t)ctx->valid_faces.len;
        while (tries != 0) {
            uint32_t valid_face_index = aven_rng_rand_bounded(
                rng,
                (uint32_t)ctx->valid_faces.len
            );
            ctx->active_face = list_get(ctx->valid_faces, valid_face_index) + 1;

            GraphPlaneGenFace *face = &list_get(
                ctx->faces,
                ctx->active_face - 1
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

        ctx->active_face = 0;
        return true;
    }

    GraphPlaneGenFace face = list_get(ctx->faces, ctx->active_face - 1);

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
        ctx->active_face - 1,
        (uint32_t)ctx->faces.len,
        (uint32_t)ctx->faces.len + 1,
    };

    list_push(ctx->faces) = (GraphPlaneGenFace){ 0 };
    list_push(ctx->faces) = (GraphPlaneGenFace){ 0 };

    list_push(ctx->valid_faces) = new_face_indices[1];
    list_push(ctx->valid_faces) = new_face_indices[2];

    for (uint32_t i = 0; i < 3; i += 1) {
        uint32_t nexti = (i + 1) % 3;
        uint32_t previ = (i + 2) % 3;

        GraphPlaneGenFace *new_face = &list_get(
            ctx->faces,
            new_face_indices[i]
        );

        new_face->vertices[0] = v;
        new_face->vertices[1] = face.vertices[i];
        new_face->vertices[2] = face.vertices[nexti];

        new_face->neighbors[0] = new_face_indices[previ];
        new_face->neighbors[1] = face.neighbors[i];
        new_face->neighbors[2] = new_face_indices[nexti];

        new_face->area = graph_plane_gen_triangulation_face_area(
            ctx,
            new_face->vertices[0],
            new_face->vertices[1],
            new_face->vertices[2]
        );
    }

    for (uint32_t i = 0; i < 3; i += 1) {
        GraphPlaneGenFace *neighbor = &list_get(
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
    } GraphPlaneGenNeighbor;
    GraphPlaneGenNeighbor neighbor_data[3];
    List(GraphPlaneGenNeighbor) valid_neighbors = list_array(neighbor_data);

    // uint32_t flips = aven_rng_rand(rng);

    for (uint32_t i = 0; i < 3; i += 1) {
        GraphPlaneGenFace *neighbor = &list_get(
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

        float new_area = graph_plane_gen_triangulation_face_area(
            ctx,
            v,
            f1,
            n
        );
        float new_neighbor_area = graph_plane_gen_triangulation_face_area(
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
            list_push(valid_neighbors) = (GraphPlaneGenNeighbor){
                .neighbor_index = i,
                .vertex_index = k,
                .new_area = new_area,
                .new_neighbor_area = new_neighbor_area,
            };
        }
    }

    // Flip selected valid edges around the newly split active face

    for (uint32_t i = 0; i < valid_neighbors.len; i += 1) {
        GraphPlaneGenNeighbor *neighbor = &list_get(valid_neighbors, i);

        uint32_t new_face_index = new_face_indices[neighbor->neighbor_index];
        GraphPlaneGenFace *new_face = &list_get(ctx->faces, new_face_index);

        uint32_t neighbor_face_index = new_face->neighbors[1];
        GraphPlaneGenFace *neighbor_face = &list_get(
            ctx->faces,
            neighbor_face_index
        );

        uint32_t ncur = neighbor->vertex_index;
        uint32_t nnext = (ncur + 1) % 3;
        uint32_t nprev = (ncur + 2) % 3;

        GraphPlaneGenFace old_neighbor_face = *neighbor_face;

        neighbor_face->neighbors[nprev] = new_face_index;
        neighbor_face->neighbors[nnext] = new_face->neighbors[2];
        neighbor_face->vertices[nprev] = new_face->vertices[0];
        neighbor_face->area = neighbor->new_neighbor_area;

        new_face->neighbors[2] = new_face->neighbors[1];
        new_face->neighbors[1] = old_neighbor_face.neighbors[nprev];
        new_face->vertices[2] = old_neighbor_face.vertices[ncur];
        new_face->area = neighbor->new_area;

        GraphPlaneGenFace *neighbor_prev_neighbor = &list_get(
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

        GraphPlaneGenFace *new_face_next_neighbor = &list_get(
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

    ctx->active_face = 0;

    return false;
}

static inline Graph graph_plane_gen_triangulation_graph_alloc(
    uint32_t size,
    AvenArena *arena
) {
    assert(size >= 3);

    Graph graph = {
        .nb = { .len = 6 * size - 12 },
        .adj = { .len = size },
    };

    graph.adj.ptr = aven_arena_create_array(
        GraphAdj,
        arena,
        graph.adj.len
    );
    graph.nb.ptr = aven_arena_create_array(
        uint32_t,
        arena,
        graph.nb.len
    );

    return graph;
}

static inline GraphPlaneGenData graph_plane_gen_triangulation_data(
    GraphPlaneGenTriangulationCtx *ctx,
    Graph graph
) {
    assert(graph.adj.len >= ctx->embedding.len);
    graph.adj.len = ctx->embedding.len;
    for (uint32_t v = 0; v < graph.adj.len; v += 1) {
        get(graph.adj, v) = (GraphAdj){ 0 };
    }

    uint32_t nb_index = 0;
    for (uint32_t i = 0; i < ctx->faces.len; i += 1) {
        GraphPlaneGenFace *face = &list_get(ctx->faces, i);

        for (uint32_t j = 0; j < 3; j += 1) {
            uint32_t v = face->vertices[j];
            if (get(graph.adj, v).len != 0) {
                continue;
            }

            get(graph.adj, v).index = nb_index;

            {
                uint32_t u = face->vertices[(j + 1) % 3];
                if (!ctx->square) {
                    get(graph.nb, nb_index) = u;
                    nb_index += 1;
                } else {
                    if ((u != 1 or v != 3) and (u != 3 or v != 1)) {
                        get(graph.nb, nb_index) = u;
                        nb_index += 1;
                    }
                }
            }

            uint32_t face_index = face->neighbors[j];

            while (face_index != i) {
                GraphPlaneGenFace *cur_face = &list_get(
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
                    get(graph.nb, nb_index) = u;
                    nb_index += 1;
                } else {
                    if ((u != 1 or v != 3) and (u != 3 or v != 1)) {
                        get(graph.nb, nb_index) = u;
                        nb_index += 1;
                    }
                }
                face_index = cur_face->neighbors[k];
            }

            get(graph.adj, v).len = nb_index - get(graph.adj, v).index;
        }
    }

    return (GraphPlaneGenData){
        .graph = graph,
        .embedding = (GraphPlaneEmbedding){
            .ptr = ctx->embedding.ptr,
            .len = ctx->embedding.len,
        },
    };
}

static inline GraphPlaneGenData graph_plane_gen_triangulation(
    uint32_t size,
    Aff2 trans,
    float min_area,
    float min_coeff,
    bool square,
    AvenRng rng,
    AvenArena *arena
) {
    assert(size >= 3);

    Graph graph = graph_plane_gen_triangulation_graph_alloc(
        size,
        arena
    );
    GraphPlaneEmbedding embedding = { .len = size };
    embedding.ptr = aven_arena_create_array(
        Vec2,
        arena,
        embedding.len
    );

    AvenArena temp_arena = *arena;
    GraphPlaneGenTriangulationCtx ctx = graph_plane_gen_triangulation_init(
        embedding,
        trans,
        min_area,
        min_coeff,
        square,
        &temp_arena
    );
    while (!graph_plane_gen_triangulation_step(&ctx, rng)) {}

    return graph_plane_gen_triangulation_data(&ctx, graph);
}

#endif // GRAPH_PLANE_GEN_H
