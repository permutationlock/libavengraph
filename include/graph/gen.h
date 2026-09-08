#ifndef GRAPH_GEN_H
    #define GRAPH_GEN_H

    #include <aven.h>
    #include <aven/arena.h>
    #include <aven/math.h>
    #include <aven/rng.h>
    #include "../graph.h"

    static inline Graph graph_gen_path(uint32_t size, AvenArena *arena) {
        assert(size > 0);
        GraphAdjSlice adj = aven_arena_create_slice(GraphAdj, arena, size);
        List(uint32_t) nb = aven_arena_create_list(
            uint32_t,
            arena,
            2 * (size - 1)
        );

        for (uint32_t v = 0; v < size; v += 1) {
            get(adj, v).index = (uint32_t)nb.len;
            if (v > 0) {
                list_push(nb) = v - 1;
            }
            if (v < size - 1) {
                list_push(nb) = v + 1;
            }
            get(adj, v).len = (uint32_t)(nb.len - get(adj, v).index);
        }
        assert(nb.len == nb.cap);

        return (Graph){ .adj = adj, .nb = slice_list(nb) };
    }

    static inline Graph graph_gen_cycle(uint32_t size, AvenArena *arena) {
        assert(size > 0);
        GraphAdjSlice adj = aven_arena_create_slice(GraphAdj, arena, size);
        List(uint32_t) nb = aven_arena_create_list(uint32_t, arena, 2 * size);

        for (uint32_t v = 0; v < size; v += 1) {
            get(adj, v).index = (uint32_t)nb.len;
            get(adj, v).len = 2;

            if (v > 0) {
                list_push(nb) = v - 1;
            } else {
                list_push(nb) = size - 1;
            }
            if (v < size - 1) {
                list_push(nb) = v + 1;
            } else {
                list_push(nb) = 0;
            }
        }
        assert(nb.len == nb.cap);

        return (Graph){ .adj = adj, .nb = slice_list(nb) };
    }

    static inline Graph graph_gen_complete(uint32_t size, AvenArena *arena) {
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
        assert(width > 1 and height > 1);
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

    static uint32_t graph_gen_pyramid_coord(uint32_t k, uint32_t x, uint32_t y) {
        assert(x < k - y);
        assert(y < k);
        return k * y - ((y * (y - 1)) / 2) + x + 3;
    }

    static inline Graph graph_gen_pyramid(uint32_t k, AvenArena *arena) {
        assert(k > 0);

        size_t size = ((k * (k + 1)) / 2) + 3;
        Graph graph = { .nb = { .len = 6 * size - 12 }, .adj = { .len = size } };
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

        Graph graph = { .nb = { .len = 6 * size - 12 }, .adj = { .len = size } };

        graph.nb.ptr = aven_arena_create_array(uint32_t, arena, graph.nb.len);
        graph.adj.ptr = aven_arena_create_array(GraphAdj, arena, graph.adj.len);

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
            uint32_t face_index = 1 +
                aven_rng_rand_bounded(rng, (uint32_t)(faces.len - 1));

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
                (uint32_t)(faces.len + 1),
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
                        neighbor_opposite_vertices[i] = neighbor_faces[i]
                            ->vertices[(j + 2) % 3];
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
                    GraphGenTriangle *face_next_neighbor = new_faces[
                        (flip_index + 1) % 3
                    ];
                    uint32_t j = 0;
                    for (; j < 3; j += 1) {
                        if (face_next_neighbor->vertices[j] == v) {
                            break;
                        }
                    }
                    assert(j < 3);
                    face_next_neighbor->neighbors[j] = og_face.neighbors[
                        flip_index
                    ];
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
                    neighbor_prev_neighbor->neighbors[j] = face_indices[
                        flip_index
                    ];
                }

                face->vertices[2] = neighbor->vertices[(nflip_index + 2) % 3];
                neighbor->vertices[(nflip_index + 1) % 3] = v;

                face->neighbors[1] = neighbor->neighbors[(nflip_index + 1) % 3];
                face->neighbors[2] = og_face.neighbors[flip_index];

                neighbor->neighbors[nflip_index] = face_indices[
                    (flip_index + 1) % 3
                ];
                neighbor->neighbors[(nflip_index + 1) % 3] = face_indices[
                    flip_index
                ];
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

    typedef struct {
        GraphDyn dgraph;
        Idx last_closure;
        Idx v;
        Idx nb;
        uint32_t root_idx;
        Idx root;
        Idx root_leaf;
    } AvenGraphGenTriangulation2PartialCtx;

    static AvenGraphGenTriangulation2PartialCtx graph_gen_triangulation2_init(
        uint32_t size,
        AvenRng rng,
        AvenArena *arena
    ) {
        assert(size >= 5);

        uint32_t n = size - 2;
        assert((uint64_t)n * 4 - 2 < UINT32_MAX);

        GraphDyn dgraph = graph_dyn_init(size, 3 * size - 6, arena);
        for (uint32_t v = 0; v < size; v += 1) {
            graph_dyn_insert_vertex(&dgraph);
        }

        AvenArena temp_arena = *arena;
        GraphPropUint8 leaf_count = aven_arena_create_slice(
            uint8_t,
            &temp_arena,
            size
        );
        for (uint32_t v = 0; v < size; v += 1) {
            get(leaf_count, v) = 0;
        }

        // generate random word of length 4n-2 of weight n-1
        uint32_t bit_len = 4U * n - 2U;
        uint32_t bit_final_len = (size_t)(bit_len % 32);
        uint32_t u32_len = (bit_len >> 5) + 1;
        Slice(uint32_t) word = aven_arena_create_slice(
            uint32_t,
            &temp_arena,
            u32_len
        );

        size_t set_bit_len = (n - 1) % 32;
        size_t set_u32_len = ((n - 1) >> 5);
        for (size_t i = 0; i < set_u32_len; i += 1) {
            get(word, i) = 0xffffffff;
        }
        get(word, set_u32_len) = (((uint32_t)0xffffffff) >> (32 - set_bit_len));
        for (size_t i = set_u32_len + 1; i < u32_len; i += 1) {
            get(word, i) = 0;
        }

        for (uint32_t tpos = 0; tpos < bit_len - 1; tpos += 1) {
            uint32_t tpos_i = tpos >> 5;
            uint32_t tpos_j = tpos % 32;

            uint32_t rnd_offset = aven_rng_rand_bounded(
                rng,
                (bit_len - 1) - tpos
            );
            uint32_t spos = tpos + rnd_offset;
            uint32_t spos_i = spos >> 5;
            uint32_t spos_j = spos % 32;

            uint32_t ttmp = get(word, tpos_i);
            uint32_t stmp = get(word, spos_i);
            uint32_t tmask = ((uint32_t)1 << tpos_j);
            uint32_t smask = ((uint32_t)1 << spos_j);
            get(word, tpos_i) &= ~tmask;
            get(word, tpos_i) |= (uint32_t)((stmp & smask) > 0) << tpos_j;
            get(word, spos_i) &= ~smask;
            get(word, spos_i) |= (uint32_t)((ttmp & tmask) > 0) << spos_j;
        }

        // find min pos on bit step graph: 1 -> +3, 0 -> -1
        size_t min_i = 0;
        size_t min_j = 0;
        int64_t min_sum = 0;
        int64_t sum = 0;
        for (size_t i = 0; i < u32_len; i += 1) {
            for (size_t j = 0; j < 32; j += 1) {
                if (i == u32_len - 1 && j == bit_final_len) {
                    break;
                }
                uint32_t pos = get(word, i) & (((uint32_t)1) << j);
                if (pos) {
                    if (sum < min_sum) {
                        min_i = i;
                        min_j = j;
                        min_sum = sum;
                    }
                    sum += 3;
                } else {
                    sum -= 1;
                }
            }
        }

        // rotate string to start at min

        // rotate 32bit words
        for (size_t i = 0; i < u32_len; i += 1) {
            uint32_t tmp = get(word, i);
            get(word, i) = get(word, u32_len - i - 1);
            get(word, u32_len - i - 1) = tmp;
        }
        for (size_t i = 0; i < min_i; i += 1) {
            uint32_t tmp = get(word, i);
            get(word, i) = get(word, u32_len - i - 1);
            get(word, u32_len - i - 1) = tmp;
        }
        for (size_t i = min_i; i < u32_len; i += 1) {
            uint32_t tmp = get(word, i);
            get(word, i) = get(word, u32_len - i - 1);
            get(word, u32_len - i - 1) = tmp;
        }

        // rotate bits within 32bit words
        uint32_t low_mask = (uint32_t)0xffffffff >> (32 - min_j);
        uint32_t high_mask = (uint32_t)0xffffffff << min_j;
        uint32_t old_start = get(word, 0);
        for (uint32_t i = 0; i < u32_len; i += 1) {
            uint32_t cur = get(word, i);
            uint32_t nex_i = (i < u32_len - 1) ? i + 1 : 0;
            uint32_t nex = (i < u32_len - 1) ? get(word, i + 1) : old_start;
            uint32_t low = (cur & high_mask) >> min_j;
            uint32_t top_nex_bit = (nex_i == u32_len - 1) ? bit_final_len : 32;
            uint32_t high = (nex & low_mask) << (top_nex_bit - min_j);
            get(word, i) = low | high;
        }

        // construct tree from bit string
        uint32_t v = 0;
        uint32_t next_v = 3;
        for (uint32_t i = 0; i < u32_len; i += 1) {
            for (uint32_t j = 0; j < 32; j += 1) {
                if (i == u32_len - 1 && j == bit_final_len) {
                    break;
                }
                Idx dv = idx_wrap(v);
                uint32_t pos = get(word, i) & (((uint32_t)1) << j);
                if (pos == 0) {
                    if (get(leaf_count, v) < 2) {
                        graph_dyn_insert_half_edge(
                            &dgraph,
                            dv,
                            graph_dyn_nb_last(dgraph, dv),
                            (Idx){ 0 }
                        );
                        get(leaf_count, v) += 1;
                    } else {
                        dv = graph_dyn_nb_vertex(
                            dgraph,
                            graph_dyn_nb(dgraph, dv)
                        );
                        v = idx_unwrap(dv);
                    }
                } else {
                    graph_dyn_insert_edge(
                        &dgraph,
                        dv,
                        graph_dyn_nb_last(dgraph, dv),
                        idx_wrap(next_v),
                        (Idx){ 0 }
                    );
                    v = next_v;
                    next_v += 1;
                }
            }
        }

        return (AvenGraphGenTriangulation2PartialCtx){
            .dgraph = dgraph,
            .v = idx_wrap(0),
            .nb = graph_dyn_nb(dgraph, idx_wrap(0)),
        };
    }

    static bool graph_gen_triangulation2_partial_step(
        AvenGraphGenTriangulation2PartialCtx *ctx
    ) {
        Idx v1 = ctx->v;
        Idx nb1 = ctx->nb;
        Idx v2 = graph_dyn_nb_vertex(ctx->dgraph, nb1);

        // If nb1 (v1 -> v2) is pendant, i.e. v2 is a leaf, then
        // skip past nb1, as well as the next leaf, if any. If the
        // next edge was marked as 'last_closure', then we have made
        // a full walk of the outer face without finding a pattern
        // and are done.
        if (!idx_valid(v2)) {
            ctx->nb = graph_dyn_nb_next(ctx->dgraph, ctx->nb);
            if (!idx_valid(graph_dyn_nb_vertex(ctx->dgraph, ctx->nb))) {
                ctx->root = ctx->v;
                ctx->root_leaf = ctx->nb;
                ctx->nb = graph_dyn_nb_next(ctx->dgraph, ctx->nb);
            }
            return idx_valid(ctx->last_closure) &&
                (idx_unwrap(ctx->nb) == idx_unwrap(ctx->last_closure));
        }

        Idx nb2 = graph_dyn_nb_next(
            ctx->dgraph,
            graph_dyn_nb_back(ctx->dgraph, nb1)
        );
        Idx v3 = graph_dyn_nb_vertex(ctx->dgraph, nb2);

        // If nb2 (v2 -> v3) is pendant, i.e. v3 is a leaf, then we
        // there is no pattern from v1 and we move to v2, skipping
        // leaves and checking for the end condition as above.
        if (!idx_valid(v3)) {
            ctx->v = v2;
            ctx->nb = graph_dyn_nb_next(ctx->dgraph, nb2);
            if (!idx_valid(graph_dyn_nb_vertex(ctx->dgraph, ctx->nb))) {
                ctx->root = ctx->v;
                ctx->root_leaf = ctx->nb;
                ctx->nb = graph_dyn_nb_next(ctx->dgraph, ctx->nb);
            }
            return idx_valid(ctx->last_closure) &&
                (idx_unwrap(ctx->nb) == idx_unwrap(ctx->last_closure));
        }

        Idx nb3 = graph_dyn_nb_next(
            ctx->dgraph,
            graph_dyn_nb_back(ctx->dgraph, nb2)
        );
        Idx v4 = graph_dyn_nb_vertex(ctx->dgraph, nb3);

        // Walk until we hit a leaf to match the closure pattern:
        //     inner -> inner -> inner -> leaf
        while (idx_valid(v4)) {
            v1 = v2;
            nb1 = nb2;
            v2 = v3;
            nb2 = nb3;
            v3 = v4;
            nb3 = graph_dyn_nb_next(
                ctx->dgraph,
                graph_dyn_nb_back(ctx->dgraph, nb2)
            );
            v4 = graph_dyn_nb_vertex(ctx->dgraph, nb3);
        }

        // Perform a partial closure:
        //     v1 - nb1 > v2 - nb2 > v3 - nb3 > leaf
        // 
        //          into
        // 
        //       v1 - nb1 > v2
        //        \        /
        //     nb1_new   nb2
        //          v    v
        //            v3
        Idx nb1_prev = graph_dyn_nb_prev(ctx->dgraph, nb1);
        Idx nb3_prev = graph_dyn_delete_half_edge(&ctx->dgraph, v3, nb3);
        GraphDynEdge new_edge = graph_dyn_insert_edge(
            &ctx->dgraph,
            v1,
            nb1_prev,
            v3,
            nb3_prev
        );
        Idx nb1_new = new_edge.nb1;

        // If the edge nb0 (v0 -> v1) before nb1 (v1 -> v3) on the new
        // outer face is an inner edge, i.e. v0 is not a leaf, we backtrack
        // to v0, nb0 (v0 -> v1) to check for newly revealed patterns.
        // Otherwise, we continue from v1, nb1_new (v1 -> v3).
        Idx v0 = graph_dyn_nb_vertex(ctx->dgraph, nb1_prev);
        if (idx_valid(v0)) {
            ctx->v = v0;
            ctx->nb = graph_dyn_nb_back(ctx->dgraph, nb1_prev);
        } else {
            ctx->nb = nb1_new;
        }
        ctx->last_closure = ctx->nb;

        return false;
    }

    typedef struct {
        GraphDyn dgraph;
        Idx root;
        Idx v;
        Idx vnb;
        Idx u;
        Idx unb;
    } AvenGraphGenTriangulation2FullCtx;

    static AvenGraphGenTriangulation2FullCtx graph_gen_triangulation2_full_init(
        AvenGraphGenTriangulation2PartialCtx *ctx
    ) {
        assert(idx_unwrap(ctx->nb) == idx_unwrap(ctx->last_closure));
        assert(idx_valid(ctx->root));
        assert(idx_valid(ctx->root_leaf));

        // We have a graph of the form:
        //             l1    l2
        //               \  /
        //                v0
        //             \ /  \ /
        //              o    o
        //             /      \ /
        //            o        o
        //           / \      /
        //              o    o
        //             / \  / \ 
        //                v0'
        //               /  \ 
        //             l2'  l1'
        // We will create a new vertex v1 and walk from l1 to l2',
        // replacing leaves with edges to v1. Then we'll create a
        // vertex v2 and walk from l1' to l2, replacing leaves with
        // edges to v2. finally we'll add the final edge v1 to v2
        // to complete the outer triangle.
        return (AvenGraphGenTriangulation2FullCtx){
            .dgraph = ctx->dgraph,
            .v = ctx->root,
            .vnb = ctx->root_leaf,
            .u = idx_wrap(1),
        };
    }

    static bool graph_gen_triangulation2_full_step(
        AvenGraphGenTriangulation2FullCtx *ctx
    ) {
        assert(!idx_valid(graph_dyn_nb_vertex(ctx->dgraph, ctx->vnb)));
        ctx->vnb = graph_dyn_delete_half_edge(&ctx->dgraph, ctx->v, ctx->vnb);
        GraphDynEdge next_nbs = graph_dyn_insert_edge(
            &ctx->dgraph,
            ctx->v,
            ctx->vnb,
            ctx->u,
            ctx->unb
        );
        ctx->vnb = graph_dyn_nb_next(ctx->dgraph, next_nbs.nb1);
        ctx->unb = graph_dyn_nb_prev(ctx->dgraph, next_nbs.nb2);

        if (!idx_valid(ctx->root)) {
            // Set the root stopping point after first vertex
            ctx->root = ctx->v;
        } else if (idx_unwrap(ctx->v) == idx_unwrap(ctx->root)) {
            // We're back at the root, insert the final edge 1 -> 2
            graph_dyn_insert_edge(
                &ctx->dgraph,
                idx_wrap(1),
                graph_dyn_nb(ctx->dgraph, idx_wrap(1)),
                idx_wrap(2),
                graph_dyn_nb(ctx->dgraph, idx_wrap(2))
            );
            return true;
        }

        Idx next_v = graph_dyn_nb_vertex(ctx->dgraph, ctx->vnb);
        if (!idx_valid(next_v)) {
            // If the next edge is pendant, we finished one pass,
            // swap to inserting edges to 2 and continue
            ctx->u = idx_wrap(2);
            ctx->unb = (Idx){ 0 };
            return false;
        }

        // Otherwise the edge is interior, move to the next face vertex
        ctx->v = next_v;
        ctx->vnb = graph_dyn_nb_next(
            ctx->dgraph,
            graph_dyn_nb_back(ctx->dgraph, ctx->vnb)
        );
        return false;
    }

    static inline Graph graph_gen_triangulation2(
        uint32_t size,
        AvenRng rng,
        AvenArena *arena
    ) {
        Graph graph = graph_from_dyn_init(size, 3 * size - 6, arena);
        AvenArena temp_arena = *arena;
        AvenGraphGenTriangulation2PartialCtx part_ctx =
            graph_gen_triangulation2_init(size, rng, &temp_arena);
        while (!graph_gen_triangulation2_partial_step(&part_ctx)) {}
        AvenGraphGenTriangulation2FullCtx full_ctx =
            graph_gen_triangulation2_full_init(&part_ctx);
        while (!graph_gen_triangulation2_full_step(&full_ctx)) {}
        graph_from_dyn(graph, full_ctx.dgraph, temp_arena);
        return graph;
    }

#endif // GRAPH_GEN_H

