#ifndef AVEN_GRAPH_PLANE_HARTMAN_GEOMETRY_H
#define AVEN_GRAPH_PLANE_HARTMAN_GEOMETRY_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/math.h>

#include <aven/gl/shape.h>

#include "../hartman.h"
#include "../geometry.h"

typedef struct {
    Slice(Vec4) list_colors;
    Vec4 outline_color;
    Vec4 active_color;
    Vec4 cycle_edge_color;
    Vec4 py_vertex_color;
    Vec4 xp_vertex_color;
    Vec4 yx_vertex_color;
    float radius;
    float border_thickness;
} AvenGraphPlaneHartmanGeometryInfo;

static inline void aven_graph_plane_hartman_geometry_push_ctx(
    AvenGlShapeRoundedGeometry *geometry,
    AvenGraphPlaneEmbedding embedding,
    AvenGraphPlaneHartmanCtx *ctx,
    Aff2 trans,
    AvenGraphPlaneHartmanGeometryInfo *info
) {
    // Draw all graph edges
    {
        AvenGraphPlaneGeometryEdge edge_info = {
            .thickness = info->radius / 4.0f,
        };
        vec4_copy(edge_info.color, info->outline_color);

        for (uint32_t v = 0; v < ctx->graph.len; v += 1) {
            AvenGraphAugAdjList v_adj = slice_get(ctx->graph, v);
            for (uint32_t i = 0; i < v_adj.len; i += 1) {
                uint32_t u = slice_get(v_adj, i).vertex;
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

    AvenGraphPlaneHartmanFrame *frame = &list_get(
        ctx->frames,
        ctx->frames.len - 1
    );

    // Draw edges of outer cycle including active edge
    {
        AvenGraphPlaneGeometryEdge cycle_edge_info = {
            .thickness = info->radius / 3.0f,
        };
        vec4_copy(cycle_edge_info.color, info->cycle_edge_color);

        AvenGraphPlaneGeometryEdge active_edge_info = {
            .thickness = info->radius / 2.0f,
        };
        vec4_copy(active_edge_info.color, info->active_edge_color);

        uint32_t v = frame->x;
        do {
            AvenGraphAugAdjList v_aug_adj = slice_get(ctx->graph, v);
            AvenGraphPlaneHartmanVertex v_info = aven_graph_plane_harman_vinfo(
                ctx,
                frame,
                v
            );

            uint32_t u = slice_get(v_aug_adj, v_info.last).vertex;
            if (u == frame->v) {
                aven_graph_plane_geometry_push_edge(
                    geometry,
                    embedding,
                    u,
                    v,
                    trans,
                    &active_edge_info
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
            v = u;
        } while (v != frame->x);
    }

    // Draw special vertex outlines: x, y, and the active vertex v
    {
        float active_radius = info->radius + info->border_thickness;
        if (frame->v == frame->x) {
            active_radius += info->border_thickness;
        }
        if (frame->v == frame->y) {
            active_radius += info->border_thickness;
        }
        
        Aff2 node_active_trans;
        aff2_identity(node_active_trans);
        aff2_stretch(
            node_active_trans,
            (Vec2){ active_radius, active_radius },
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

        float y_radius = info->radius + info->border_thickness;
        if (frame->y == frame->x) {
            y_radius += info->border_thickness;
        }

        Aff2 node_y_trans;
        aff2_identity(node_y_trans);
        aff2_stretch(
            node_y_trans,
            (Vec2){ y_radius, y_radius },
            node_y_trans
        );
        aff2_add_vec2(
            node_y_trans,
            node_y_trans,
            slice_get(embedding, frame->y)
        );
        aven_gl_shape_rounded_geometry_push_square(
            rounded_geometry,
            node_y_trans,
            1.0f,
            info->y_color
        );

        float x_radius = info->radius + info->border_thickness;

        Aff2 node_x_trans;
        aff2_identity(node_x_trans);
        aff2_stretch(
            node_x_trans,
            (Vec2){ x_radius, x_radius },
            node_x_trans
        );
        aff2_add_vec2(
            node_x_trans,
            node_x_trans,
            slice_get(embedding, frame->x)
        );
        aven_gl_shape_rounded_geometry_push_square(
            rounded_geometry,
            node_x_trans,
            1.0f,
            info->x_color
        );
    }

    // Draw vertex color lists

    for (uint32_t v = 0; v < ctx->graph.len; v += 1) {
        Vec2 v_pos = slice_get(embedding, v);
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
            case 1:
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
            case 2:
                Aff2 node1_trans;
                aff2_identity(node1_trans);
                aff2_stretch(
                    node1_trans,
                    (Vec2){
                        (info->radius - info->border_thickness) / 2.0f,
                        info->radius - info->border_thickness,
                    },
                    node1_trans
                );
                aff2_add_vec2(
                    node1_trans,
                    node1_trans,
                    (Vec2){
                        -1.0f * (info->radius - info->border_thickness) / 4.0f,
                        0.0f
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
            case 3:
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
            default:
                assert(false);
                break;
        }
    }
}

#endif // AVEN_GRAPH_PLANE_HARTMAN_GEOMETRY_H
