#ifndef GRAPH_PLANE_GEN_GEOMETRY_H
    #define GRAPH_PLANE_GEN_GEOMETRY_H

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
    } GraphPlaneGenGeometryTriInfo;

    static inline void graph_plane_gen_geometry_push_tri_ctx(
        AvenGlShapeGeometry *geometry,
        AvenGlShapeRoundedGeometry *rounded_geometry,
        GraphPlaneGenTriCtx *ctx,
        Aff2 trans,
        GraphPlaneGenGeometryTriInfo *info,
        AvenArena arena
    ) {
        GraphPlaneGenTriData tri_data = graph_plane_gen_tri_data_alloc(
            (uint32_t)ctx->embedding.len,
            &arena
        );

        GraphPlaneGenData data = graph_plane_gen_tri_data(ctx, &tri_data);

        if (ctx->active_face != GRAPH_PLANE_GEN_FACE_INVALID) {
            GraphPlaneGeometryEdge active_edge_info = {
                .thickness = info->edge_thickness + info->border_thickness,
            };
            vec4_copy(active_edge_info.color, info->active_color);

            GraphPlaneGeometryNode active_node_info = {
                .mat = {
                    { info->radius + info->border_thickness, 0.0f },
                    { 0.0f, info->radius + info->border_thickness },
                },
                .shape = GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
                .roundness = 1.0f,
            };
            vec4_copy(active_node_info.color, info->active_color);

            GraphPlaneGenFace face = list_get(ctx->faces, ctx->active_face);
            for (uint32_t i = 0; i < 3; i += 1) {
                uint32_t j = (i + 1) % 3;
                graph_plane_geometry_push_edge(
                    geometry,
                    data.embedding,
                    face.vertices[i],
                    face.vertices[j],
                    trans,
                    &active_edge_info
                );
            }
            for (uint32_t i = 0; i < 3; i += 1) {
                graph_plane_geometry_push_vertex(
                    rounded_geometry,
                    data.embedding,
                    face.vertices[i],
                    trans,
                    &active_node_info
                );
            }
        }

        GraphPlaneGeometryEdge edge_info = { .thickness = info->edge_thickness };
        vec4_copy(edge_info.color, info->edge_color);

        graph_plane_geometry_push_edges(
            geometry,
            data.graph,
            data.embedding,
            trans,
            &edge_info
        );

        GraphPlaneGeometryNode outline_node_info = {
            .mat = { { info->radius, 0.0f }, { 0.0f, info->radius } },
            .shape = GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
            .roundness = 1.0f,
        };
        vec4_copy(outline_node_info.color, info->outline_color);

        graph_plane_geometry_push_vertices(
            rounded_geometry,
            data.embedding,
            trans,
            &outline_node_info
        );

        GraphPlaneGeometryNode node_info = {
            .mat = {
                { info->radius - info->border_thickness, 0.0f },
                { 0.0f, info->radius - info->border_thickness },
            },
            .shape = GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
            .roundness = 1.0f,
        };
        vec4_copy(node_info.color, info->node_color);

        graph_plane_geometry_push_vertices(
            rounded_geometry,
            data.embedding,
            trans,
            &node_info
        );
    }
#endif // GRAPH_PLANE_GEN_GEOMETRY_H
