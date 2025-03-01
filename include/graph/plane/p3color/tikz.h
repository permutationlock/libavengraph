#ifndef GRAPH_PLANE_P3CHOOSE_TIKZ_H
#define GRAPH_PLANE_P3CHOOSE_TIKZ_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/math.h>

#include "../../../graph.h"
#include "../../plane.h"
#include "../p3color.h"

#include <stdio.h>

static inline void graph_plane_p3color_tikz(
    GraphPlaneEmbedding embedding,
    GraphPlaneP3ColorCtx *ctx,
    GraphPlaneP3ColorFrame *frame,
    Vec2 dim_cm,
    AvenArena temp_arena
) {
    Aff2 trans;
    aff2_identity(trans);
    aff2_stretch(trans, dim_cm, trans);
    aff2_scale(trans, 0.5f, trans);

    printf("\\begin{tikzpicture}\n");
    for (uint32_t v = 0; v < embedding.len; v += 1) {
        Vec2 pos;
        vec2_copy(pos, get(embedding, v));

        Vec2 pos_cm;
        aff2_transform(pos_cm, trans, pos);

        const char *dir_names[8] = {
            "above right",
            "right",
            "below right",
            "below",
            "below left",
            "left",
            "above left",
            "above"
        };
        const float sqrt2_2 = AVEN_MATH_SQRT2_F / 2.0f;
        Vec2 dirs[8] = {
            { sqrt2_2, sqrt2_2 },
            { 1.0f, 0.0f },
            { sqrt2_2, -sqrt2_2 },
            { 0.0f, -1.0f },
            { -sqrt2_2, -sqrt2_2 },
            { -1.0f, 0.0f },
            { -sqrt2_2, sqrt2_2 },
            { 0.0f, 1.0f },
        };
        float dir_score[8] = { 0 };
        float dir_bonus[8] = { 0 };

        GraphPlaneP3ColorVertex v_info = get(ctx->vertex_info, v);
        for (uint32_t i = 0; i < v_info.adj.len; i += 1) {
            uint32_t u = graph_nb(ctx->nb, v_info.adj, i);

            Vec2 u_pos;
            vec2_copy(u_pos, get(embedding, u));

            Vec2 vu;
            vec2_sub(vu, u_pos, pos);

            float vu_mag = vec2_mag(vu);
            vec2_scale(vu, 1.0f / vu_mag, vu);

            for (uint32_t j = 0; j < countof(dirs); j += 1) {
                float vu_dot_dir = vec2_dot(vu, dirs[j]);
                if (vu_dot_dir > 0.0f) {
                    dir_score[j] += vu_dot_dir * vu_dot_dir / vu_mag;
                } else {
                    dir_bonus[j] += vu_dot_dir * vu_dot_dir / vu_mag;
                }
            }
        }

        uint32_t dir_index = 0;
        for (uint32_t i = 1; i < countof(dirs); i += 1) {
            if (dir_score[i] < dir_score[dir_index]) {
                dir_index = i;
            } else if (
                dir_score[i] == dir_score[dir_index] and
                dir_bonus[i] > dir_bonus[dir_index]
            ) {
                dir_index = i;
            }
        }

        printf(
            "\t\\node (v%u) [label=%s:{%d}] at (%fcm, %fcm) {",
            (unsigned int)v,
            dir_names[dir_index],
            (int)v_info.mark,
            (double)pos_cm[0],
            (double)pos_cm[1]
        );

        if (v == frame->u) {
            printf("$u$");
        } else if (v == frame->z) {
            printf("$z$");
        } else if (v == frame->y) {
            printf("$y$");
        } else if (v == frame->x) {
            printf("$x$");
        } else {
            bool done = false;
            for (size_t i = 0; i < ctx->frames.len; i += 1) {
                GraphPlaneP3ColorFrame *stack_frame = &get(ctx->frames, i);

                if (v == stack_frame->u) {
                    printf("$u_{%lu}$", (unsigned long) i);
                    done = true;
                    break;
                }
            }
            if (!done) {
                for (size_t i = 0; i < ctx->frames.len; i += 1) {
                    GraphPlaneP3ColorFrame *stack_frame = &get(
                        ctx->frames,
                        i
                    );

                    if (v == stack_frame->z) {
                        printf("$z_{%lu}$", (unsigned long) i);
                        done = true;
                        break;
                    }
                }
            }
            if (!done) {
                for (size_t i = 0; i < ctx->frames.len; i += 1) {
                    GraphPlaneP3ColorFrame *stack_frame = &get(
                        ctx->frames,
                        i
                    );

                    if (v == stack_frame->y) {
                        printf("$y_{%lu}$", (unsigned long) i);
                        done = true;
                        break;
                    }
                }
            }
            if (!done) {
                for (size_t i = 0; i < ctx->frames.len; i += 1) {
                    GraphPlaneP3ColorFrame *stack_frame = &get(
                        ctx->frames,
                        i
                    );

                    if (v == stack_frame->x) {
                        printf("$x_{%lu}$", (unsigned long) i);
                        done = true;
                        break;
                    }
                }
            }
        }

        printf("};\n");
    }

    typedef Slice(bool) GraphPlaneP3ColorTikzDrawSlice;
    typedef Slice(GraphPlaneP3ColorTikzDrawSlice)
        GraphPlaneP3ColorTikzDrawGraph;

    GraphPlaneP3ColorTikzDrawGraph draw_graph = aven_arena_create_slice(
        GraphPlaneP3ColorTikzDrawSlice,
        &temp_arena,
        ctx->vertex_info.len
    );

    for (uint32_t v = 0; v < ctx->vertex_info.len; v += 1) {
        get(draw_graph, v).len = get(ctx->vertex_info, v).adj.len;
        get(draw_graph, v).ptr = aven_arena_create_array(
            bool,
            &temp_arena,
            get(draw_graph, v).len
        );

        for (size_t i = 0; i < get(draw_graph, v).len; i += 1) {
            get(get(draw_graph, v), i) = false;
        }
    }

    Queue(uint32_t) vertices = aven_arena_create_queue(
        uint32_t,
        &temp_arena,
        ctx->vertex_info.len
    );
    Slice(uint32_t) visited = aven_arena_create_slice(
        uint32_t,
        &temp_arena,
        ctx->vertex_info.len
    );

    for (uint32_t v = 0; v < ctx->vertex_info.len; v += 1) {
        get(visited, v) = 0;
    }

    printf("\t\\begin{pgfonlayer}{bg}\n");
    {
        GraphPlaneP3ColorVertex fu_info = get(ctx->vertex_info, frame->u);
        uint32_t n_index = frame->u_nb_first + frame->edge_index;
        if (frame->edge_index != 0) {
            n_index -= 1;
        }
        if (n_index >= fu_info.adj.len) {
            n_index -= fu_info.adj.len;
        }
        uint32_t n = graph_nb(ctx->nb, fu_info.adj, n_index);
        if (n > frame->u) {
            get(get(draw_graph, frame->u), n_index) = true;
            printf(
                "\t\t\\draw (v%u) edge [double, very thick] (v%u);\n",
                (unsigned int)frame->u,
                (unsigned int)n
            );
        } else {
            GraphPlaneP3ColorVertex n_info = get(ctx->vertex_info, n);
            for (uint32_t i = 0; i < n_info.adj.len; i += 1) {
                if (graph_nb(ctx->nb, n_info.adj, i) == frame->u) {
                    get(get(draw_graph, n), i) = true;
                    printf(
                        "\t\t\\draw (v%u) edge [double, very thick] (v%u);\n",
                        (unsigned int)n,
                        (unsigned int)frame->u
                    );
                    break;
                }
            }
        }
    }

    GraphPlaneP3ColorFrame *cur_frame = frame;
    uint32_t frame_index = (uint32_t)0 - (uint32_t)1;
    do {
        frame_index += 1;
        queue_clear(vertices);

        uint32_t mark = frame_index + 1;

        get(visited, cur_frame->u) = mark;

        GraphPlaneP3ColorVertex cfu_info = get(ctx->vertex_info, cur_frame->u);
        GraphPlaneP3ColorTikzDrawSlice cfu_drawn = get(
            draw_graph,
            cur_frame->u
        );
        uint32_t edge_index = cur_frame->edge_index;
        if (edge_index != 0) {
            edge_index = edge_index - 1;
        }
        for (uint32_t i = cur_frame->edge_index; i < cfu_info.adj.len; i += 1) {
            uint32_t n_index = cur_frame->u_nb_first + i;
            if (n_index >= cfu_info.adj.len) {
                n_index -= cfu_info.adj.len;
            }
            uint32_t n = graph_nb(ctx->nb, cfu_info.adj, n_index);
            if (get(ctx->vertex_info, n).mark <= 0) {
                get(visited, n) = mark;
                queue_push(vertices) = n;
            }
            if (n > cur_frame->u and !get(cfu_drawn, n_index)) {
                get(cfu_drawn, n_index) = true;
                if (frame_index == 0) {
                    printf(
                        "\t\t\\draw (v%u) edge [very thick] (v%u);\n",
                        (unsigned int)cur_frame->u,
                        (unsigned int)n
                    );
                } else {
                    printf(
                        "\t\t\\draw (v%u) edge [thin] (v%u);\n",
                        (unsigned int)cur_frame->u,
                        (unsigned int)n
                    );
                }
            }
        }

        if (cur_frame->x != cur_frame->u) {
            queue_push(vertices) = cur_frame->x;
            get(visited, cur_frame->x) = mark;
        }

        if (cur_frame->y != cur_frame->u) {
            queue_push(vertices) = cur_frame->y;
            get(visited, cur_frame->y) = mark;
        }

        if (cur_frame->z != cur_frame->u) {
            queue_push(vertices) = cur_frame->z;
            get(visited, cur_frame->z) = mark;
        }

        while (vertices.used > 0) {
            uint32_t v = queue_pop(vertices);
            GraphPlaneP3ColorTikzDrawSlice v_drawn = get(draw_graph, v);
            GraphPlaneP3ColorVertex v_info = get(ctx->vertex_info, v);

            for (uint32_t i = 0; i < v_info.adj.len; i += 1) {
                uint32_t u = graph_nb(ctx->nb, v_info.adj, i);
                GraphPlaneP3ColorVertex u_info = get(ctx->vertex_info, u);

                if (u_info.mark <= 0) {
                    if (get(visited, u) != mark) {
                        queue_push(vertices) = u;
                        get(visited, u) = mark;
                    }
                }

                if (u > v and !get(v_drawn, i)) {
                    get(v_drawn, i) = true;
                    if (frame_index == 0) {
                        printf(
                            "\t\t\\draw (v%u) edge [very thick] (v%u);\n",
                            (unsigned int)v,
                            (unsigned int)u
                        );
                    } else {
                        printf(
                            "\t\t\\draw (v%u) edge [thin] (v%u);\n",
                            (unsigned int)v,
                            (unsigned int)u
                        );
                    }
                }
            }
        }

        for (uint32_t v = 0; v < ctx->vertex_info.len; v += 1) {
            GraphPlaneP3ColorTikzDrawSlice v_drawn = get(draw_graph, v);
            GraphPlaneP3ColorVertex v_info = get(ctx->vertex_info, v);

            for (uint32_t i = 0; i < v_info.adj.len; i += 1) {
                uint32_t u = graph_nb(ctx->nb, v_info.adj, i);
                if (u < v or get(v_drawn, i)) {
                    continue;
                }

                uint32_t i_prev = graph_adj_prev(v_info.adj, i);
                uint32_t i_next = graph_adj_next(v_info.adj, i);

                uint32_t u_prev = graph_nb(ctx->nb, v_info.adj, i_prev);
                uint32_t u_next = graph_nb(ctx->nb, v_info.adj, i_next);

                bool u_prev_visited = get(visited, u_prev) == mark;
                bool u_next_visited = get(visited, u_next) == mark;
                bool u_visited = get(visited, u) == mark;

                if (
                    u == cur_frame->u or
                    u_prev == cur_frame->u or
                    u_next == cur_frame->u
                ) {
                    bool valid = false;
                    for (
                        uint32_t j = cur_frame->edge_index;
                        j < cfu_info.adj.len;
                        j += 1
                    ) {
                        uint32_t n_index = cur_frame->u_nb_first + j;
                        if (n_index >= cfu_info.adj.len) {
                            n_index -= cfu_info.adj.len;
                        }
                        uint32_t n = graph_nb(ctx->nb, cfu_info.adj, n_index);
                        if (n == v) {
                            valid = true;
                            break;
                        }
                    }

                    if (!valid) {
                        if (u == cur_frame->u) {
                            u_visited = false;
                        }
                        if (u_prev == cur_frame->u) {
                            u_prev_visited = false;
                        }
                        if (u_next == cur_frame->u) {
                            u_next_visited = false;
                        }
                    }
                }

                if (u_visited or u_prev_visited or u_next_visited) {
                    get(v_drawn, i) = true;
                    if (frame_index == 0) {
                        printf(
                            "\t\t\\draw (v%u) edge [very thick] (v%u);\n",
                            (unsigned int)v,
                            (unsigned int)u
                        );
                    } else {
                        printf(
                            "\t\t\\draw (v%u) edge [thin] (v%u);\n",
                            (unsigned int)v,
                            (unsigned int)u
                        );
                    }
                }
            }
        }

        if (frame_index < ctx->frames.len) {
            cur_frame = &get(ctx->frames, frame_index);
        }
    } while (frame_index < ctx->frames.len);

    for (uint32_t v = 0; v < ctx->vertex_info.len; v += 1) {
        GraphPlaneP3ColorTikzDrawSlice v_drawn = get(draw_graph, v);
        GraphPlaneP3ColorVertex v_info = get(ctx->vertex_info, v);

        for (uint32_t i = 0; i < v_info.adj.len; i += 1) {
            uint32_t u = graph_nb(ctx->nb, v_info.adj, i);
            if (u < v or get(v_drawn, i)) {
                continue;
            }
            get(v_drawn, i) = true;
            printf(
                "\t\t\\draw (v%u) [dashed] edge (v%u);\n",
                (unsigned int)v,
                (unsigned int)u
            );
        }
    }

    printf("\t\\end{pgfonlayer}\n");
    printf("\\end{tikzpicture}\n");
}

#endif // GRAPH_PLANE_P3CHOOSE_TIKZ_H
