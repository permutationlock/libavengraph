#ifndef AVEN_GRAPH_PLANE_POH_GEOMETRY_H
#define AVEN_GRAPH_PLANE_POH_GEOMETRY_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/math.h>

#include <aven/gl/shape.h>

#include "../poh.h"
#include "../geometry.h"

typedef struct {
    Vec4 colors[4];
    Vec4 outline_color;
    Vec4 active_color;
    Vec4 face_color;
    Vec4 below_color;
    float edge_thickness;
    float border_thickness;
    float radius;
} AvenGraphPlanePohGeometryInfo;

static inline void aven_graph_plane_poh_geometry_push_ctx(
    AvenGlShapeGeometry *geometry,
    AvenGlShapeRoundedGeometry *rounded_geometry,
    AvenGraphPlaneEmbedding embedding,
    AvenGraphPlanePohCtx *ctx,
    AvenGraphPlanePohFrameOptional *maybe_frame,
    Aff2 trans,
    AvenGraphPlanePohGeometryInfo *info
) {
    if (maybe_frame->valid) {
        AvenGraphPlanePohFrame frame = maybe_frame->value;
    
        AvenGraphAdjList u_adj = get(ctx->graph, frame.u);
        if (frame.edge_index < u_adj.len) {
            AvenGraphPlaneGeometryEdge active_edge_info = {
                .thickness = info->edge_thickness + info->border_thickness,
            };
            vec4_copy(active_edge_info.color, info->active_color);

            uint32_t uv_index = (frame.u_nb_first + frame.edge_index);
            if (uv_index >= u_adj.len) {
                uv_index -= (uint32_t)u_adj.len;
            }

            uint32_t v = get(u_adj, uv_index);

            if (get(ctx->marks, v) == frame.face_mark) {
                active_edge_info.thickness += info->border_thickness;
            }

            AvenGraphPlaneGeometryNode active_node_info = {
                .mat = {
                    { info->radius + 2.0f * info->border_thickness, 0.0f },
                    { 0.0f, info->radius + 2.0f * info->border_thickness }
                },
                .shape = AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
                .roundness = 1.0f,
            };
            vec4_copy(active_node_info.color, info->active_color);

            aven_graph_plane_geometry_push_edge(
                geometry,
                embedding,
                frame.u,
                v,
                trans,
                &active_edge_info
            );
            aven_graph_plane_geometry_push_vertex(
                rounded_geometry,
                embedding,
                frame.u,
                trans,
                &active_node_info
            );
        }

        AvenGraphPlaneGeometryEdge face_edge_info = {
            .thickness = info->edge_thickness + info->border_thickness,
        };
        vec4_copy(face_edge_info.color, info->face_color);

        AvenGraphPlaneGeometryNode face_node_info = {
            .mat = {
                { info->radius + info->border_thickness, 0.0f },
                { 0.0f, info->radius + info->border_thickness }
            },
            .shape = AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
            .roundness = 1.0f,
        };
        vec4_copy(face_node_info.color, info->face_color);

        AvenGraphPlaneGeometryNode below_node_info = {
            .mat = {
                { info->radius + info->border_thickness, 0.0f },
                { 0.0f, info->radius + info->border_thickness }
            },
            .shape = AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
            .roundness = 1.0f,
        };
        vec4_copy(below_node_info.color, info->below_color);

        AvenGraphPlaneGeometryEdge below_edge_info = {
            .thickness = info->edge_thickness + info->border_thickness,
        };
        vec4_copy(below_edge_info.color, info->below_color);

        for (uint32_t v = 0; v < ctx->graph.len; v += 1) {
            int32_t v_mark = get(ctx->marks, v);
            if (v_mark == frame.face_mark) {
                aven_graph_plane_geometry_push_vertex(
                    rounded_geometry,
                    embedding,
                    v,
                    trans,
                    &face_node_info
                );
            } else if (v_mark == (frame.face_mark + 1)) {
                aven_graph_plane_geometry_push_vertex(
                    rounded_geometry,
                    embedding,
                    v,
                    trans,
                    &below_node_info
                );
            } else {
                continue;
            }

            AvenGraphAdjList v_adj = get(ctx->graph, v);
            for (uint32_t i = 0; i < v_adj.len; i += 1) {
                uint32_t u = get(v_adj, i);
                int32_t u_mark = get(ctx->marks, u);
                if (u_mark != v_mark) {
                    continue;
                } else if (u_mark == frame.face_mark) {
                    aven_graph_plane_geometry_push_edge(
                        geometry,
                        embedding,
                        u,
                        v,
                        trans,
                        &face_edge_info
                    );
                } else if (u_mark == (frame.face_mark + 1)) {
                    aven_graph_plane_geometry_push_edge(
                        geometry,
                        embedding,
                        u,
                        v,
                        trans,
                        &below_edge_info
                    );
                }
            }
        }
    }

    AvenGraphPlaneGeometryEdge simple_edge_info = {
        .thickness = info->edge_thickness,
    };
    vec4_copy(simple_edge_info.color, info->outline_color);

    for (uint32_t v = 0; v < ctx->graph.len; v += 1) {
        int32_t v_mark = get(ctx->marks, v);

        AvenGraphAdjList v_adj = get(ctx->graph, v);
        for (uint32_t i = 0; i < v_adj.len; i += 1) {
            uint32_t u = get(v_adj, i);
            if (u < v or (v_mark > 0 and get(ctx->marks, u) == v_mark)) {
                continue;
            }

            aven_graph_plane_geometry_push_edge(
                geometry,
                embedding,
                v,
                u,
                trans,
                &simple_edge_info
            );
        }
    }

    AvenGraphPlaneGeometryNode outline_node_info = {
        .mat = {
            { info->radius, 0.0f },
            { 0.0f, info->radius }
        },
        .shape = AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
        .roundness = 1.0f,
    };
    vec4_copy(outline_node_info.color, info->outline_color);

    aven_graph_plane_geometry_push_vertices(
        rounded_geometry,
        embedding,
        trans,
        &outline_node_info
    );

    AvenGraphPlaneGeometryNode color_node_info_data[4];
    AvenGraphPlaneGeometryNodeSlice color_node_infos = slice_array(
        color_node_info_data
    );

    for (size_t i = 0; i < color_node_infos.len; i += 1) {
        AvenGraphPlaneGeometryNode *node_info = &get(color_node_infos, i);
        *node_info = (AvenGraphPlaneGeometryNode){
            .mat = {
                { info->radius - info->border_thickness, 0.0f },
                { 0.0f, info->radius - info->border_thickness }
            },
            .shape = AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
            .roundness = 1.0f,            
        };
        vec4_copy(node_info->color, info->colors[i]);
    }

    for (uint32_t v = 0; v < ctx->graph.len; v += 1) {
        uint32_t v_color = 0;
        if (get(ctx->marks, v) >= 0) {
            v_color = (uint32_t)get(ctx->marks, v);
        }
        aven_graph_plane_geometry_push_vertex(
            rounded_geometry,
            embedding,
            v,
            trans,
            &get(color_node_infos, v_color)
        );
    }

    AvenGraphPlaneGeometryEdge color_edge_info_data[3];
    AvenGraphPlaneGeometryEdgeSlice color_edge_infos = slice_array(
        color_edge_info_data
    );

    for (size_t i = 0; i < color_edge_infos.len; i += 1) {
        AvenGraphPlaneGeometryEdge *edge_info = &get(color_edge_infos, i);
        *edge_info = (AvenGraphPlaneGeometryEdge){
             .thickness = info->edge_thickness,
         };
         vec4_copy(edge_info->color, info->colors[i + 1]);
    }

    for (uint32_t v = 0; v < ctx->graph.len; v += 1) {
        int32_t v_mark = get(ctx->marks, v);

        if (v_mark <= 0) {
            continue;
        }

        AvenGraphAdjList v_adj = get(ctx->graph, v);
        for (uint32_t i = 0; i < v_adj.len; i += 1) {
            uint32_t u = get(v_adj, i);
            if (u < v or get(ctx->marks, u) != v_mark) {
                continue;
            }

            aven_graph_plane_geometry_push_edge(
                geometry,
                embedding,
                v,
                u,
                trans,
                &get(color_edge_infos, (size_t)(v_mark - 1))
            );
        }
    }
}

#endif // AVEN_GRAPH_PLANE_POH_GEOMETRY_H
