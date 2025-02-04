#ifndef GRAPH_PLANE_HARTMAN_GEOMETRY_H
#define GRAPH_PLANE_HARTMAN_GEOMETRY_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/math.h>

#include <aven/gl/shape.h>

#include "../hartman.h"
#include "../geometry.h"

typedef struct {
    Vec4 active_color;
    Vec4 cycle_color;
    Vec4 py_color;
    Vec4 xp_color;    
} GraphPlaneHartmanGeometryFrame;

typedef struct {
    Slice(Vec4) colors;
    GraphPlaneHartmanGeometryFrame active_frame;
    GraphPlaneHartmanGeometryFrame inactive_frame;
    Vec4 outline_color;
    Vec4 uncolored_edge_color;
    Vec4 edge_color;
    float edge_thickness;
    float radius;
    float border_thickness;
} GraphPlaneHartmanGeometryInfo;

static inline void graph_plane_hartman_geometry_push_frame_active(
    AvenGlShapeGeometry *geometry,
    AvenGlShapeRoundedGeometry *rounded_geometry,
    GraphPlaneEmbedding embedding,
    GraphPlaneHartmanCtx *ctx,
    GraphPlaneHartmanFrame *frame,
    Aff2 trans,
    float radius,
    float edge_thickness,
    GraphPlaneHartmanGeometryFrame *info
) {
    GraphPlaneHartmanVertex z_info = *graph_plane_hartman_vinfo(
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
        (Vec2){ radius, radius },
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
        .thickness = edge_thickness,
    };
    vec4_copy(active_edge_info.color, info->active_color);

    GraphAugAdjList z_adj = get(ctx->graph, frame->z);

    if (z_info.nb.last != z_info.nb.first) {
        uint32_t v = get(
            z_adj,
            graph_aug_adj_next(z_adj, z_info.nb.first)
        ).vertex;
        graph_plane_geometry_push_edge(
            geometry,
            embedding,
            v,
            frame->z,
            trans,
            &active_edge_info
        );
    } else {
        uint32_t u = get(z_adj, z_info.nb.first).vertex;
        graph_plane_geometry_push_edge(
            geometry,
            embedding,
            u,
            frame->z,
            trans,
            &active_edge_info
        );
    }
}

static inline void graph_plane_hartman_geometry_push_frame_outline(
    AvenGlShapeGeometry *geometry,
    AvenGlShapeRoundedGeometry *rounded_geometry,
    GraphPlaneEmbedding embedding,
    GraphPlaneHartmanCtx *ctx,
    GraphPlaneHartmanFrame *frame,
    Aff2 trans,
    float radius,
    float edge_thickness,
    float border_thickness,
    Vec4 outline_color,
    GraphPlaneHartmanGeometryFrame *info
) {
    uint32_t x_mark = get(ctx->marks, frame->x_info.mark);
    uint32_t y_mark = get(ctx->marks, frame->y_info.mark);

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
        GraphPlaneHartmanNeighbors v_nb = frame->x_info.nb;
        uint32_t v_mark = get(ctx->marks, frame->x_info.mark);
        do {
            GraphAugAdjList v_aug_adj = get(ctx->graph, v);
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
            GraphPlaneHartmanVertex u_info =
                *graph_plane_hartman_vinfo(ctx, frame, u);
            uint32_t u_mark = get(ctx->marks, u_info.mark);
            // GraphAugAdjList u_aug_adj = get(ctx->graph, u);

            assert(u_mark != 0);

            if (u_mark == v_mark and u_mark == y_mark) {
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
            v_nb = u_info.nb;
            v_mark = u_mark;
        } while (v != frame->x);

        {
            GraphPlaneGeometryEdge active_edge_info = {
                .thickness = edge_thickness + border_thickness,
            };
            vec4_copy(active_edge_info.color, info->active_color);
            GraphPlaneHartmanVertex *z_info = graph_plane_hartman_vinfo(
                ctx,
                frame,
                frame->z
            );
            GraphAugAdjList z_adj = get(ctx->graph, frame->z);
            uint32_t u = get(z_adj, z_info->nb.first).vertex;
            if (z_info->nb.first != z_info->nb.last) {
                u = get(
                    z_adj,
                    graph_aug_adj_next(z_adj, z_info->nb.first)
                ).vertex;
            }

            graph_plane_geometry_push_edge(
                geometry,
                embedding,
                frame->z,
                u,
                trans,
                &active_edge_info
            );
        }

        GraphPlaneGeometryEdge edge_info = {
            .thickness = edge_thickness,
        };
        vec4_copy(edge_info.color, outline_color);

        v = frame->x;
        do {
            GraphPlaneHartmanVertex *v_info = graph_plane_hartman_vinfo(
                ctx,
                frame,
                v
            );
            GraphAugAdjList v_adj = get(ctx->graph, v);
            for (
                uint32_t i = v_info->nb.first;
                i != v_info->nb.last;
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
            if (v_info->nb.first == v_info->nb.last) {
                uint32_t u = get(v_adj, v_info->nb.last).vertex;
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
            v = get(v_adj, v_info->nb.last).vertex;
        } while (v != frame->x);
    }
}

typedef Slice(GraphPlaneHartmanFrame) GraphPlaneHartmanFrameSlice;

static inline void graph_plane_hartman_geometry_push_ctx(
    AvenGlShapeGeometry *geometry,
    AvenGlShapeRoundedGeometry *rounded_geometry,
    GraphPlaneEmbedding embedding,
    GraphPlaneHartmanCtx *ctx,
    GraphPlaneHartmanFrameSlice active_frames,
    Aff2 trans,
    GraphPlaneHartmanGeometryInfo *info
) {
    // Draw all graph edges
    {
        GraphPlaneGeometryEdge edge_info = {
            .thickness = info->edge_thickness,
        };

        vec4_copy(edge_info.color, info->uncolored_edge_color);
        for (uint32_t v = 0; v < ctx->graph.len; v += 1) {
            GraphAugAdjList v_adj = get(ctx->graph, v);

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
            graph_plane_hartman_geometry_push_frame_active(
                geometry,
                rounded_geometry,
                embedding,
                ctx,
                &list_get(ctx->frames, i),
                trans,
                info->radius + info->border_thickness,
                info->edge_thickness,
                &info->inactive_frame
            );
        }

        for (size_t i = 0; i < ctx->frames.len; i += 1) {
            graph_plane_hartman_geometry_push_frame_outline(
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

        for (size_t i = 0; i < active_frames.len; i += 1) {
            graph_plane_hartman_geometry_push_frame_active(
                geometry,
                rounded_geometry,
                embedding,
                ctx,
                &get(active_frames, i),
                trans,
                info->radius + info->border_thickness,
                info->edge_thickness + info->border_thickness,
                &info->active_frame
            );
        }

        for (size_t i = 0; i < active_frames.len; i += 1) {
            graph_plane_hartman_geometry_push_frame_outline(
                geometry,
                rounded_geometry,
                embedding,
                ctx,
                &get(active_frames, i),
                trans,
                info->radius + info->border_thickness,
                info->edge_thickness,
                info->border_thickness,
                info->outline_color,
                &info->active_frame
            );
        }

        vec4_copy(edge_info.color, info->edge_color);
        for (uint32_t v = 0; v < ctx->graph.len; v += 1) {
            GraphAugAdjList v_adj = get(ctx->graph, v);

            for (uint32_t i = 0; i < v_adj.len; i += 1) {
                uint32_t u = get(v_adj, i).vertex;
                if (u < v) {
                    continue;
                }

                if (
                    get(ctx->vertex_info, v).mark != 0 or
                    get(ctx->vertex_info, u).mark != 0
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

        for (uint32_t v = 0; v < ctx->graph.len; v += 1) {
            GraphAugAdjList v_adj = get(ctx->graph, v);
            GraphPlaneHartmanList *v_colors = &get(ctx->color_lists, v);
            for (uint32_t i = 0; i < v_adj.len; i += 1) {
                uint32_t u = get(v_adj, i).vertex;
                if (u < v) {
                    continue;
                }
                GraphPlaneHartmanList *u_colors = &get(
                    ctx->color_lists,
                    u
                );
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

    for (uint32_t v = 0; v < ctx->graph.len; v += 1) {
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

        GraphPlaneHartmanList v_list = get(ctx->color_lists, v);
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
                        (3.0f * AVEN_MATH_PI_F / 2.0f) +
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
//     GraphPlaneHartmanCtx alg_ctx;
//     GraphPlaneEmbedding embedding;
//     GraphPropUint8 visited_marks;
//     int64_t timestep;
//     AvenTimeInst last_update;
// } GraphPlaneHartmanGeometryCtx;

// static inline void graph_plane_hartman_geometry_init(
//     Graph graph,
//     GraphPlaneEmbedding embedding,
//     GraphPlaneHartmanList list_colors,
//     GraphSubset cwise_outer_face,
//     int64_t timestep,
//     AvenTimeInst now,
//     AvenArena *arena
// ) {
//     GraphPlaneHartmanGeometryCtx ctx = {
//         .embedding = embedding,
//         .visited_marks = { .len = embedding.len },
//         .timestep = timestep,
//         .last_update = now,
//     };

//     ctx.alg_ctx = graph_plane_hartman_init(
//         list_colors,
//         graph_aug(graph, arena),
//         cwise_outer_face,
//         arena
//     );

//     return ctx;
// }

// static inline void graph_plane_hartman_geometry_push_active_frame(
//     AvenGlShapeGeometry *geometry,
//     AvenGlShapeRoundedGeometry *rounded_geometry,
//     GraphPlaneHartmanGeometryCtx *ctx,
//     Aff2 trans,
//     AvenTimeInst now
// ) {
    
// }

#endif // GRAPH_PLANE_HARTMAN_GEOMETRY_H
