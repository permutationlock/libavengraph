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

    typedef Slice(int32_t) GraphPlaneP3ColorTikzDrawSlice;
    typedef Slice(GraphPlaneP3ColorTikzDrawSlice)
        GraphPlaneP3ColorTikzDrawGraph;

    GraphPlaneP3ColorTikzDrawGraph draw_graph = aven_arena_create_slice(
        GraphPlaneP3ColorTikzDrawSlice,
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
        GraphPlaneP3ColorFrameOptional *maybe_frame = &get(
            active_frames,
            i
        );
        if (maybe_frame->valid) {
            GraphPlaneP3ColorFrame *frame = &maybe_frame->value;

            GraphPlaneP3ColorVertex fu_info = get(ctx->vertex_info, frame->u);

            if (frame->edge_index < fu_info.len) {
                uint32_t n_index = frame->u_nb_first + frame->edge_index;
                if (n_index >= fu_info.len) {
                    n_index -= fu_info.len;
                }

                if (n_index < fu_info.len) {
                    uint32_t n = get(fu_info, n_index);
                    if (n > frame->u) {
                        get(get(draw_graph, frame->u), n_index) = -3;
                    } else {
                        GraphPlaneP3ColorVertex n_info = get(
                            ctx->vertex_info,
                            n
                        );
                        uint32_t nu_index = graph_adj_neighbor_index(
                            (GraphAdjList){
                                .ptr = n_info.ptr,
                                .len = n_info.len
                            },
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
        GraphPlaneP3ColorTikzDrawSlice v_drawn = get(draw_graph, v);
        for (uint32_t i = 0; i < v_info.len; i += 1) {
            uint32_t u = get(v_info, i);
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
        GraphPlaneP3ColorTikzDrawSlice cfu_drawn = get(
            draw_graph,
            cur_frame->u
        );
        uint32_t edge_index = cur_frame->edge_index;
        if (edge_index != 0) {
            edge_index = edge_index - 1;
        }
        for (uint32_t i = cur_frame->edge_index; i < cfu_info.len; i += 1) {
            uint32_t n_index = cur_frame->u_nb_first + i;
            if (n_index >= cfu_info.len) {
                n_index -= cfu_info.len;
            }
            uint32_t n = get(cfu_info, n_index);
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
            GraphPlaneP3ColorTikzDrawSlice v_drawn = get(draw_graph, v);
            GraphPlaneP3ColorVertex v_info = get(ctx->vertex_info, v);

            for (uint32_t i = 0; i < v_info.len; i += 1) {
                uint32_t u = get(v_info, i);
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

        for (uint32_t v = 0; v < ctx->vertex_info.len; v += 1) {
            GraphPlaneP3ColorTikzDrawSlice v_drawn = get(draw_graph, v);
            GraphPlaneP3ColorVertex v_info = get(ctx->vertex_info, v);

            for (uint32_t i = 0; i < v_info.len; i += 1) {
                uint32_t u = get(v_info, i);
                if (u < v or get(v_drawn, i) != 0) {
                    continue;
                }

                uint32_t i_prev = graph_adj_prev(
                    (GraphAdjList){ .len = v_info.len, .ptr = v_info.ptr },
                    i
                );
                uint32_t i_next = graph_adj_next(
                    (GraphAdjList){ .len = v_info.len, .ptr = v_info.ptr },
                    i
                );

                uint32_t u_prev = get(v_info, i_prev);
                uint32_t u_next = get(v_info, i_next);

                bool u_prev_visited = get(visited, u_prev) == mark;
                bool u_next_visited = get(visited, u_next) == mark;
                bool u_visited = get(visited, u) == mark;

                if (
                    u == cur_frame->u or
                    u_prev == cur_frame->u or
                    u_next == cur_frame->u
                ) {
                    bool v_valid = false;
                    bool u_valid = (u == cur_frame->u);
                    bool up_valid = (u_prev == cur_frame->u);
                    bool un_valid = (u_next == cur_frame->u);
                    for (
                        uint32_t j = cur_frame->edge_index;
                        j < cfu_info.len;
                        j += 1
                    ) {
                        uint32_t n_index = cur_frame->u_nb_first + j;
                        if (n_index >= cfu_info.len) {
                            n_index -= cfu_info.len;
                        }
                        uint32_t n = get(cfu_info, n_index);
                        if (n == v) {
                            v_valid = true;
                        }
                        if (n == u) {
                            u_valid = true;
                        }
                        if (n == u_next) {
                            un_valid = true;
                        }
                        if (n == u_prev) {
                            up_valid = true;
                        }
                    }

                    if (u == cur_frame->u) {
                        u_visited = u_visited && v_valid && u_valid;
                    }
                    if (u_prev == cur_frame->u) {
                        u_prev_visited = u_prev_visited
                            && u_valid && v_valid && up_valid;
                    }
                    if (u_next == cur_frame->u) {
                        u_next_visited = u_next_visited
                            && u_valid && v_valid && un_valid;
                    }
                }

                if (u_visited or u_prev_visited or u_next_visited) {
                    if (active) {
                        get(v_drawn, i) = -1;
                    } else {
                        get(v_drawn, i) = -2;
                    }
                }
            }
        }
    }

    for (uint32_t v = 0; v < ctx->vertex_info.len; v += 1) {
        GraphPlaneP3ColorTikzDrawSlice v_drawn = get(draw_graph, v);
        GraphPlaneP3ColorVertex v_info = get(ctx->vertex_info, v);

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
        GraphPlaneP3ColorTikzDrawSlice v_drawn = get(draw_graph, v);
        GraphPlaneP3ColorVertex v_info = get(ctx->vertex_info, v);

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
        GraphPlaneP3ColorTikzDrawSlice v_drawn = get(draw_graph, v);
        GraphPlaneP3ColorVertex v_info = get(ctx->vertex_info, v);

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
        GraphPlaneP3ColorTikzDrawSlice v_drawn = get(draw_graph, v);
        GraphPlaneP3ColorVertex v_info = get(ctx->vertex_info, v);

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
        GraphPlaneP3ColorTikzDrawSlice v_drawn = get(draw_graph, v);
        GraphPlaneP3ColorVertex v_info = get(ctx->vertex_info, v);

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
