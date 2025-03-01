#ifndef GRAPH_PLANE_P3COLOR_GEOMETRY_H
#define GRAPH_PLANE_P3COLOR_GEOMETRY_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/math.h>

#include <aven/gl/shape.h>

#include "../p3color.h"
#include "../geometry.h"

typedef struct {
    Vec4 colors[4];
    Vec4 outline_color;
    Vec4 done_color;
    Vec4 active_color;
    Vec4 face_color;
    Vec4 below_color;
    Vec4 inactive_color;
    float edge_thickness;
    float border_thickness;
    float radius;
} GraphPlaneP3ColorGeometryInfo;

static inline void graph_plane_p3color_geometry_push_ctx(
    AvenGlShapeGeometry *geometry,
    AvenGlShapeRoundedGeometry *rounded_geometry,
    GraphPlaneEmbedding embedding,
    GraphPlaneP3ColorCtx *ctx,
    GraphPlaneP3ColorFrameOptionalSlice active_frames,
    Aff2 trans,
    GraphPlaneP3ColorGeometryInfo *info,
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

    GraphPlaneGeometryNode face_node_info = {
        .mat = {
            { info->radius + info->border_thickness, 0.0f },
            { 0.0f, info->radius + info->border_thickness }
        },
        .shape = GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
        .roundness = 1.0f,
    };
    vec4_copy(face_node_info.color, info->face_color);

    GraphPlaneGeometryNode below_node_info = {
        .mat = {
            { info->radius + info->border_thickness, 0.0f },
            { 0.0f, info->radius + info->border_thickness }
        },
        .shape = GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
        .roundness = 1.0f,
    };
    vec4_copy(below_node_info.color, info->below_color);

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
        for (size_t i = 0; i < active_frames.len; i += 1) {
            GraphPlaneP3ColorFrameOptional *maybe_frame = &get(
                active_frames,
                i
            );
            if (maybe_frame->valid) {
                GraphPlaneP3ColorFrame *frame = &maybe_frame->value;
                if (v == frame->u) {
                    graph_plane_geometry_push_vertex(
                        rounded_geometry,
                        embedding,
                        v,
                        trans,
                        &active_node_info
                    );
                } else if (v == frame->z) {
                    graph_plane_geometry_push_vertex(
                        rounded_geometry,
                        embedding,
                        v,
                        trans,
                        &below_node_info
                    );
                } else if (v == frame->y) {
                    graph_plane_geometry_push_vertex(
                        rounded_geometry,
                        embedding,
                        v,
                        trans,
                        &face_node_info
                    );
                } else if (v == frame->x) {
                    graph_plane_geometry_push_vertex(
                        rounded_geometry,
                        embedding,
                        v,
                        trans,
                        &below_node_info
                    );
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

    typedef Slice(int32_t) GraphPlaneP3ColorGeometryDrawSlice;
    typedef Slice(GraphPlaneP3ColorGeometryDrawSlice)
        GraphPlaneP3ColorGeometryDrawGraph;

    GraphPlaneP3ColorGeometryDrawGraph draw_graph = aven_arena_create_slice(
        GraphPlaneP3ColorGeometryDrawSlice,
        &temp_arena,
        ctx->vertex_info.len
    );

    for (uint32_t v = 0; v < ctx->vertex_info.len; v += 1) {
        get(draw_graph, v).len = get(ctx->vertex_info, v).adj.len;
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
        GraphPlaneP3ColorFrameOptional *maybe_frame = &get(
            active_frames,
            i
        );
        if (maybe_frame->valid) {
            GraphPlaneP3ColorFrame *frame = &maybe_frame->value;

            GraphPlaneP3ColorVertex fu_info = get(ctx->vertex_info, frame->u);

            if (frame->edge_index < fu_info.adj.len) {
                uint32_t n_index = frame->u_nb_first + frame->edge_index;
                if (n_index >= fu_info.adj.len) {
                    n_index -= fu_info.adj.len;
                }

                if (n_index < fu_info.adj.len) {
                    uint32_t n = graph_nb(ctx->nb, fu_info.adj, n_index);
                    if (n > frame->u) {
                        get(get(draw_graph, frame->u), n_index) = -3;
                    } else {
                        GraphPlaneP3ColorVertex n_info = get(
                            ctx->vertex_info,
                            n
                        );
                        uint32_t nu_index = graph_nb_index(
                            ctx->nb,
                            n_info.adj,
                            frame->u
                        );
                        get(get(draw_graph, n), nu_index) = -3;
                    }
                }
            }
        }
    }

    Queue(uint32_t) vertices = aven_arena_create_queue(
        uint32_t,
        &temp_arena,
        ctx->vertex_info.len
    );
    Slice(uint32_t) visited = aven_arena_create_slice(
        uint32_t,
        &temp_arena,
        ctx->vertex_info.len
    );

    for (uint32_t v = 0; v < ctx->vertex_info.len; v += 1) {
        get(visited, v) = 0;
    }

    GraphPlaneGeometryEdge active_edge_info = {
        .thickness = info->edge_thickness + info->border_thickness,
    };
    vec4_copy(active_edge_info.color, info->active_color);

    GraphPlaneGeometryEdge face_edge_info = {
        .thickness = info->edge_thickness + info->border_thickness,
    };
    vec4_copy(face_edge_info.color, info->face_color);

    GraphPlaneGeometryEdge below_edge_info = {
        .thickness = info->edge_thickness + info->border_thickness,
    };
    vec4_copy(below_edge_info.color, info->below_color);

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
        GraphPlaneP3ColorVertex v_info = get(ctx->vertex_info, v);
        if (v_info.mark <= 0) {
            continue;
        }
        GraphPlaneP3ColorGeometryDrawSlice v_drawn = get(draw_graph, v);
        for (uint32_t i = 0; i < v_info.adj.len; i += 1) {
            uint32_t u = graph_nb(ctx->nb, v_info.adj, i);
            GraphPlaneP3ColorVertex u_info = get(ctx->vertex_info, u);
            if (u > v and u_info.mark == v_info.mark) {
                if (get(v_drawn, i) == 0) {
                    get(v_drawn, i) = v_info.mark;
                }
            }
        }
    }

    Optional(GraphPlaneP3ColorFrame *)maybe_cur_frame = { 0 };
    bool active = true;
    uint32_t frame_index = 0;
    while (1) {
        if (active) {
            while (frame_index < active_frames.len) {
                if (get(active_frames, frame_index).valid) {
                    maybe_cur_frame.value = &get(
                        active_frames,
                        frame_index
                    ).value;
                    maybe_cur_frame.valid = true;
                    break;
                }
                frame_index += 1;
            }
            if (frame_index == active_frames.len) {
                active = false;
                frame_index = 0;
            }
        }
        if (!active) {
            if (frame_index < ctx->frames.len) {
                maybe_cur_frame.value = &get(ctx->frames, frame_index);
                maybe_cur_frame.valid = true;
            }
        }

        if (!maybe_cur_frame.valid) {
            break;
        }

        GraphPlaneP3ColorFrame *cur_frame = maybe_cur_frame.value;
        maybe_cur_frame.valid = false;

        frame_index += 1;
        queue_clear(vertices);

        uint32_t mark = frame_index + 1;
        if (!active) {
            mark += (uint32_t)active_frames.len;
        }

        get(visited, cur_frame->u) = mark;

        GraphPlaneP3ColorVertex cfu_info = get(ctx->vertex_info, cur_frame->u);
        GraphPlaneP3ColorGeometryDrawSlice cfu_drawn = get(
            draw_graph,
            cur_frame->u
        );
        uint32_t edge_index = cur_frame->edge_index;
        if (edge_index != 0) {
            edge_index = edge_index - 1;
        }
        for (uint32_t i = cur_frame->edge_index; i < cfu_info.adj.len; i += 1) {
            uint32_t n_index = cur_frame->u_nb_first + i;
            if (n_index >= cfu_info.adj.len) {
                n_index -= cfu_info.adj.len;
            }
            uint32_t n = graph_nb(ctx->nb, cfu_info.adj, n_index);
            if (get(ctx->vertex_info, n).mark <= 0) {
                get(visited, n) = mark;
                queue_push(vertices) = n;
            }
            if (n > cur_frame->u and !get(cfu_drawn, n_index)) {
                if (active) {
                    get(cfu_drawn, n_index) = -1;
                } else {
                    get(cfu_drawn, n_index) = -2;
                }
            } else {
                GraphPlaneP3ColorVertex n_info = get(ctx->vertex_info, n);
                uint32_t nu_index = graph_nb_index(
                    ctx->nb,
                    n_info.adj,
                    cur_frame->u
                );
                GraphPlaneP3ColorGeometryDrawSlice n_drawn = get(
                    draw_graph,
                    n
                );
                if (get(n_drawn, nu_index) == 0) {
                    if (active) {
                        get(n_drawn, nu_index) = -1;
                    } else {
                        get(n_drawn, nu_index) = -2;
                    }
                }
            }
        }

        if (cur_frame->x != cur_frame->u) {
            queue_push(vertices) = cur_frame->x;
            get(visited, cur_frame->x) = mark;
        }

        if (cur_frame->y != cur_frame->u) {
            queue_push(vertices) = cur_frame->y;
            get(visited, cur_frame->y) = mark;
        }

        if (cur_frame->z != cur_frame->u) {
            queue_push(vertices) = cur_frame->z;
            get(visited, cur_frame->z) = mark;
        }

        while (vertices.used > 0) {
            uint32_t v = queue_pop(vertices);
            GraphPlaneP3ColorGeometryDrawSlice v_drawn = get(draw_graph, v);
            GraphPlaneP3ColorVertex v_info = get(ctx->vertex_info, v);

            for (uint32_t i = 0; i < v_info.adj.len; i += 1) {
                uint32_t u = graph_nb(ctx->nb, v_info.adj, i);
                GraphPlaneP3ColorVertex u_info = get(ctx->vertex_info, u);

                if (u_info.mark <= 0) {
                    if (get(visited, u) != mark) {
                        queue_push(vertices) = u;
                        get(visited, u) = mark;
                    }
                }

                if (u > v and !get(v_drawn, i)) {
                    if (active) {
                        get(v_drawn, i) = -1;
                    } else {
                        get(v_drawn, i) = -2;
                    }
                }
            }
        }

        for (
            uint32_t i = cur_frame->edge_index;
            i < cfu_info.adj.len - 1;
            i += 1
        ) {
            uint32_t n_index = cur_frame->u_nb_first + i;
            if (n_index >= cfu_info.adj.len) {
                n_index -= cfu_info.adj.len;
            }

            uint32_t v1 = graph_nb(ctx->nb, cfu_info.adj, n_index);
            uint32_t v2 = graph_nb(
                ctx->nb,
                cfu_info.adj,
                graph_adj_next(cfu_info.adj, n_index)
            );

            if (v1 > v2) {
                GraphPlaneP3ColorVertex v2_info = get(ctx->vertex_info, v2);
                uint32_t v2v1_index = graph_nb_index(
                    ctx->nb,
                    v2_info.adj,
                    v1
                );
                GraphPlaneP3ColorGeometryDrawSlice v2_drawn = get(draw_graph, v2);
                if (get(v2_drawn, v2v1_index) == 0) {
                    if (active) {
                        get(v2_drawn, v2v1_index) = -1;
                    } else {
                        get(v2_drawn, v2v1_index) = -2;
                    }
                }
            } else {
                GraphPlaneP3ColorVertex v1_info = get(ctx->vertex_info, v1);
                uint32_t v1v2_index = graph_nb_index(
                    ctx->nb,
                    v1_info.adj,
                    v2
                );
                GraphPlaneP3ColorGeometryDrawSlice v1_drawn = get(draw_graph, v1);
                if (get(v1_drawn, v1v2_index) == 0) {
                    if (active) {
                        get(v1_drawn, v1v2_index) = -1;
                    } else {
                        get(v1_drawn, v1v2_index) = -2;
                    }
                }
            }
        }

        get(visited, cur_frame->u) = 0;
    }

    for (uint32_t v = 0; v < ctx->vertex_info.len; v += 1) {
        GraphPlaneP3ColorGeometryDrawSlice v_drawn = get(draw_graph, v);
        GraphPlaneP3ColorVertex v_info = get(ctx->vertex_info, v);

        for (uint32_t i = 0; i < v_info.adj.len; i += 1) {
            uint32_t u = graph_nb(ctx->nb, v_info.adj, i);
            if (u < v or get(v_drawn, i) != 0) {
                continue;
            }

            uint32_t i_prev = graph_adj_prev(v_info.adj, i);
            uint32_t i_next = graph_adj_next(v_info.adj, i);

            uint32_t u_prev = graph_nb(ctx->nb, v_info.adj, i_prev);
            uint32_t u_next = graph_nb(ctx->nb, v_info.adj, i_next);

            uint32_t min_mark = get(visited, u_prev);
            uint32_t u_mark = get(visited, u);
            if (min_mark == 0) {
                min_mark = u_mark;
            } else if (u_mark > 0) {
                min_mark = min(u_mark, min_mark);
            }
            uint32_t u_next_mark = get(visited, u_next);
            if (min_mark == 0) {
                min_mark = u_next_mark;
            } else if (u_next_mark > 0) {
                min_mark = min(u_next_mark, min_mark);
            }

            if (min_mark > 0) {
                if (min_mark <= active_frames.len) {
                    get(v_drawn, i) = -1;
                } else {
                    get(v_drawn, i) = -2;
                }
            }
        }
    }

    for (uint32_t v = 0; v < ctx->vertex_info.len; v += 1) {
        GraphPlaneP3ColorGeometryDrawSlice v_drawn = get(draw_graph, v);
        GraphPlaneP3ColorVertex v_info = get(ctx->vertex_info, v);

        for (uint32_t i = 0; i < v_info.adj.len; i += 1) {
            uint32_t u = graph_nb(ctx->nb, v_info.adj, i);
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
        GraphPlaneP3ColorGeometryDrawSlice v_drawn = get(draw_graph, v);
        GraphPlaneP3ColorVertex v_info = get(ctx->vertex_info, v);

        for (uint32_t i = 0; i < v_info.adj.len; i += 1) {
            uint32_t u = graph_nb(ctx->nb, v_info.adj, i);
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
        GraphPlaneP3ColorGeometryDrawSlice v_drawn = get(draw_graph, v);
        GraphPlaneP3ColorVertex v_info = get(ctx->vertex_info, v);

        for (uint32_t i = 0; i < v_info.adj.len; i += 1) {
            uint32_t u = graph_nb(ctx->nb, v_info.adj, i);
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
        GraphPlaneP3ColorGeometryDrawSlice v_drawn = get(draw_graph, v);
        GraphPlaneP3ColorVertex v_info = get(ctx->vertex_info, v);

        for (uint32_t i = 0; i < v_info.adj.len; i += 1) {
            uint32_t u = graph_nb(ctx->nb, v_info.adj, i);
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
        GraphPlaneP3ColorGeometryDrawSlice v_drawn = get(draw_graph, v);
        GraphPlaneP3ColorVertex v_info = get(ctx->vertex_info, v);

        for (uint32_t i = 0; i < v_info.adj.len; i += 1) {
            uint32_t u = graph_nb(ctx->nb, v_info.adj, i);
            if (u < v or get(v_drawn, i) != -3) {
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
            GraphPlaneP3ColorVertex u_info = get(ctx->vertex_info, u);
            if (v_info.mark > 0 and v_info.mark == u_info.mark) {
                graph_plane_geometry_push_edge(
                    geometry,
                    embedding,
                    v,
                    u,
                    trans,
                    &get(color_edge_infos, (size_t)v_info.mark - 1)
                );
            } else {
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
}

#endif // GRAPH_PLANE_P3COLOR_GEOMETRY_H
