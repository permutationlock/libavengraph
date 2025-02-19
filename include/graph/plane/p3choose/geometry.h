#ifndef GRAPH_PLANE_P3CHOOSE_GEOMETRY_H
#define GRAPH_PLANE_P3CHOOSE_GEOMETRY_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/math.h>

#include <aven/gl/shape.h>

#include "../p3choose.h"
#include "../geometry.h"

typedef struct {
    Vec4 active_color;
    Vec4 cycle_color;
    Vec4 py_color;
    Vec4 xp_color;    
} GraphPlaneP3ChooseGeometryFrame;

typedef struct {
    Slice(Vec4) colors;
    GraphPlaneP3ChooseGeometryFrame active_frame;
    GraphPlaneP3ChooseGeometryFrame inactive_frame;
    Vec4 outline_color;
    Vec4 uncolored_edge_color;
    Vec4 edge_color;
    float edge_thickness;
    float radius;
    float border_thickness;
} GraphPlaneP3ChooseGeometryInfo;

static inline void graph_plane_p3choose_geometry_push_frame_active(
    AvenGlShapeGeometry *geometry,
    AvenGlShapeRoundedGeometry *rounded_geometry,
    GraphPlaneEmbedding embedding,
    GraphPlaneP3ChooseCtx *ctx,
    GraphPlaneP3ChooseFrame *frame,
    Aff2 trans,
    float radius,
    float border_thickness,
    float edge_thickness,
    Vec4 edge_color,
    GraphPlaneP3ChooseGeometryFrame *info
) {
    GraphPlaneP3ChooseVertexLoc z_loc = *graph_plane_p3choose_vloc(
        ctx,
        frame,
        frame->z
    );

    Vec2 z_pos;
    vec2_copy(z_pos, get(embedding, frame->z));
    aff2_transform(z_pos, trans, z_pos);

    Aff2 node_active_trans;
    aff2_identity(node_active_trans);
    aff2_stretch(
        node_active_trans,
        (Vec2){ radius + border_thickness, radius + border_thickness },
        node_active_trans
    );
    aff2_add_vec2(
        node_active_trans,
        node_active_trans,
        z_pos
    );
    aven_gl_shape_rounded_geometry_push_square(
        rounded_geometry,
        node_active_trans,
        1.0f,
        info->active_color
    );

    GraphPlaneGeometryEdge active_edge_info = {
        .thickness = edge_thickness + border_thickness,
    };
    vec4_copy(active_edge_info.color, info->active_color);

    GraphPlaneGeometryEdge simple_edge_info = {
        .thickness = edge_thickness,
    };
    vec4_copy(simple_edge_info.color, edge_color);

    GraphAugAdjList z_adj = get(ctx->vertex_info, frame->z).adj;

    if (z_loc.nb.last != z_loc.nb.first) {
        uint32_t v = get(
            z_adj,
            graph_aug_adj_next(z_adj, z_loc.nb.first)
        ).vertex;
        graph_plane_geometry_push_edge(
            geometry,
            embedding,
            v,
            frame->z,
            trans,
            &active_edge_info
        );
        graph_plane_geometry_push_edge(
            geometry,
            embedding,
            v,
            frame->z,
            trans,
            &simple_edge_info
        );
    } else {
        uint32_t u = get(z_adj, z_loc.nb.first).vertex;
        graph_plane_geometry_push_edge(
            geometry,
            embedding,
            u,
            frame->z,
            trans,
            &active_edge_info
        );
        graph_plane_geometry_push_edge(
            geometry,
            embedding,
            u,
            frame->z,
            trans,
            &simple_edge_info
        );
    }
}

static inline void graph_plane_p3choose_geometry_push_frame_outline(
    AvenGlShapeGeometry *geometry,
    AvenGlShapeRoundedGeometry *rounded_geometry,
    GraphPlaneEmbedding embedding,
    GraphPlaneP3ChooseCtx *ctx,
    GraphPlaneP3ChooseFrame *frame,
    Aff2 trans,
    float radius,
    float edge_thickness,
    float border_thickness,
    Vec4 outline_color,
    GraphPlaneP3ChooseGeometryFrame *info
) {
    uint32_t x_mark = get(ctx->marks, frame->x_loc.mark);
    uint32_t y_mark = get(ctx->marks, frame->y_loc.mark);

    // Draw edges of outer cycle including active edge
    {
        GraphPlaneGeometryEdge xp_edge_info = {
            .thickness = edge_thickness + border_thickness,
        };
        vec4_copy(xp_edge_info.color, info->xp_color);
        GraphPlaneGeometryEdge py_edge_info = {
            .thickness = edge_thickness + border_thickness,
        };
        vec4_copy(py_edge_info.color, info->py_color);
        GraphPlaneGeometryEdge cycle_edge_info = {
            .thickness = edge_thickness + border_thickness,
        };
        vec4_copy(cycle_edge_info.color, info->cycle_color);

        uint32_t v = frame->x;
        GraphPlaneP3ChooseNeighbors v_nb = frame->x_loc.nb;
        uint32_t v_mark = get(ctx->marks, frame->x_loc.mark);
        do {
            GraphAugAdjList v_aug_adj = get(ctx->vertex_info, v).adj;
            Vec2 v_pos;
            vec2_copy(v_pos, get(embedding, v));
            aff2_transform(v_pos, trans, v_pos);

            if (v == frame->z) {
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
                    v_pos
                );
                aven_gl_shape_rounded_geometry_push_square(
                    rounded_geometry,
                    node_x_trans,
                    1.0f,
                    info->active_color
                );
            } else if (v_mark == y_mark) {
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
                    v_pos
                );
                aven_gl_shape_rounded_geometry_push_square(
                    rounded_geometry,
                    node_x_trans,
                    1.0f,
                    info->py_color
                );
            } else if (v_mark == x_mark) {
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
                    v_pos
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
                    v_pos
                );
                aven_gl_shape_rounded_geometry_push_square(
                    rounded_geometry,
                    node_face_trans,
                    1.0f,
                    info->cycle_color
                );
            }

            GraphAugAdjListNode vu_node = get(v_aug_adj, v_nb.last);
            uint32_t u = vu_node.vertex;
            GraphPlaneP3ChooseVertexLoc u_loc =
                *graph_plane_p3choose_vloc(ctx, frame, u);
            uint32_t u_mark = get(ctx->marks, u_loc.mark);

            assert(u_mark != 0);

            if (
                v == frame->z and
                graph_aug_adj_next(v_aug_adj, v_nb.first) == v_nb.last
            ) {
                // active edge, outline already drawn
            } else if (u_mark == v_mark and u_mark == y_mark) {
                graph_plane_geometry_push_edge(
                    geometry,
                    embedding,
                    u,
                    v,
                    trans,
                    &py_edge_info
                );
            } else if (u_mark == v_mark and u_mark == x_mark) {
                graph_plane_geometry_push_edge(
                    geometry,
                    embedding,
                    u,
                    v,
                    trans,
                    &xp_edge_info
                );
            } else {
                graph_plane_geometry_push_edge(
                    geometry,
                    embedding,
                    u,
                    v,
                    trans,
                    &cycle_edge_info
                );
            }

            v = u;
            v_nb = u_loc.nb;
            v_mark = u_mark;
        } while (v != frame->x);

        // {
        //     GraphPlaneGeometryEdge active_edge_info = {
        //         .thickness = edge_thickness + border_thickness,
        //     };
        //     vec4_copy(active_edge_info.color, info->active_color);
        //     GraphPlaneP3ChooseVertexLoc *z_loc = graph_plane_p3choose_vloc(
        //         ctx,
        //         frame,
        //         frame->z
        //     );
        //     GraphAugAdjList z_adj = get(ctx->vertex_info, frame->z).adj;
        //     uint32_t u = get(z_adj, z_loc->nb.first).vertex;
        //     if (z_loc->nb.first != z_loc->nb.last) {
        //         u = get(
        //             z_adj,
        //             graph_aug_adj_next(z_adj, z_loc->nb.first)
        //         ).vertex;
        //     }

        //     graph_plane_geometry_push_edge(
        //         geometry,
        //         embedding,
        //         frame->z,
        //         u,
        //         trans,
        //         &active_edge_info
        //     );
        // }

        GraphPlaneGeometryEdge edge_info = {
            .thickness = edge_thickness,
        };
        vec4_copy(edge_info.color, outline_color);

        v = frame->x;
        do {
            GraphPlaneP3ChooseVertexLoc *v_loc = graph_plane_p3choose_vloc(
                ctx,
                frame,
                v
            );
            GraphAugAdjList v_adj = get(ctx->vertex_info, v).adj;
            for (
                uint32_t i = v_loc->nb.first;
                i != v_loc->nb.last;
                i = graph_aug_adj_next(v_adj, i)
            ) {
                uint32_t u = get(v_adj, i).vertex;
                // if (u < v) {
                //     continue;
                // }
                graph_plane_geometry_push_edge(
                    geometry,
                    embedding,
                    u,
                    v,
                    trans,
                    &edge_info
                );
            }
            if (v_loc->nb.first == v_loc->nb.last) {
                uint32_t u = get(v_adj, v_loc->nb.last).vertex;
                // if (u >= v) {
                graph_plane_geometry_push_edge(
                    geometry,
                    embedding,
                    u,
                    v,
                    trans,
                    &edge_info
                );
                // }
            }
            v = get(v_adj, v_loc->nb.last).vertex;
        } while (v != frame->x);
    }
}

typedef Slice(GraphPlaneP3ChooseFrame) GraphPlaneP3ChooseFrameSlice;

static inline void graph_plane_p3choose_geometry_push_ctx(
    AvenGlShapeGeometry *geometry,
    AvenGlShapeRoundedGeometry *rounded_geometry,
    GraphPlaneEmbedding embedding,
    GraphPlaneP3ChooseCtx *ctx,
    GraphPlaneP3ChooseFrameOptionalSlice active_frames,
    Aff2 trans,
    GraphPlaneP3ChooseGeometryInfo *info
) {
    // Draw all graph edges
    {
        GraphPlaneGeometryEdge edge_info = {
            .thickness = info->edge_thickness,
        };

        vec4_copy(edge_info.color, info->uncolored_edge_color);
        for (uint32_t v = 0; v < ctx->vertex_info.len; v += 1) {
            GraphAugAdjList v_adj = get(ctx->vertex_info, v).adj;

            for (uint32_t i = 0; i < v_adj.len; i += 1) {
                uint32_t u = get(v_adj, i).vertex;
                if (u < v) {
                    continue;
                }
                graph_plane_geometry_push_edge(
                    geometry,
                    embedding,
                    get(v_adj, i).vertex,
                    v,
                    trans,
                    &edge_info
                );
            }
        } 

        for (size_t i = 0; i < ctx->frames.len; i += 1) {
            graph_plane_p3choose_geometry_push_frame_outline(
                geometry,
                rounded_geometry,
                embedding,
                ctx,
                &list_get(ctx->frames, i),
                trans,
                info->radius + info->border_thickness,
                info->edge_thickness,
                info->border_thickness,
                info->outline_color,
                &info->inactive_frame
            );
        }

        for (size_t i = 0; i < ctx->frames.len; i += 1) {
            graph_plane_p3choose_geometry_push_frame_active(
                geometry,
                rounded_geometry,
                embedding,
                ctx,
                &list_get(ctx->frames, i),
                trans,
                info->radius,
                info->border_thickness,
                info->edge_thickness,
                info->edge_color,
                &info->inactive_frame
            );
        }

        for (size_t i = 0; i < active_frames.len; i += 1) {
            if (get(active_frames, i).valid) {
                graph_plane_p3choose_geometry_push_frame_outline(
                    geometry,
                    rounded_geometry,
                    embedding,
                    ctx,
                    &get(active_frames, i).value,
                    trans,
                    info->radius + info->border_thickness,
                    info->edge_thickness,
                    info->border_thickness,
                    info->outline_color,
                    &info->active_frame
                );
            }
        }

        for (size_t i = 0; i < active_frames.len; i += 1) {
            if (get(active_frames, i).valid) {
                graph_plane_p3choose_geometry_push_frame_active(
                    geometry,
                    rounded_geometry,
                    embedding,
                    ctx,
                    &get(active_frames, i).value,
                    trans,
                    info->radius,
                    info->border_thickness,
                    info->edge_thickness,
                    info->edge_color,
                    &info->active_frame
                );
            }
        }

        vec4_copy(edge_info.color, info->edge_color);
        for (uint32_t v = 0; v < ctx->vertex_info.len; v += 1) {
            GraphAugAdjList v_adj = get(ctx->vertex_info, v).adj;

            for (uint32_t i = 0; i < v_adj.len; i += 1) {
                uint32_t u = get(v_adj, i).vertex;
                if (u < v) {
                    continue;
                }

                if (
                    get(ctx->vertex_info, v).loc.mark != 0 or
                    get(ctx->vertex_info, u).loc.mark != 0
                ) {
                    continue;
                }

                graph_plane_geometry_push_edge(
                    geometry,
                    embedding,
                    get(v_adj, i).vertex,
                    v,
                    trans,
                    &edge_info
                );
            }
        }

        for (uint32_t v = 0; v < ctx->vertex_info.len; v += 1) {
            GraphAugAdjList v_adj = get(ctx->vertex_info, v).adj;
            GraphPlaneP3ChooseList *v_colors = &get(ctx->vertex_info, v).colors;
            for (uint32_t i = 0; i < v_adj.len; i += 1) {
                uint32_t u = get(v_adj, i).vertex;
                if (u < v) {
                    continue;
                }
                GraphPlaneP3ChooseList *u_colors = &get(
                    ctx->vertex_info,
                    u
                ).colors;
                if (
                    v_colors->len == 1 and
                    u_colors->len == 1 and
                    get(*u_colors, 0) == get(*v_colors, 0)
                ) {
                    vec4_copy(
                        edge_info.color,
                        get(info->colors, get(*u_colors, 0))
                    );
                    graph_plane_geometry_push_edge(
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
    }

    // Draw vertex color lists

    for (uint32_t v = 0; v < ctx->vertex_info.len; v += 1) {
        Vec2 v_pos;
        vec2_copy(v_pos, get(embedding, v));
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

        GraphPlaneP3ChooseList v_list = get(ctx->vertex_info, v).colors;
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
                    get(info->colors, get(v_list, 0))
                );
                break;
            }
            case 2: {
                Aff2 node1_trans;
                aff2_identity(node1_trans);
                aff2_add_vec2(
                    node1_trans,
                    node1_trans,
                    (Vec2){ 0.0f, -0.01f }
                );
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
                    get(info->colors, get(v_list, 0))
                );
                aven_gl_shape_rounded_geometry_push_square_half(
                    rounded_geometry,
                    node2_trans,
                    1.0f,
                    get(info->colors, get(v_list, 1))
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
                        0.01f + (3.0f * AVEN_MATH_PI_F / 2.0f) +
                            ((float)i + 1.0f) * (2.0f * AVEN_MATH_PI_F / 3.0f),
                        get(info->colors, get(v_list, i))
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

// typedef struct {
//     GraphPlaneP3ChooseCtx alg_ctx;
//     GraphPlaneEmbedding embedding;
//     GraphPropUint8 visited_marks;
//     int64_t timestep;
//     AvenTimeInst last_update;
// } GraphPlaneP3ChooseGeometryCtx;

// static inline void graph_plane_p3choose_geometry_init(
//     Graph graph,
//     GraphPlaneEmbedding embedding,
//     GraphPlaneP3ChooseList list_colors,
//     GraphSubset cwise_outer_face,
//     int64_t timestep,
//     AvenTimeInst now,
//     AvenArena *arena
// ) {
//     GraphPlaneP3ChooseGeometryCtx ctx = {
//         .embedding = embedding,
//         .visited_marks = { .len = embedding.len },
//         .timestep = timestep,
//         .last_update = now,
//     };

//     ctx.alg_ctx = graph_plane_p3choose_init(
//         list_colors,
//         graph_aug(graph, arena),
//         cwise_outer_face,
//         arena
//     );

//     return ctx;
// }

// static inline void graph_plane_p3choose_geometry_push_active_frame(
//     AvenGlShapeGeometry *geometry,
//     AvenGlShapeRoundedGeometry *rounded_geometry,
//     GraphPlaneP3ChooseGeometryCtx *ctx,
//     Aff2 trans,
//     AvenTimeInst now
// ) {
    
// }

#endif // GRAPH_PLANE_P3CHOOSE_GEOMETRY_H
