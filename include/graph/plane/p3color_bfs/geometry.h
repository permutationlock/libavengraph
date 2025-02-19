#ifndef GRAPH_PLANE_P3COLOR_BFS_GEOMETRY_H
#define GRAPH_PLANE_P3COLOR_BFS_GEOMETRY_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/math.h>

#include <aven/gl/shape.h>

#include "../p3color_bfs.h"
#include "../geometry.h"

typedef struct {
    Vec4 colors[4];
    Vec4 outline_color;
    Vec4 done_color;
    Vec4 active_color;
    Vec4 tree_color;
    Vec4 u_color;
    Vec4 inactive_color;
    float edge_thickness;
    float border_thickness;
    float radius;
} GraphPlaneP3ColorBfsGeometryInfo;

static inline void graph_plane_p3color_bfs_geometry_push_ctx(
    AvenGlShapeGeometry *geometry,
    AvenGlShapeRoundedGeometry *rounded_geometry,
    GraphPlaneEmbedding embedding,
    GraphPlaneP3ColorBfsCtx *ctx,
    GraphPlaneP3ColorBfsFrameOptionalSlice active_frames,
    Aff2 trans,
    GraphPlaneP3ColorBfsGeometryInfo *info,
    AvenArena temp_arena
) {
    GraphPlaneGeometryNode active_node_info = {
        .mat = {
            { info->radius + info->border_thickness, 0.0f },
            { 0.0f, info->radius + info->border_thickness }
        },
        .shape = GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
        .roundness = 1.0f,
    };
    vec4_copy(active_node_info.color, info->active_color);

    GraphPlaneGeometryNode tree_node_info = {
        .mat = {
            { info->radius + info->border_thickness, 0.0f },
            { 0.0f, info->radius + info->border_thickness }
        },
        .shape = GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
        .roundness = 1.0f,
    };
    vec4_copy(tree_node_info.color, info->tree_color);

    GraphPlaneGeometryNode u_node_info = {
        .mat = {
            { info->radius + info->border_thickness, 0.0f },
            { 0.0f, info->radius + info->border_thickness }
        },
        .shape = GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
        .roundness = 1.0f,
    };
    vec4_copy(u_node_info.color, info->u_color);

    GraphPlaneGeometryNode outline_node_info = {
        .mat = {
            { info->radius, 0.0f },
            { 0.0f, info->radius }
        },
        .shape = GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
        .roundness = 1.0f,
    };
    vec4_copy(outline_node_info.color, info->outline_color);

    GraphPlaneGeometryNode color_node_info_data[4];
    GraphPlaneGeometryNodeSlice color_node_infos = slice_array(
        color_node_info_data
    );

    for (size_t i = 0; i < color_node_infos.len; i += 1) {
        GraphPlaneGeometryNode *node_info = &get(color_node_infos, i);
        *node_info = (GraphPlaneGeometryNode){
            .mat = {
                { info->radius - info->border_thickness, 0.0f },
                { 0.0f, info->radius - info->border_thickness }
            },
            .shape = GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
            .roundness = 1.0f,
        };
        vec4_copy(node_info->color, info->colors[i]);
    }

    for (uint32_t v = 0; v < embedding.len; v += 1) {
        GraphPlaneP3ColorBfsVertex v_info = get(ctx->vertex_info, v);
        for (size_t i = 0; i < active_frames.len; i += 1) {
            const GraphPlaneP3ColorBfsFrameOptional *maybe_frame = &get(
                active_frames,
                i
            );
            if (maybe_frame->valid) {
                const GraphPlaneP3ColorBfsFrame *frame = &maybe_frame->value;
                uint32_t active_v = frame->uj;
                if (frame->uj == frame->vk) {
                    active_v = frame->v1;
                }
                if (v == active_v) {
                    graph_plane_geometry_push_vertex(
                        rounded_geometry,
                        embedding,
                        v,
                        trans,
                        &active_node_info
                    );
                    break;
                } else if (v_info.mark == frame->mark) {
                    if (v_info.parent == v) {
                        graph_plane_geometry_push_vertex(
                            rounded_geometry,
                            embedding,
                            v,
                            trans,
                            &u_node_info
                        );
                    } else {
                        graph_plane_geometry_push_vertex(
                            rounded_geometry,
                            embedding,
                            v,
                            trans,
                            &tree_node_info
                        );
                    }
                    break;
                }
            }
        }

        graph_plane_geometry_push_vertex(
            rounded_geometry,
            embedding,
            v,
            trans,
            &outline_node_info
        );

        int32_t mark = get(ctx->vertex_info, v).mark;
        uint32_t color =  mark > 0 ? (uint32_t)mark : 0;
        graph_plane_geometry_push_vertex(
            rounded_geometry,
            embedding,
            v,
            trans,
            &get(color_node_infos, color)
        );
    }

    typedef Slice(int32_t) GraphPlaneP3ColorBfsTikzDrawSlice;
    typedef Slice(GraphPlaneP3ColorBfsTikzDrawSlice)
        GraphPlaneP3ColorBfsTikzDrawGraph;

    GraphPlaneP3ColorBfsTikzDrawGraph draw_graph = aven_arena_create_slice(
        GraphPlaneP3ColorBfsTikzDrawSlice,
        &temp_arena,
        ctx->vertex_info.len
    );

    for (uint32_t v = 0; v < ctx->vertex_info.len; v += 1) {
        get(draw_graph, v).len = get(ctx->vertex_info, v).len;
        get(draw_graph, v).ptr = aven_arena_create_array(
            int32_t,
            &temp_arena,
            get(draw_graph, v).len
        );

        for (size_t i = 0; i < get(draw_graph, v).len; i += 1) {
            get(get(draw_graph, v), i) = 0;
        }
    }

    for (size_t i = 0; i < active_frames.len; i += 1) {
        const GraphPlaneP3ColorBfsFrameOptional *maybe_frame = &get(
            active_frames,
            i
        );
        if (!maybe_frame->valid) {
            continue;
        }
        const GraphPlaneP3ColorBfsFrame *frame = &maybe_frame->value;

        uint32_t v = frame->uj;
        uint32_t edge_index = frame->edge_index;
        if (frame->uj == frame->vk) {
            v = frame->v1;
            edge_index = frame->v1vk_index;
        }
        GraphPlaneP3ColorBfsVertex v_info = get(ctx->vertex_info, v);
        GraphAdjList v_adj = { .ptr = v_info.ptr, .len = v_info.len };
        GraphPlaneP3ColorBfsTikzDrawSlice v_drawn = get(draw_graph, v);

        if (edge_index < v_adj.len) {
            uint32_t u = get(v_adj, edge_index);
            GraphPlaneP3ColorBfsVertex u_info = get(ctx->vertex_info, u);
            GraphAdjList u_adj = { .ptr = u_info.ptr, .len = u_info.len };
            GraphPlaneP3ColorBfsTikzDrawSlice u_drawn = get(draw_graph, u);
            if (u > v) {
                get(v_drawn, edge_index) = -4;
            } else {
                uint32_t uv_index = graph_adj_neighbor_index(u_adj, v);
                get(u_drawn, uv_index) = -4;
            }
        }
    }

    Queue(uint32_t) bfs_queue = aven_arena_create_queue(
        uint32_t,
        &temp_arena,
        ctx->vertex_info.len
    );
    Slice(bool) visited = aven_arena_create_slice(
        bool,
        &temp_arena,
        ctx->vertex_info.len
    );
    for (uint32_t v = 0; v < visited.len; v += 1) {
        get(visited, v) = 0;
    }

    for (size_t i = 0; i < active_frames.len + ctx->frames.len; i += 1) {
        const GraphPlaneP3ColorBfsFrame *frame;
        bool active = false;
        if (i < active_frames.len) {
            const GraphPlaneP3ColorBfsFrameOptional *maybe_frame = &get(
                active_frames,
                i
            );
            if (!maybe_frame->valid) {
                continue;
            }
            frame = &maybe_frame->value;
            active = true;
        } else {
            frame = &get(ctx->frames, i - active_frames.len);
        }

        {
            uint32_t vl_index = frame->v1vk_index;
            uint32_t v = frame->v1;

            bool done = false;
            while (!done) {
                GraphPlaneP3ColorBfsVertex v_info = get(ctx->vertex_info, v);
                GraphAdjList v_adj = {
                    .ptr = v_info.ptr,
                    .len = v_info.len
                };
                GraphPlaneP3ColorBfsTikzDrawSlice v_drawn = get(draw_graph, v);

                uint32_t edge_index = vl_index;
                for (uint32_t j = 0; j < v_adj.len; j += 1) {
                    uint32_t u = get(v_adj, edge_index);
                    GraphPlaneP3ColorBfsVertex u_info = get(ctx->vertex_info, u);
                    GraphAdjList u_adj = {
                        .ptr = u_info.ptr,
                        .len = u_info.len,
                    };

                    if (u > v and get(v_drawn, edge_index) == 0) {
                        if (v_info.mark == u_info.mark) {
                            get(v_drawn, edge_index) = v_info.mark;
                        } else {
                            get(v_drawn, edge_index) = active ? -1 : -2;
                        }
                    }

                    if (u_info.mark <= 0) {
                        if (
                            !get(visited, u) and
                            get(ctx->vertex_info, u).mark <= 0
                        ) {
                            queue_push(bfs_queue) = u;
                            get(visited, u) = true;
                        }
                    } else if (
                        (j > 0 and u_info.mark == v_info.mark) or
                        (v == frame->vi and u == frame->vi1)
                    ) {
                        vl_index = graph_adj_neighbor_index(u_adj, v);
                        v = u;
                        break;
                    } else if (v == frame->vk and u == frame->v1) {
                        done = true;
                        break;
                    }

                    edge_index = graph_adj_next(v_adj, edge_index);
                }
            }
        }

        while (bfs_queue.used > 0) {
            uint32_t v = queue_pop(bfs_queue);
            GraphPlaneP3ColorBfsVertex v_info = get(ctx->vertex_info, v);
            GraphAdjList v_adj = {
                .ptr = v_info.ptr,
                .len = v_info.len
            };
            GraphPlaneP3ColorBfsTikzDrawSlice v_drawn = get(draw_graph, v);

            for (uint32_t j = 0; j < v_adj.len; j += 1) {
                uint32_t u = get(v_adj, j);
                GraphPlaneP3ColorBfsVertex u_info = get(ctx->vertex_info, u);

                if (u > v and get(v_drawn, j) == 0) {
                    if (
                        (
                            v_info.mark == frame->mark and
                            u_info.mark == frame->mark
                        ) and (
                            v_info.parent == u or
                            u_info.parent == v
                        )
                    ) {
                        get(v_drawn, j) = -3;
                    } else {
                        get(v_drawn, j) = active ? -1 : -2;
                    }
                }

                if (!get(visited, u) and u_info.mark <= 0) {
                    queue_push(bfs_queue) = u;
                    get(visited, u) = true;
                }
            }
        }
    }

    GraphPlaneGeometryEdge active_edge_info = {
        .thickness = info->edge_thickness + info->border_thickness,
    };
    vec4_copy(active_edge_info.color, info->active_color);

    GraphPlaneGeometryEdge tree_edge_info = {
        .thickness = info->edge_thickness + info->border_thickness,
    };
    vec4_copy(tree_edge_info.color, info->tree_color);

    GraphPlaneGeometryEdge simple_edge_info = {
        .thickness = info->edge_thickness,
    };
    vec4_copy(simple_edge_info.color, info->outline_color);

    GraphPlaneGeometryEdge inactive_edge_info = {
        .thickness = info->edge_thickness,
    };
    vec4_copy(inactive_edge_info.color, info->inactive_color);

    GraphPlaneGeometryEdge done_edge_info = {
        .thickness = info->edge_thickness,
    };
    vec4_copy(done_edge_info.color, info->done_color);

    GraphPlaneGeometryEdge color_edge_info_data[3];
    GraphPlaneGeometryEdgeSlice color_edge_infos = slice_array(
        color_edge_info_data
    );

    for (size_t i = 0; i < color_edge_infos.len; i += 1) {
        GraphPlaneGeometryEdge *edge_info = &get(color_edge_infos, i);
        *edge_info = (GraphPlaneGeometryEdge){
             .thickness = info->edge_thickness,
         };
         vec4_copy(edge_info->color, info->colors[i + 1]);
    }

    for (uint32_t v = 0; v < ctx->vertex_info.len; v += 1) {
        GraphPlaneP3ColorBfsVertex v_info = get(ctx->vertex_info, v);
        if (v_info.mark <= 0) {
            continue;
        }
        GraphPlaneP3ColorBfsTikzDrawSlice v_drawn = get(draw_graph, v);
        for (uint32_t i = 0; i < v_info.len; i += 1) {
            uint32_t u = get(v_info, i);
            GraphPlaneP3ColorBfsVertex u_info = get(ctx->vertex_info, u);
            if (u > v and u_info.mark == v_info.mark) {
                if (get(v_drawn, i) == 0) {
                    get(v_drawn, i) = v_info.mark;
                }
            }
        }
    }

    for (uint32_t v = 0; v < ctx->vertex_info.len; v += 1) {
        GraphPlaneP3ColorBfsTikzDrawSlice v_drawn = get(draw_graph, v);
        GraphPlaneP3ColorBfsVertex v_info = get(ctx->vertex_info, v);

        for (uint32_t i = 0; i < v_info.len; i += 1) {
            uint32_t u = get(v_info, i);
            if (u < v or get(v_drawn, i) != 0) {
                continue;
            }
            graph_plane_geometry_push_edge(
                geometry,
                embedding,
                v,
                u,
                trans,
                &done_edge_info
            );
        }
    }
    for (uint32_t v = 0; v < ctx->vertex_info.len; v += 1) {
        GraphPlaneP3ColorBfsTikzDrawSlice v_drawn = get(draw_graph, v);
        GraphPlaneP3ColorBfsVertex v_info = get(ctx->vertex_info, v);

        for (uint32_t i = 0; i < v_info.len; i += 1) {
            uint32_t u = get(v_info, i);
            if (u < v or get(v_drawn, i) != -2) {
                continue;
            }
            graph_plane_geometry_push_edge(
                geometry,
                embedding,
                v,
                u,
                trans,
                &inactive_edge_info
            );
        }
    }
    for (uint32_t v = 0; v < ctx->vertex_info.len; v += 1) {
        GraphPlaneP3ColorBfsTikzDrawSlice v_drawn = get(draw_graph, v);
        GraphPlaneP3ColorBfsVertex v_info = get(ctx->vertex_info, v);

        for (uint32_t i = 0; i < v_info.len; i += 1) {
            uint32_t u = get(v_info, i);
            if (u < v or get(v_drawn, i) != -1) {
                continue;
            }
            graph_plane_geometry_push_edge(
                geometry,
                embedding,
                v,
                u,
                trans,
                &simple_edge_info
            );
        }
    }
    for (uint32_t v = 0; v < ctx->vertex_info.len; v += 1) {
        GraphPlaneP3ColorBfsTikzDrawSlice v_drawn = get(draw_graph, v);
        GraphPlaneP3ColorBfsVertex v_info = get(ctx->vertex_info, v);

        for (uint32_t i = 0; i < v_info.len; i += 1) {
            uint32_t u = get(v_info, i);
            if (u < v or get(v_drawn, i) <= 0) {
                continue;
            }
            graph_plane_geometry_push_edge(
                geometry,
                embedding,
                v,
                u,
                trans,
                &get(color_edge_infos, (size_t)get(v_drawn, i) - 1)
            );
        }
    }
    for (uint32_t v = 0; v < ctx->vertex_info.len; v += 1) {
        GraphPlaneP3ColorBfsTikzDrawSlice v_drawn = get(draw_graph, v);
        GraphPlaneP3ColorBfsVertex v_info = get(ctx->vertex_info, v);

        for (uint32_t i = 0; i < v_info.len; i += 1) {
            uint32_t u = get(v_info, i);
            if (u < v or get(v_drawn, i) != -3) {
                continue;
            }
            graph_plane_geometry_push_edge(
                geometry,
                embedding,
                v,
                u,
                trans,
                &tree_edge_info
            );
            graph_plane_geometry_push_edge(
                geometry,
                embedding,
                v,
                u,
                trans,
                &simple_edge_info
            );
        }
    }
    for (uint32_t v = 0; v < ctx->vertex_info.len; v += 1) {
        GraphPlaneP3ColorBfsTikzDrawSlice v_drawn = get(draw_graph, v);
        GraphPlaneP3ColorBfsVertex v_info = get(ctx->vertex_info, v);

        for (uint32_t i = 0; i < v_info.len; i += 1) {
            uint32_t u = get(v_info, i);
            if (u < v or get(v_drawn, i) != -4) {
                continue;
            }
            graph_plane_geometry_push_edge(
                geometry,
                embedding,
                v,
                u,
                trans,
                &active_edge_info
            );
            graph_plane_geometry_push_edge(
                geometry,
                embedding,
                v,
                u,
                trans,
                &simple_edge_info
            );
        }
    }
}

#endif // GRAPH_PLANE_P3COLOR_BFS_GEOMETRY_H
