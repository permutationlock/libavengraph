#ifndef GRAPH_PLANE_P3CHOOSE_TIKZ_H
#define GRAPH_PLANE_P3CHOOSE_TIKZ_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/math.h>

#include "../../../graph.h"
#include "../p3choose.h"

#include <stdio.h>

static inline void graph_plane_p3choose_tikz(
    GraphPlaneEmbedding embedding,
    GraphPlaneP3ChooseCtx *ctx,
    GraphPlaneP3ChooseFrame *frame,
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

        GraphAugAdjList v_adj = get(ctx->vertex_info, v).adj;
        for (uint32_t i = 0; i < v_adj.len; i += 1) {
            uint32_t u = get(v_adj, i).vertex;

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

        uint32_t v_mark = graph_plane_p3choose_vloc(ctx, frame, v)->mark;
        printf(
            "\t\\node (v%u) [label=%s:{%u,\\{",
            (unsigned int)v,
            dir_names[dir_index],
            (unsigned int)v_mark
        );

        GraphPlaneP3ChooseList v_colors = get(ctx->vertex_info, v).colors;
        for (uint8_t i = 0; i < v_colors.len - 1; i += 1) {
            printf("%u,", get(v_colors, i));
        }
        printf(
            "%u\\}}] at (%fcm, %fcm) {",
            get(v_colors, v_colors.len - 1),
            (double)pos_cm[0],
            (double)pos_cm[1]
        );

        if (v == frame->z) {
            printf("$z$");
        } else if (v == frame->y) {
            printf("$y$");
        } else if (v == frame->x) {
            printf("$x$");
        } else {
            bool done = false;
            for (size_t i = 0; i < ctx->frames.len; i += 1) {
                GraphPlaneP3ChooseFrame *stack_frame = &get(ctx->frames, i);

                if (v == stack_frame->z) {
                    printf("$z_{%lu}$", (unsigned long) i);
                    done = true;
                    break;
                }
            }
            if (!done) {
                for (size_t i = 0; i < ctx->frames.len; i += 1) {
                    GraphPlaneP3ChooseFrame *stack_frame = &get(
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
                    GraphPlaneP3ChooseFrame *stack_frame = &get(
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

    typedef Slice(bool) GraphPlaneP3ChooseTikzDrawSlice;
    typedef Slice(GraphPlaneP3ChooseTikzDrawSlice)
        GraphPlaneP3ChooseTikzDrawGraph;

    GraphPlaneP3ChooseTikzDrawGraph draw_graph = aven_arena_create_slice(
        GraphPlaneP3ChooseTikzDrawSlice,
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

    printf("\t\\begin{pgfonlayer}{bg}\n"); 

    {
        GraphAugAdjList z_adj = get(ctx->vertex_info, frame->z).adj;
        GraphPlaneP3ChooseTikzDrawSlice z_drawn = get(draw_graph, frame->z);
        GraphPlaneP3ChooseVertexLoc *z_loc =
            graph_plane_p3choose_vloc(ctx, frame, frame->z);
        uint32_t zv_index = z_loc->nb.first;
        if (zv_index != z_loc->nb.last) {
            zv_index = graph_aug_adj_next(z_adj, zv_index);
        }
        GraphAugAdjListNode zv = get(z_adj, zv_index);

        if (zv.vertex > frame->z) {
            get(z_drawn, zv_index) = true;
            printf(
                "\t\t\\draw (v%u) edge [double, very thick] (v%u);\n",
                (unsigned int)frame->z,
                (unsigned int)zv.vertex
            );
        } else {
            get(get(draw_graph, zv.vertex), zv.back_index) = true;
            printf(
                "\t\t\\draw (v%u) edge [double, very thick] (v%u);\n",
                (unsigned int)zv.vertex,
                (unsigned int)frame->z
            );
        }
    }

    GraphPlaneP3ChooseFrame *cur_frame = frame;
    size_t frame_index = (size_t)0 - (size_t)1;
    do {
        frame_index += 1;

        uint32_t v = cur_frame->z;

        do {
            GraphAugAdjList v_adj = get(ctx->vertex_info, v).adj;
            GraphPlaneP3ChooseTikzDrawSlice v_drawn = get(draw_graph, v);
            GraphPlaneP3ChooseVertexLoc *v_info =
                graph_plane_p3choose_vloc(ctx, cur_frame, v);

            for (
                uint32_t i = v_info->nb.first;
                i != v_info->nb.last;
                i = graph_aug_adj_next(v_adj, i)
            ) {
                uint32_t u = get(v_adj, i).vertex;
                if (u > v and !get(v_drawn, i)) {
                    get(v_drawn, i) = true;
                    printf(
                        "\t\t\\draw (v%u) edge (v%u);\n",
                        (unsigned int)v,
                        (unsigned int)u
                    );
                }
            }

            uint32_t u = get(v_adj, v_info->nb.last).vertex;
            if (u > v and !get(v_drawn, v_info->nb.last)) {
                get(v_drawn, v_info->nb.last) = true;
                printf(
                    "\t\t\\draw (v%u) edge (v%u);\n",
                    (unsigned int)v,
                    (unsigned int)u
                );
            }

            v = u;
        } while (v != cur_frame->z); 

        if (frame_index < ctx->frames.len) {
            cur_frame = &get(ctx->frames, frame_index);
        }
    } while (frame_index < ctx->frames.len);

    for (uint32_t v = 0; v < ctx->vertex_info.len; v += 1) {
        GraphAugAdjList v_adj = get(ctx->vertex_info, v).adj;
        GraphPlaneP3ChooseTikzDrawSlice v_drawn = get(draw_graph, v);

        for (uint32_t i = 0; i < v_adj.len; i += 1) {
            uint32_t u = get(v_adj, i).vertex;
            if (u > v and !get(v_drawn, i)) {
                get(v_drawn, i) = true;
                if (get(ctx->vertex_info, v).loc.mark == 0) {
                    printf(
                        "\t\t\\draw (v%u) edge (v%u);\n",
                        (unsigned int)v,
                        (unsigned int)u
                    );
                } else {
                    printf(
                        "\t\t\\draw (v%u) edge [dashed] (v%u);\n",
                        (unsigned int)v,
                        (unsigned int)u
                    );
                }
            }
        }
    }

    printf("\t\\end{pgfonlayer}\n");
    printf("\\end{tikzpicture}\n");
}

#endif // GRAPH_PLANE_P3CHOOSE_TIKZ_H
