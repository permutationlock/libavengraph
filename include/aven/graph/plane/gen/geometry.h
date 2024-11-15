#ifndef AVEN_GRAPH_PLANE_GEN_GEOMETRY_H
#define AVEN_GRAPH_PLANE_GEN_GEOMETRY_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/math.h>

#include <aven/gl/shape.h>

#include "../gen.h"
#include "../geometry.h"

typedef struct {
    Vec4 node_color;
    Vec4 outline_color;
    Vec4 edge_color;
    Vec4 active_color;
    float edge_thickness;
    float radius;
    float border_thickness;
} AvenGraphPlaneGenGeometryTriInfo;

static inline void aven_graph_plane_gen_geometry_push_tri_ctx(
    AvenGlShapeGeometry *geometry,
    AvenGlShapeRoundedGeometry *rounded_geometry,
    AvenGraphPlaneGenTriCtx *ctx,
    Aff2 trans,
    AvenGraphPlaneGenGeometryTriInfo *info,
    AvenArena arena
) {
    AvenGraphPlaneGenTriData tri_data = aven_graph_plane_gen_tri_data_alloc(
        (uint32_t)ctx->embedding.len,
        &arena
    );

    AvenGraphPlaneGenData data = aven_graph_plane_gen_tri_data(ctx, &tri_data);

    if (ctx->active_face != AVEN_GRAPH_PLANE_GEN_FACE_INVALID) {
        AvenGraphPlaneGeometryEdge active_edge_info = {
            .thickness = info->edge_thickness + info->border_thickness,
        };
        vec4_copy(active_edge_info.color, info->active_color);

        AvenGraphPlaneGeometryNode active_node_info = {
            .mat = {
                { info->radius + info->border_thickness, 0.0f },
                { 0.0f, info->radius + info->border_thickness }
            },
            .shape = AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
            .roundness = 1.0f,
        };
        vec4_copy(active_node_info.color, info->active_color);

        AvenGraphPlaneGenFace face = list_get(
            ctx->faces,
            ctx->active_face
        );
        for (uint32_t i = 0; i < 3; i += 1) {
            uint32_t j = (i + 1) % 3;
            aven_graph_plane_geometry_push_edge(
                geometry,
                data.embedding,
                face.vertices[i],
                face.vertices[j],
                trans,
                &active_edge_info
            );
        }
        for (uint32_t i = 0; i < 3; i += 1) {
            aven_graph_plane_geometry_push_vertex(
                rounded_geometry,
                data.embedding,
                face.vertices[i],
                trans,
                &active_node_info
            );
        }
    }
    
    AvenGraphPlaneGeometryEdge edge_info = {
        .thickness = info->edge_thickness,
    };
    vec4_copy(edge_info.color, info->edge_color);

    aven_graph_plane_geometry_push_edges(
        geometry,
        data.graph,
        data.embedding,
        trans,
        &edge_info
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
        data.embedding,
        trans,
        &outline_node_info
    );

    AvenGraphPlaneGeometryNode node_info = {
        .mat = {
            { info->radius - info->border_thickness, 0.0f },
            { 0.0f, info->radius - info->border_thickness }
        },
        .shape = AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
        .roundness = 1.0f,
    };
    vec4_copy(node_info.color, info->node_color);

    aven_graph_plane_geometry_push_vertices(
        rounded_geometry,
        data.embedding,
        trans,
        &node_info
    );
}

#endif // AVEN_GRAPH_PLANE_GEN_GEOMETRY_H
