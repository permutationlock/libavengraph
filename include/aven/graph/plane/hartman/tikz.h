#ifndef AVEN_GRAPH_PLANE_HARTMAN_TIKZ_H
#define AVEN_GRAPH_PLANE_HARTMAN_TIKZ_H

#include <stdio.h>

static inline void aven_graph_plane_hartman_tikz(
    AvenGraphPlaneEmbedding embedding,
    AvenGraphPlaneHartmanCtx *ctx,
    AvenGraphPlaneHartmanFrame *frame,
    Vec2 dim_cm
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
        Vec2 dirs[8] = {
            { 1.0f, 1.0f },
            { 1.0f, 0.0f },
            { 1.0f, -1.0f },
            { 0.0f, -1.0f },
            { -1.0f, -1.0f },
            { -1.0f, 0.0f },
            { -1.0f, 1.0f },
            { 0.0f, 1.0f },
        };
        float dir_score[8] = { 0 };

        AvenGraphAugAdjList v_adj = get(ctx->graph, v);
        for (uint32_t i = 0; i < v_adj.len; i += 1) {
            uint32_t u = get(v_adj, i).vertex;

            Vec2 u_pos;
            vec2_copy(u_pos, get(embedding, u));

            Vec2 vu;
            vec2_sub(vu, u_pos, pos);

            float vu_mag = vec2_mag(vu);
            vec2_scale(vu, vu_mag, vu);

            for (uint32_t j = 0; j < countof(dirs); j += 1) {
                float vu_dot_dir = vec2_dot(vu, dirs[j]);
                if (vu_dot_dir > 0.0f) {
                    dir_score[j] += vu_dot_dir / vu_mag;
                }
            }
        }

        uint32_t dir_index = 0;
        for (uint32_t i = 1; i < countof(dirs); i += 1) {
            if (dir_score[i] < dir_score[dir_index]) {
                dir_index = i;
            }
        }

        uint32_t v_mark = get(ctx->marks, v);
        printf(
            "\t(v%u) [label=%s:{%u,\\{",
            (unsigned int)v,
            dir_names[dir_index],
            (unsigned int)v_mark
        );

        AvenGraphPlaneHartmanList v_colors = get(ctx->color_lists, v);
        for (uint32_t i = 0; i < v_colors.len - 1; i += 1) {
            printf("%u,", get(v_colors, i));
        }
        printf(
            "%u\\}}] at (%fcm, %fcm) {",
            get(v_colors, v_colors.len - 1),
            (double)pos_cm[0],
            (double)pos_cm[1]
        );

        if (v == frame->z) {
            printf("z");
        } else if (v == frame->y) {
            printf("y");
        } else if (v == frame->x) {
            printf("x");
        } else {
            for (size_t i = 0; i < ctx->frames.len; i += 1) {
                AvenGraphPlaneHartmanFrame *stack_frame = &get(ctx->frames, i);

                if (v == stack_frame->z) {
                    printf("z_{%lu}", (unsigned long) i);
                } else if (v == stack_frame->y) {
                    printf("y_{%lu}", (unsigned long) i);
                } else if (v == stack_frame->x) {
                    printf("x_{%lu}", (unsigned long) i);
                }
            }
        }

        printf("};\n");
    }

    printf("\t\\begin{pgfonlayer}{bg}\n");
    for (uint32_t v = 0; v < embedding.len; v += 1) {
        if (get(ctx->vertex_info, v).mark != 0) {
            continue;
        }

        AvenGraphAugAdjList v_adj = get(ctx->graph, v);
        for (uint32_t i = 0; i < v_adj.len; i += 1) {
            uint32_t u = get(v_adj, i).vertex;
            if (u > v) {
                printf(
                    "\t\t\\draw (v%u) edge (v%u);\n",
                    (unsigned int)v,
                    (unsigned int)u
                );
            }
        }
    }
    AvenGraphPlaneHartmanFrame *cur_frame = frame;
    size_t frame_index = (size_t)0 - (size_t)1;
    do {
        frame_index += 1;

        uint32_t v = cur_frame->z;
        do {
            AvenGraphAugAdjList v_adj = get(ctx->graph, v);
            AvenGraphPlaneHartmanVertex *v_info = aven_graph_plane_hartman_vinfo(
                ctx,
                cur_frame,
                v
            );
            for (
                uint32_t i = v_info->nb.first;
                i != v_info->nb.last;
                i = aven_graph_aug_adj_next(v_adj, i)
            ) {
                uint32_t u = get(v_adj, i).vertex;
                if (u > v) {
                    printf(
                        "\t\t\\draw (v%u) edge (v%u);\n",
                        (unsigned int)v,
                        (unsigned int)u
                    );
                }
            }

            uint32_t u = get(v_adj, v_info->nb.last).vertex;
            if (u > v) {
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
    printf("\t\\end{pgfonlayer}\n");
    printf("\\end{tikzpicture}\n");
}

#endif // AVEN_GRAPH_PLANE_HARTMAN_TIKZ_H
