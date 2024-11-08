#ifndef AVEN_GRAPH_PLANE_HARTMAN_GEOMETRY_H
#define AVEN_GRAPH_PLANE_HARTMAN_GEOMETRY_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/math.h>

#include <aven/gl/shape.h>

#include "../hartman.h"
#include "../geometry.h"

typedef struct {
    Slice(Vec4) colors;
    Vec4 outline_color;
    Vec4 active_color;
    Vec4 cycle_color;
    Vec4 py_color;
    Vec4 xp_color;
    float edge_thickness;
    float radius;
    float border_thickness;
} AvenGraphPlaneHartmanGeometryInfo;

static inline void aven_graph_plane_hartman_geometry_push_ctx(
    AvenGlShapeGeometry *geometry,
    AvenGlShapeRoundedGeometry *rounded_geometry,
    AvenGraphPlaneEmbedding embedding,
    AvenGraphPlaneHartmanCtx *ctx,
    Aff2 trans,
    AvenGraphPlaneHartmanGeometryInfo *info
) {
    AvenGraphPlaneHartmanFrame *frame = &list_get(
        ctx->frames,
        ctx->frames.len - 1
    );

    AvenGraphPlaneHartmanVertex v_info = *aven_graph_plane_hartman_vinfo(
        ctx,
        frame,
        frame->v
    );

    uint32_t x_mark = slice_get(ctx->marks, frame->x_info.mark);
    uint32_t y_mark = slice_get(ctx->marks, frame->y_info.mark);

    // Draw edges of outer cycle including active edge
    {
        AvenGraphPlaneGeometryEdge xp_edge_info = {
            .thickness = info->edge_thickness + info->border_thickness,
        };
        vec4_copy(xp_edge_info.color, info->xp_color);
        AvenGraphPlaneGeometryEdge py_edge_info = {
            .thickness = info->edge_thickness + info->border_thickness,
        };
        vec4_copy(py_edge_info.color, info->py_color);
        AvenGraphPlaneGeometryEdge cycle_edge_info = {
            .thickness = info->edge_thickness + info->border_thickness,
        };
        vec4_copy(cycle_edge_info.color, info->cycle_color);

        uint32_t v = frame->x;
        AvenGraphPlaneHartmanNeighbors v_nb = frame->x_info.nb;
        uint32_t v_mark = slice_get(ctx->marks, frame->x_info.mark);
        do {
            AvenGraphAugAdjList v_aug_adj = slice_get(ctx->graph, v);

            float radius = info->radius + info->border_thickness;
            if (v != frame->v and v_mark == y_mark) {
                Aff2 node_x_trans;
                aff2_identity(node_x_trans);
                aff2_stretch(
                    node_x_trans,
                    (Vec2){ radius, radius },
                    node_x_trans
                );
                aff2_add_vec2(
                    node_x_trans,
                    node_x_trans,
                    slice_get(embedding, v)
                );
                aven_gl_shape_rounded_geometry_push_square(
                    rounded_geometry,
                    node_x_trans,
                    1.0f,
                    info->py_color
                );
            } else if (v != frame->v and v_mark == x_mark) {
                Aff2 node_y_trans;
                aff2_identity(node_y_trans);
                aff2_stretch(
                    node_y_trans,
                    (Vec2){ radius, radius },
                    node_y_trans
                );
                aff2_add_vec2(
                    node_y_trans,
                    node_y_trans,
                    slice_get(embedding, v)
                );
                aven_gl_shape_rounded_geometry_push_square(
                    rounded_geometry,
                    node_y_trans,
                    1.0f,
                    info->xp_color
                );
            } else {
                Aff2 node_face_trans;
                aff2_identity(node_face_trans);
                aff2_stretch(
                    node_face_trans,
                    (Vec2){ radius, radius },
                    node_face_trans
                );
                aff2_add_vec2(
                    node_face_trans,
                    node_face_trans,
                    slice_get(embedding, v)
                );
                aven_gl_shape_rounded_geometry_push_square(
                    rounded_geometry,
                    node_face_trans,
                    1.0f,
                    info->cycle_color
                );
            }

            AvenGraphAugAdjListNode vu_node = slice_get(v_aug_adj, v_nb.last);
            uint32_t u = vu_node.vertex;
            AvenGraphPlaneHartmanVertex u_info =
                *aven_graph_plane_hartman_vinfo(ctx, frame, u);
            uint32_t u_mark = slice_get(ctx->marks, u_info.mark);
            AvenGraphAugAdjList u_aug_adj = slice_get(ctx->graph, u);

            if (u_mark == v_mark and u_mark == y_mark) {
                aven_graph_plane_geometry_push_edge(
                    geometry,
                    embedding,
                    u,
                    v,
                    trans,
                    &py_edge_info
                );
            } else if (u_mark == v_mark and u_mark == x_mark) {
                aven_graph_plane_geometry_push_edge(
                    geometry,
                    embedding,
                    u,
                    v,
                    trans,
                    &xp_edge_info
                );
            } else {
                aven_graph_plane_geometry_push_edge(
                    geometry,
                    embedding,
                    u,
                    v,
                    trans,
                    &cycle_edge_info
                );
            }

            if (u_mark == 0) {
                v = u;
                v_nb = u_info.nb;
                v_mark = u_mark;

                v_nb.last = aven_graph_aug_neighbor_index(
                    ctx->graph,
                    v,
                    frame->v
                );
            } else if (
                u != frame->v and
                u_mark == x_mark and
                vu_node.back_index != u_info.nb.first
            ) {
                uint32_t maybe_uv_index = (uint32_t)(
                    (vu_node.back_index + u_aug_adj.len - 1) % u_aug_adj.len
                );

                assert(slice_get(u_aug_adj, maybe_uv_index).vertex == frame->v);
                v = frame->v;
                AvenGraphPlaneHartmanVertex *fv_info =
                    aven_graph_plane_hartman_vinfo(ctx, frame, frame->v);
                v_nb = fv_info->nb;
                v_mark = slice_get(ctx->marks, fv_info->mark);
            } else {
                v = u;
                v_nb = u_info.nb;
                v_mark = u_mark;
            }
            // else if (
            //     v_mark == x_mark and
            //     vu_node.back_index != u_info.nb.last
            // ) {
            //     uint32_t maybe_uv_index = (u_info.nb.last + 1
            //     ) % u_aug_adj.len;
            //     if (slice_get(u_aug_adj, maybe_uv_index).vertex == frame.v) {
            //         u = frame->v;
            //         AvenGraphPlaneHartmanVertex *fv_info =
            //             aven_graph_plane_hartman_vinfo(ctx, frame, u);
            //         v_nb = fv_info->nb;
            //         v_mark = slice_get(ctx->marks, fv_info->mark);
            //     } else {
            //         assert(false);
            //     }
            // }
        } while (v != frame->x);
    }

    // Draw active edge and vertex
    {
        float radius = info->radius + info->border_thickness;
        
        Aff2 node_active_trans;
        aff2_identity(node_active_trans);
        aff2_stretch(
            node_active_trans,
            (Vec2){ radius, radius },
            node_active_trans
        );
        aff2_add_vec2(
            node_active_trans,
            node_active_trans,
            slice_get(embedding, frame->v)
        );
        aven_gl_shape_rounded_geometry_push_square(
            rounded_geometry,
            node_active_trans,
            1.0f,
            info->active_color
        );

        AvenGraphPlaneGeometryEdge active_edge_info = {
            .thickness = info->edge_thickness + info->border_thickness,
        };
        vec4_copy(active_edge_info.color, info->active_color);

        AvenGraphAugAdjList v_adj = slice_get(ctx->graph, frame->v);
        uint32_t u = slice_get(v_adj, v_info.nb.first).vertex;
        aven_graph_plane_geometry_push_edge(
            geometry,
            embedding,
            u,
            frame->v,
            trans,
            &active_edge_info
        );
    }

    // Draw all graph edges
    {
        AvenGraphPlaneGeometryEdge edge_info = {
            .thickness = info->edge_thickness,
        };

        for (uint32_t v = 0; v < ctx->graph.len; v += 1) {
            AvenGraphAugAdjList v_adj = slice_get(ctx->graph, v);
            AvenGraphPlaneHartmanList *v_colors = &slice_get(ctx->color_lists, v);
            for (uint32_t i = 0; i < v_adj.len; i += 1) {
                uint32_t u = slice_get(v_adj, i).vertex;
                AvenGraphPlaneHartmanList *u_colors = &slice_get(
                    ctx->color_lists,
                    u
                );
                if (
                    v_colors->len == 1 and
                    u_colors->len == 1 and
                    u_colors->data[0] == v_colors->data[0]
                ) {
                    vec4_copy(
                        edge_info.color,
                        slice_get(info->colors, u_colors->data[0])
                    );
                } else {
                    vec4_copy(edge_info.color, info->outline_color);
                }
                aven_graph_plane_geometry_push_edge(
                    geometry,
                    embedding,
                    u,
                    v,
                    trans,
                    &edge_info
                );
            }
        }
    }

    // Draw vertex color lists

    for (uint32_t v = 0; v < ctx->graph.len; v += 1) {
        Vec2 v_pos;
        vec2_copy(v_pos, slice_get(embedding, v));
        aff2_transform(v_pos, trans, v_pos);

        Aff2 node_outline_trans;
        aff2_identity(node_outline_trans);
        aff2_stretch(
            node_outline_trans,
            (Vec2){ info->radius, info->radius },
            node_outline_trans
        );
        aff2_add_vec2(node_outline_trans, node_outline_trans, v_pos);
        aven_gl_shape_rounded_geometry_push_square(
            rounded_geometry,
            node_outline_trans,
            1.0f,
            info->outline_color
        );

        AvenGraphPlaneHartmanList v_list = slice_get(ctx->color_lists, v);
        switch (v_list.len) {
            case 1: {
                Aff2 node_trans;
                aff2_identity(node_trans);
                aff2_stretch(
                    node_trans,
                    (Vec2){
                        info->radius - info->border_thickness,
                        info->radius - info->border_thickness,
                    },
                    node_trans
                );
                aff2_add_vec2(node_trans, node_trans, v_pos);
                aven_gl_shape_rounded_geometry_push_square(
                    rounded_geometry,
                    node_trans,
                    1.0f,
                    slice_get(info->colors, v_list.data[0])
                );
                break;
            }
            case 2: {
                Aff2 node1_trans;
                aff2_identity(node1_trans);
                aff2_stretch(
                    node1_trans,
                    (Vec2){
                        info->radius - info->border_thickness,
                        (info->radius - info->border_thickness)  / 2.0f,
                    },
                    node1_trans
                );
                aff2_add_vec2(
                    node1_trans,
                    node1_trans,
                    (Vec2){
                        0.0f,
                        (info->radius - info->border_thickness) / 2.0f,
                    }
                );

                Aff2 node2_trans;
                aff2_rotate(node2_trans, node1_trans, AVEN_MATH_PI_F);

                aff2_add_vec2(node1_trans, node1_trans, v_pos);
                aff2_add_vec2(node2_trans, node2_trans, v_pos);
                
                aven_gl_shape_rounded_geometry_push_square_half(
                    rounded_geometry,
                    node1_trans,
                    1.0f,
                    slice_get(info->colors, v_list.data[0])
                );
                aven_gl_shape_rounded_geometry_push_square_half(
                    rounded_geometry,
                    node2_trans,
                    1.0f,
                    slice_get(info->colors, v_list.data[1])
                );
                break;
            }
            case 3: {
                Aff2 node_trans;
                aff2_identity(node_trans);
                aff2_stretch(
                    node_trans,
                    (Vec2){
                        info->radius - info->border_thickness,
                        info->radius - info->border_thickness,
                    },
                    node_trans
                );
                aff2_add_vec2(node_trans, node_trans, v_pos);

                for (size_t i = 0; i < 3; i += 1) {
                    aven_gl_shape_rounded_geometry_push_sector(
                        rounded_geometry,
                        node_trans,
                        (3.0f * AVEN_MATH_PI_F / 2.0f) +
                            (float)i * (2.0f * AVEN_MATH_PI_F / 3.0f),
                        (3.0f * AVEN_MATH_PI_F / 2.0f) +
                            ((float)i + 1.0f) * (2.0f * AVEN_MATH_PI_F / 3.0f),
                        slice_get(info->colors, v_list.data[i])
                    );
                }
                break;
            }
            default: {
                assert(false);
                break;
            }
        }
    }
}

#endif // AVEN_GRAPH_PLANE_HARTMAN_GEOMETRY_H
