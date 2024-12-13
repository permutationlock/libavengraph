#ifndef AVEN_GRAPH_PLANE_GEOMETRY_H
#define AVEN_GRAPH_PLANE_GEOMETRY_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/math.h>

#include <aven/gl.h>
#include <aven/gl/shape.h>
#include <aven/gl/text.h>

#include "../plane.h"

typedef enum {
    AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
    AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_TRIANGLE,
} AvenGraphPlaneGeometryShape;

typedef struct {
    Vec4 color;
    Mat2 mat;
    AvenGraphPlaneGeometryShape shape;
    float roundness;
} AvenGraphPlaneGeometryNode;
typedef Slice(AvenGraphPlaneGeometryNode) AvenGraphPlaneGeometryNodeSlice;

typedef struct {
    Vec4 color;
    float thickness;
} AvenGraphPlaneGeometryEdge;
typedef Slice(AvenGraphPlaneGeometryEdge) AvenGraphPlaneGeometryEdgeSlice;

static inline void aven_graph_plane_geometry_push_vertex(
    AvenGlShapeRoundedGeometry *geometry,
    AvenGraphPlaneEmbedding embedding,
    uint32_t v,
    Aff2 trans,
    AvenGraphPlaneGeometryNode *draw_info
) {
    Vec2 node_pos;
    aff2_transform(node_pos, trans, get(embedding, v));

    Aff2 node_trans;
    aff2_identity(node_trans);
    mat2_mul_aff2(node_trans, draw_info->mat, node_trans);
    aff2_add_vec2(node_trans, node_trans, node_pos);

    switch (draw_info->shape) {
        case AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_TRIANGLE:
            aven_gl_shape_rounded_geometry_push_triangle_isoceles(
                geometry,
                node_trans,
                draw_info->roundness,
                draw_info->color
            );
            break;
        case AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE:
            aven_gl_shape_rounded_geometry_push_square(
                geometry,
                node_trans,
                draw_info->roundness,
                draw_info->color
            );
            break;
    }
}

static inline void aven_graph_plane_geometry_push_vertices(
    AvenGlShapeRoundedGeometry *geometry,
    AvenGraphPlaneEmbedding embedding,
    Aff2 trans,
    AvenGraphPlaneGeometryNode *draw_info
) {
    for (uint32_t v = 0; v < embedding.len; v += 1) {
        aven_graph_plane_geometry_push_vertex(
            geometry,
            embedding,
            v,
            trans,
            draw_info
        );
    }
}

static inline void aven_graph_plane_geometry_push_edge(
    AvenGlShapeGeometry *geometry,
    AvenGraphPlaneEmbedding embedding,
    uint32_t u,
    uint32_t v,
    Aff2 trans,
    AvenGraphPlaneGeometryEdge *draw_info
) {
    Vec2 p1;
    aff2_transform(p1, trans, get(embedding, u));

    Vec2 p2;
    aff2_transform(p2, trans, get(embedding, v));

    Vec2 center;
    vec2_midpoint(center, p1, p2);

    Vec2 p1p2;
    vec2_sub(p1p2, p2, p1);

    float dist = vec2_mag(p1p2);

    Aff2 edge_trans;
    aff2_position(
        edge_trans,
        center,
        (Vec2){ draw_info->thickness + dist / 2.0f, draw_info->thickness },
        vec2_angle_xaxis(p1p2)
    );

    aven_gl_shape_geometry_push_square(
        geometry,
        edge_trans,
        draw_info->color
    );
}

static inline void aven_graph_plane_geometry_push_edges(
    AvenGlShapeGeometry *geometry,
    AvenGraph graph,
    AvenGraphPlaneEmbedding embedding,
    Aff2 trans,
    AvenGraphPlaneGeometryEdge *draw_info
) {
    for (uint32_t u = 0; u < graph.len; u += 1) {
        AvenGraphAdjList u_neighbors = get(graph, u);
        for (uint32_t i = 0; i < u_neighbors.len; i += 1) {
            uint32_t v = get(u_neighbors, i);
            if (v < u) {
                continue;
            }

            aven_graph_plane_geometry_push_edge(
                geometry,
                embedding,
                u,
                v,
                trans,
                draw_info
            );
        }
    }
}

static inline void aven_graph_plane_geometry_push_label(
    AvenGlTextGeometry *geometry,
    AvenGlTextFont * font,
    AvenGraphPlaneEmbedding embedding,
    uint32_t v,
    Aff2 trans,
    Vec2 label_offset,
    float scale,
    Vec4 color,
    AvenArena arena
) {
    AvenStr label_text = aven_str_uint_decimal(v, &arena);
    AvenGlTextLine label_line = aven_gl_text_line(font, label_text, &arena);

    Vec2 pos;
    aff2_transform(pos, trans, get(embedding, v));

    Aff2 label_trans;
    aff2_identity(label_trans);
    aff2_add_vec2(label_trans, label_trans, pos);
    aff2_add_vec2(label_trans, label_trans, label_offset);

    aven_gl_text_geometry_push_line(
        geometry,
        &label_line,
        label_trans,
        scale,
        color
    );
}

static inline void aven_graph_plane_geometry_push_labels(
    AvenGlTextGeometry *geometry,
    AvenGlTextFont * font,
    AvenGraphPlaneEmbedding embedding,
    Aff2 trans,
    Vec2 label_offset,
    float scale,
    Vec4 color,
    AvenArena arena
) {
    for (uint32_t v = 0; v < embedding.len; v += 1) {
        aven_graph_plane_geometry_push_label(
            geometry,
            font,
            embedding,
            v,
            trans,
            label_offset,
            scale,
            color,
            arena
        );
    }
}

static inline void aven_graph_plane_geometry_push_subset_vertices(
    AvenGlShapeRoundedGeometry *geometry,
    AvenGraphPlaneEmbedding embedding,
    AvenGraphSubset vertices,
    Aff2 trans,
    AvenGraphPlaneGeometryNode *draw_info
) {
    if (vertices.len == 0) {
        return;
    }

    for (size_t i = 0; i < vertices.len; i += 1) {
        uint32_t v = get(vertices, i);
        aven_graph_plane_geometry_push_vertex(
            geometry,
            embedding,
            v,
            trans,
            draw_info
        );
    }
}

static inline void aven_graph_plane_geometry_push_path_edges(
    AvenGlShapeGeometry *geometry,
    AvenGraphPlaneEmbedding embedding,
    AvenGraphSubset path,
    Aff2 trans,
    AvenGraphPlaneGeometryEdge *draw_info
) {
    if (path.len < 2) {
        return;
    }

    for (size_t i = 0; i < path.len - 1; i += 1) {
        uint32_t u = get(path, i);
        uint32_t v = get(path, i + 1);

        aven_graph_plane_geometry_push_edge(
            geometry,
            embedding,
            u,
            v,
            trans,
            draw_info
        );
    }
}

static inline void aven_graph_plane_geometry_push_cycle_edges(
    AvenGlShapeGeometry *geometry,
    AvenGraphPlaneEmbedding embedding,
    AvenGraphSubset cycle,
    Aff2 trans,
    AvenGraphPlaneGeometryEdge *draw_info
) {
    if (cycle.len < 2) {
        return;
    }

    aven_graph_plane_geometry_push_path_edges(
        geometry,
        embedding,
        cycle,
        trans,
        draw_info
    );

    if (cycle.len > 2) {
        aven_graph_plane_geometry_push_edge(
            geometry,
            embedding,
            get(cycle, 0),
            get(cycle, cycle.len - 1),
            trans,
            draw_info
        );
    }
}

static inline void aven_graph_plane_geometry_push_marked_vertices(
    AvenGlShapeRoundedGeometry *geometry,
    AvenGraphPlaneEmbedding embedding,
    AvenGraphPropUint32 marking,
    uint32_t mark,
    Aff2 trans,
    AvenGraphPlaneGeometryNode *draw_info
) {
    for (uint32_t v = 0; v < embedding.len; v += 1) {
        if (get(marking, v) != mark) {
            continue;
        }

        aven_graph_plane_geometry_push_vertex(
            geometry,
            embedding,
            v,
            trans,
            draw_info
        );
    }
}

static inline void aven_graph_plane_geometry_push_marked_edges(
    AvenGlShapeGeometry *geometry,
    AvenGraph graph,
    AvenGraphPlaneEmbedding embedding,
    AvenGraphPropUint32 marking,
    uint32_t mark,
    Aff2 trans,
    AvenGraphPlaneGeometryEdge *draw_info
) {
    for (uint32_t u = 0; u < graph.len; u += 1) {
        if (get(marking, u) != mark) {
            continue;
        }

        AvenGraphAdjList adj = get(graph, u);
        for (uint32_t i = 0; i < adj.len; i += 1) {
            uint32_t v = get(adj, i);
            if (get(marking, v) != mark) {
                continue;
            }

            aven_graph_plane_geometry_push_edge(
                geometry,
                embedding,
                u,
                v,
                trans,
                draw_info
            );
        }
    }
}

static inline void aven_graph_plane_geometry_push_unmarked_vertices(
    AvenGlShapeRoundedGeometry *geometry,
    AvenGraphPlaneEmbedding embedding,
    AvenGraphPropUint32 marking,
    uint32_t mark,
    Aff2 trans,
    AvenGraphPlaneGeometryNode *draw_info
) {
    for (uint32_t v = 0; v < embedding.len; v += 1) {
        if (get(marking, v) == mark) {
            continue;
        }

        aven_graph_plane_geometry_push_vertex(
            geometry,
            embedding,
            v,
            trans,
            draw_info
        );
    }
}

static inline void aven_graph_plane_geometry_push_unmarked_edges(
    AvenGlShapeGeometry *geometry,
    AvenGraph graph,
    AvenGraphPlaneEmbedding embedding,
    AvenGraphPropUint32 marking,
    uint32_t mark,
    Aff2 trans,
    AvenGraphPlaneGeometryEdge *draw_info
) {
    for (uint32_t u = 0; u < graph.len; u += 1) {
        if (get(marking, u) == mark) {
            continue;
        }

        AvenGraphAdjList adj = get(graph, u);
        for (uint32_t i = 0; i < adj.len; i += 1) {
            uint32_t v = get(adj, i);
            if (get(marking, v) == mark) {
                continue;
            }

            aven_graph_plane_geometry_push_edge(
                geometry,
                embedding,
                u,
                v,
                trans,
                draw_info
            );
        }
    }
}

static inline void aven_graph_plane_geometry_subgraph_transform(
    Aff2 dest,
    AvenGraphPlaneEmbedding embedding,
    AvenGraphSubset vertices
) {
    assert(vertices.len > 0);

    Vec2 min_coord = { 1.0f, 1.0f };
    Vec2 max_coord = { -1.0f, -1.0f };

    for (size_t i = 0; i < vertices.len; i += 1) {
        uint32_t v = get(vertices, i);
        Vec2 pos;
        vec2_copy(pos, get(embedding, v));

        min_coord[0] = min(min_coord[0], pos[0]);
        min_coord[1] = min(min_coord[1], pos[1]);
        max_coord[0] = max(max_coord[0], pos[0]);
        max_coord[1] = max(max_coord[1], pos[1]);
    }

    float width = max_coord[0] - min_coord[0];
    float height = max_coord[1] - min_coord[1];

    float denom = max(height, width);
    float scale = 1.0f;
    if (denom > 0.000000000001f) {
        scale = 2.0f / denom;
    }

    Vec2 center;
    vec2_midpoint(center, min_coord, max_coord);

    aff2_identity(dest);
    aff2_sub_vec2(dest, dest, center);
    aff2_scale(
        dest,
        scale,
        dest
    );
}

static inline void aven_graph_plane_geometry_push_colored_vertices(
    AvenGlShapeRoundedGeometry *geometry,
    AvenGraphPlaneEmbedding embedding,
    AvenGraphPropUint8 coloring,
    Aff2 trans,
    AvenGraphPlaneGeometryNodeSlice draw_infos
) {
    for (uint32_t v = 0; v < embedding.len; v += 1) {
        aven_graph_plane_geometry_push_vertex(
            geometry,
            embedding,
            v,
            trans,
            &get(draw_infos, get(coloring, v))
        );
    }
}

static inline void aven_graph_plane_geometry_push_colored_edges(
    AvenGlShapeGeometry *geometry,
    AvenGraph graph,
    AvenGraphPlaneEmbedding embedding,
    AvenGraphPropUint8 coloring,
    Aff2 trans,
    AvenGraphPlaneGeometryEdgeSlice draw_infos
) {
    for (uint32_t v = 0; v < embedding.len; v += 1) {
        uint8_t v_color = get(coloring, v);
        if (v_color == 0) {
            continue;
        }
        AvenGraphAdjList v_adj = get(graph, v);
        for (uint32_t i = 0; i < v_adj.len; i += 1) {
            uint32_t u = get(v_adj, i);
            if (v < u) {
                continue;
            }
            if (get(coloring, u) != v_color) {
                continue;
            }
            aven_graph_plane_geometry_push_edge(
                geometry,
                embedding,
                v,
                u,
                trans,
                &get(draw_infos, v_color - 1)
            );
        }
    }
}

static inline void aven_graph_plane_geometry_push_uncolored_edges(
    AvenGlShapeGeometry *geometry,
    AvenGraph graph,
    AvenGraphPlaneEmbedding embedding,
    AvenGraphPropUint8 coloring,
    Aff2 trans,
    AvenGraphPlaneGeometryEdge *draw_info
) {
    for (uint32_t v = 0; v < embedding.len; v += 1) {
        uint8_t v_color = get(coloring, v);
        AvenGraphAdjList v_adj = get(graph, v);
        for (uint32_t i = 0; i < v_adj.len; i += 1) {
            uint32_t u = get(v_adj, i);
            if (v < u) {
                continue;
            }
            if (v_color != 0 and get(coloring, u) == v_color) {
                continue;
            }
            aven_graph_plane_geometry_push_edge(
                geometry,
                embedding,
                v,
                u,
                trans,
                draw_info
            );
        }
    }
}

#endif // AVEN_GRAPH_PLANE_GEOMETRY_H
