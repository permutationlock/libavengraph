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
    Aff2 trans,
    AvenGraphPlanePohGeometryInfo *info
) {
    if (ctx->frames.len > 0) {
        AvenGraphPlanePohFrame frame = list_get(
            ctx->frames,
            ctx->frames.len - 1
        );
    
        AvenGraphAdjList u_adj = slice_get(ctx->graph, frame.u);
        if (frame.edge_index < u_adj.len) {
            AvenGraphPlaneGeometryEdge active_edge_info = {
                .thickness = info->edge_thickness + info->border_thickness,
            };
            vec4_copy(active_edge_info.color, info->active_color);

            uint32_t v = slice_get(
                u_adj,
                (slice_get(ctx->first_edges, frame.u) + frame.edge_index) %
                    u_adj.len
            );

            if (slice_get(ctx->marks, v) == frame.face_mark) {
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

        aven_graph_plane_geometry_push_marked_edges(
            geometry,
            ctx->graph,
            embedding,
            ctx->marks,
            frame.face_mark,
            trans,
            &face_edge_info
        );

        AvenGraphPlaneGeometryNode face_node_info = {
            .mat = {
                { info->radius + info->border_thickness, 0.0f },
                { 0.0f, info->radius + info->border_thickness }
            },
            .shape = AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
            .roundness = 1.0f,
        };
        vec4_copy(face_node_info.color, info->face_color);

        aven_graph_plane_geometry_push_marked_vertices(
            rounded_geometry,
            embedding,
            ctx->marks,
            frame.face_mark,
            trans,
            &face_node_info
        );

        AvenGraphPlaneGeometryEdge below_edge_info = {
            .thickness = info->edge_thickness + info->border_thickness,
        };
        vec4_copy(below_edge_info.color, info->below_color);

        aven_graph_plane_geometry_push_marked_edges(
            geometry,
            ctx->graph,
            embedding,
            ctx->marks,
            frame.face_mark + 1,
            trans,
            &below_edge_info
        );

        AvenGraphPlaneGeometryNode below_node_info = {
            .mat = {
                { info->radius + info->border_thickness, 0.0f },
                { 0.0f, info->radius + info->border_thickness }
            },
            .shape = AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
            .roundness = 1.0f,
        };
        vec4_copy(below_node_info.color, info->below_color);

        aven_graph_plane_geometry_push_marked_vertices(
            rounded_geometry,
            embedding,
            ctx->marks,
            frame.face_mark + 1,
            trans,
            &below_node_info
        );
    }

    AvenGraphPlaneGeometryEdge simple_edge_info = {
        .thickness = info->edge_thickness,
    };
    vec4_copy(simple_edge_info.color, info->outline_color);

    aven_graph_plane_geometry_push_uncolored_edges(
        geometry,
        ctx->graph,
        embedding,
        ctx->coloring,
        trans,
        &simple_edge_info
    );

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
        AvenGraphPlaneGeometryNode *node_info = &slice_get(color_node_infos, i);
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

    aven_graph_plane_geometry_push_colored_vertices(
        rounded_geometry,
        embedding,
        ctx->coloring,
        trans,
        color_node_infos
    );

    AvenGraphPlaneGeometryEdge color_edge_info_data[3];
    AvenGraphPlaneGeometryEdgeSlice color_edge_infos = slice_array(
        color_edge_info_data
    );

    for (size_t i = 0; i < color_edge_infos.len; i += 1) {
        AvenGraphPlaneGeometryEdge *edge_info = &slice_get(color_edge_infos, i);
        *edge_info = (AvenGraphPlaneGeometryEdge){
             .thickness = info->edge_thickness,
         };
         vec4_copy(edge_info->color, info->colors[i + 1]);
    }

    aven_graph_plane_geometry_push_colored_edges(
        geometry,
        ctx->graph,
        embedding,
        ctx->coloring,
        trans,
        color_edge_infos
    );
}

#endif // AVEN_GRAPH_PLANE_POH_GEOMETRY_H
