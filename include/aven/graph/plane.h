#ifndef AVEN_GRAPH_PLANE_H
#define AVEN_GRAPH_PLANE_H

#include <aven.h>
#include <aven/arena.h>

#include <aven/gl.h>
#include <aven/glm.h>

#include "../graph.h"

typedef Slice(Vec2) AvenGraphPlaneEmbedding;

typedef enum {
    AVEN_GRAPH_PLANE_NODE_DRAW_SHAPE_SQUARE,
    AVEN_GRAPH_PLANE_NODE_DRAW_SHAPE_TRIANGLE,
} AvenGraphPlaneNodeDrawShape;

typedef struct {
    Vec4 color;
    AvenGraphPlaneNodeDrawShape shape;
    float radius;
    float roundness;
    float rotation_angle;
} AvenGraphPlaneNodeDrawInfo;
typedef Slice(AvenGraphPlaneNodeDrawInfo) AvenGraphPlaneNodeDrawInfoSlice;

static inline void aven_graph_plane_shape_geometry_push_edges(
    AvenGlShapeGeometry *geometry,
    AvenGraph graph,
    AvenGraphPlaneEmbedding embedding,
    float thickness,
    Vec4 color
) {
    for (uint32_t v = 0; v < graph.len; v += 1) {
        AvenGraphAdjList adj = slice_get(g, v);
        for (size_t n = 0; n < adj.len; n += 1) {
            uint32_t u = slice_get(adj, n);
            if (u <= v) {
                continue;
            }

            aven_gl_shape_geometry_push_line(
                geometry,
                slice_get(embedding, v),
                slice_get(embedding, u),
                thickness,
                0.0f,
                color
            );
        }
    }
}

static inline void aven_graph_plane_shape_geometry_push_vertex(
    AvenGlShapeGeometry *geometry,
    Vec2 pos,
    AvenGraphPlaneNodeDrawInfo draw_info
) {
    switch (draw_info.shape) {
        case AVEN_GRAPH_PLANE_NODE_DRAW_SHAPE_TRIANGLE:
            aven_gl_shape_geometry_push_triangle_equilateral(
                geometry,
                pos,
                draw_info.radius * AVEN_GLM_SQRT3_F,
                draw_info.rotation_angle,
                draw_info.roundness,
                draw_info.color
            );
            break;
        case AVEN_GRAPH_PLANE_NODE_DRAW_SHAPE_SQUARE:
            aven_gl_shape_geometry_push_rectangle(
                geometry,
                pos,
                (Vec2){ draw_info.radius, draw_info.radius },
                draw_info.rotation_angle,
                draw_info.roundness,
                draw_info.color
            );
            break;
    }
}

static inline void aven_graph_plane_shape_geometry_push_graph(
    AvenGlShapeGeometry *geometry,
    AvenGraph graph,
    AvenGraphPlaneEmbedding embedding,
    float edge_thickness,
    Vec4 edge_color,
    AvenGraphPlaneNodeDrawInfo draw_info
) {
    aven_graph_plane_shape_geometry_push_edges(
        geometry,
        graph,
        embedding,
        edge_thickness,
        edge_color
    );

    for (uint32_t v = 0; v < graph.len; v += 1) {
        aven_graph_plane_shape_geometry_push_vertex(
            geometry,
            slice_get(embedding, v),
            draw_info
        );
    }
}

static inline void aven_graph_plane_shape_geometry_push_graph_coloring(
    AvenGlShapeGeometry *geometry,
    AvenGraph graph,
    AvenGraphPlaneEmbedding embedding,
    AvenGraphColoring coloring,
    float edge_thickness,
    Vec4 edge_color,
    AvenGraphPlaneNodeDrawInfoSlice color_draw_info
) {
    aven_graph_plane_shape_geometry_push_edges(
        geometry,
        graph,
        embedding,
        edge_thickness,
        edge_color
    );

    for (uint32_t v = 0; v < graph.len; v += 1) {
        aven_graph_plane_shape_geometry_push_vertex(
            geometry,
            slice_get(embedding, v),
            slice_get(color_draw_info, slice_get(coloring, v))
        );
    }
}

#endif // AVEN_GRAPH_H

