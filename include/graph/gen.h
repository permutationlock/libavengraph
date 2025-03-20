#ifndef GRAPH_GEN_H
#define GRAPH_GEN_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/math.h>
#include <aven/rng.h>
#include "../graph.h"

static inline Graph graph_gen_complete(
    uint32_t size,
    AvenArena *arena
) {
    Graph graph = {
        .nb = { .len = size * (size - 1) },
        .adj = { .len = size },
    };
    graph.nb.ptr = aven_arena_create_array(uint32_t, arena, graph.nb.len);
    graph.adj.ptr = aven_arena_create_array(GraphAdj, arena, graph.adj.len);

    uint32_t i = 0;
    for (uint32_t v = 0; v < graph.adj.len; v += 1) {
        get(graph.adj, v).len = size - 1;
        get(graph.adj, v).index = i;

        for (uint32_t j = 0; j < graph.adj.len; j += 1) {
            uint32_t u = ((v & 1) == 0) ?
                ((uint32_t)graph.adj.len - (j + 1)) :
                j;
            if (u == v) {
                continue;
            }
            get(graph.nb, i) = u;
            i += 1;
        }
    }

    return graph;
}

static inline Graph graph_gen_grid(
    uint32_t width,
    uint32_t height,
    AvenArena *arena
) {
    Graph graph = {
        .nb = {
            .len = 4 * (width - 2) * (height - 2) +
                6 * (width - 2) +
                6 * (height - 2) +
                8,
        },
        .adj = { .len = width * height },
    };
    graph.nb.ptr = aven_arena_create_array(uint32_t, arena, graph.nb.len);
    graph.adj.ptr = aven_arena_create_array(GraphAdj, arena, graph.adj.len);

    uint32_t i = 0;
    for (uint32_t v = 0; v < graph.adj.len; v += 1) {
        GraphAdj *v_adj = &get(graph.adj, v);
        v_adj->index = i;

        uint32_t x = v % width;
        uint32_t y = v / width;

        if (x > 0) {
            get(graph.nb, i) = (x - 1) + y * width;
            i += 1;
        }
        if (y > 0) {
            get(graph.nb, i) = x + (y - 1) * width;
            i += 1;
        }
        if (x < width - 1) {
            get(graph.nb, i) = (x + 1) + y * width;
            i += 1;
        }
        if (y < height - 1) {
            get(graph.nb, i) = x + (y + 1) * width;
            i += 1;
        }

        v_adj->len = i - v_adj->index;
    }

    return graph;
}

static uint32_t graph_gen_pyramid_coord(
    uint32_t k,
    uint32_t x,
    uint32_t y
) {
    assert(x < k - y);
    assert(y < k);
    return k * y - ((y * (y - 1)) / 2) + x + 3;
}

static inline Graph graph_gen_pyramid(
    uint32_t k,
    AvenArena *arena
) {
    assert(k > 0);

    size_t size = ((k * (k + 1)) / 2) + 3;
    Graph graph = {
        .nb = { .len = 6 * size - 12 },
        .adj = { .len = size },
    };
    graph.nb.ptr = aven_arena_create_array(uint32_t, arena, graph.nb.len);
    graph.adj.ptr = aven_arena_create_array(GraphAdj, arena, graph.adj.len);

    uint32_t nb_index = 0;
    {
        get(graph.adj, 0).index = nb_index;

        get(graph.nb, nb_index) = 2;
        nb_index += 1;

        for (uint32_t y = 0; y < k; y += 1) {
            uint32_t u = graph_gen_pyramid_coord(k, 0, y);
            get(graph.nb, nb_index) = u;
            nb_index += 1;
        }

        for (uint32_t x = 1; x < k; x += 1) {
            uint32_t y = (k - x) - 1;
            uint32_t u = graph_gen_pyramid_coord(k, x, y);
            get(graph.nb, nb_index) = u;
            nb_index += 1;
        }

        get(graph.nb, nb_index) = 1;
        nb_index += 1;

        get(graph.adj, 0).len = nb_index - get(graph.adj, 0).index;
        assert(get(graph.adj, 0).len == 2 + 2 * k - 1);
    }

    {
        get(graph.adj, 1).index = nb_index;

        get(graph.nb, nb_index) = 0;
        nb_index += 1;

        {
            uint32_t u = graph_gen_pyramid_coord(k, k - 1, 0);
            get(graph.nb, nb_index) = u;
            nb_index += 1;
        }

        get(graph.nb, nb_index) = 2;
        nb_index += 1;

        get(graph.adj, 1).len = nb_index - get(graph.adj, 1).index;
        assert(get(graph.adj, 1).len == 3);
    }

    {
        get(graph.adj, 2).index = nb_index;

        get(graph.nb, nb_index) = 1;
        nb_index += 1;

        for (uint32_t x = k; x > 0; x -= 1) {
            uint32_t u = graph_gen_pyramid_coord(k, x - 1, 0);
            get(graph.nb, nb_index) = u;
            nb_index += 1;
        }

        get(graph.nb, nb_index) = 0;
        nb_index += 1;

        get(graph.adj, 2).len = nb_index - get(graph.adj, 2).index;
        assert(get(graph.adj, 2).len == 2 + k);
    }

    for (uint32_t y = 0; y < k; y += 1) {
        uint32_t width = k - y;
        for (uint32_t x = 0; x < width; x += 1) {
            uint32_t v = graph_gen_pyramid_coord(k, x, y);

            get(graph.adj, v).index = nb_index;

            if (x > 0) {
                uint32_t u = graph_gen_pyramid_coord(k, x - 1, y);
                get(graph.nb, nb_index) = u;
                nb_index += 1;
            }
            if (y > 0) {
                get(graph.nb, nb_index) = graph_gen_pyramid_coord(
                    k,
                    x,
                    y - 1
                );
                nb_index += 1;
                get(graph.nb, nb_index) = graph_gen_pyramid_coord(
                    k,
                    x + 1,
                    y - 1
                );
                nb_index += 1;
            } else {
                get(graph.nb, nb_index) = 2;
                nb_index += 1;
                if (x == width - 1) {
                    get(graph.nb, nb_index) = 1;
                    nb_index += 1;
                }
            }
            if (x < (width - 1)) {
                uint32_t u = graph_gen_pyramid_coord(k, x + 1, y);
                get(graph.nb, nb_index) = u;
                nb_index += 1;
            } else if ((width - 1) != 0) {
                get(graph.nb, nb_index) = 0;
                nb_index += 1;
            }
            if (y < (k - 1) and x < width - 1) {
                uint32_t u = graph_gen_pyramid_coord(k, x, y + 1);
                get(graph.nb, nb_index) = u;
                nb_index += 1;
            }
            if (x == 0) {
                get(graph.nb, nb_index) = 0;
                nb_index += 1;
            } else if (y < (k - 1)) {
                uint32_t u = graph_gen_pyramid_coord(k, x - 1, y + 1);
                get(graph.nb, nb_index) = u;
                nb_index += 1;
            }

            get(graph.adj, v).len = nb_index - get(graph.adj, v).index;
        }
    }

    return graph;
}

typedef struct {
    uint32_t vertices[3];
    uint32_t neighbors[3];
} GraphGenTriangle;

static inline Graph graph_gen_triangulation(
    uint32_t size,
    AvenRng rng,
    Vec2 flip_prob,
    AvenArena *arena
) {
    assert(size >= 3);

    Graph graph = {
        .nb = { .len = 6 * size - 12 },
        .adj = { .len = size },
    };

    graph.nb.ptr = aven_arena_create_array(
        uint32_t,
        arena,
        graph.nb.len
    );
    graph.adj.ptr = aven_arena_create_array(
        GraphAdj,
        arena,
        graph.adj.len
    );

    for (uint32_t v = 0; v < graph.adj.len; v += 1) {
        get(graph.adj, v) = (GraphAdj){ 0 };
    }

    AvenArena temp_arena = *arena;

    List(GraphGenTriangle) faces = aven_arena_create_list(
        GraphGenTriangle,
        &temp_arena,
        2 * size - 4
    );

    list_push(faces) = (GraphGenTriangle){
        .vertices = { 0, 2, 1 },
        .neighbors = { 1, 1, 1 },
    };
    list_push(faces) = (GraphGenTriangle){
        .vertices = { 0, 1, 2 },
        .neighbors = { 0, 0, 0 },
    };

    for (uint32_t v = 3; v < size; v += 1) {
        uint32_t face_index = 1 + aven_rng_rand_bounded(
            rng,
            (uint32_t)(faces.len - 1)
        );

        float r = aven_rng_randf(rng);
        uint32_t edge_flips = 0;
        if (r >= flip_prob[0]) {
            edge_flips += 1;
        }
        if (r >= flip_prob[1]) {
            edge_flips += 1;
        }
        uint32_t flip_start = aven_rng_rand_bounded(rng, 3);

        GraphGenTriangle og_face = get(faces, face_index);

        uint32_t face_indices[3] = {
            face_index,
            (uint32_t)faces.len,
            (uint32_t)(faces.len + 1)
        };
        faces.len += 2;

        GraphGenTriangle *new_faces[3];
        for (size_t i = 0; i < 3; i += 1) {
            new_faces[i] = &get(faces, face_indices[i]);
            *(new_faces[i]) = (GraphGenTriangle){
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

        GraphGenTriangle *neighbor_faces[3];
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

        // avoid creating double edges when flipping
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

            GraphGenTriangle *face = new_faces[flip_index];
            GraphGenTriangle *neighbor = neighbor_faces[flip_index];

            {
                GraphGenTriangle *face_next_neighbor =
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
                GraphGenTriangle *neighbor_prev_neighbor = &get(
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

    uint32_t nb_index = 0;
    for (uint32_t i = 0; i < faces.len; i += 1) {
        GraphGenTriangle *face = &get(faces, i);

        for (uint32_t j = 0; j < 3; j += 1) {
            uint32_t v = face->vertices[j];
            uint32_t vl = get(labels, v);
            if (get(graph.adj, vl).len != 0) {
                continue;
            }

            get(graph.adj, vl).index = nb_index;
            get(graph.nb, nb_index) = get(
                labels,
                face->vertices[(j + 1) % 3]
            );
            nb_index += 1;

            uint32_t face_index = face->neighbors[j];
            while (face_index != i) {
                GraphGenTriangle *cur_face = &get(faces, face_index);

                uint32_t k = 0;
                for (; k < 3; k += 1) {
                    if (cur_face->vertices[k] == v) {
                        break;
                    }
                }
                assert(k < 3);

                get(graph.nb, nb_index) = get(
                    labels,
                    cur_face->vertices[(k + 1) % 3]
                );
                nb_index += 1;
                face_index = cur_face->neighbors[k];
            }

            get(graph.adj, vl).len = nb_index - get(graph.adj, vl).index;
        }
    }

    assert((size_t)nb_index == graph.nb.len);

    return graph;
}

#endif // GRAPH_GEN_H

