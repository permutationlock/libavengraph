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
    Vec4 done_color;
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
    AvenGraphPlanePohGeometryInfo *info,
    AvenArena temp_arena
) {
    AvenGraphPlaneGeometryNode active_node_info = {
        .mat = {
            { info->radius + info->border_thickness, 0.0f },
            { 0.0f, info->radius + info->border_thickness }
        },
        .shape = AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
        .roundness = 1.0f,
    };
    vec4_copy(active_node_info.color, info->active_color);

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

    AvenGraphPlaneGeometryNode outline_node_info = {
        .mat = {
            { info->radius, 0.0f },
            { 0.0f, info->radius }
        },
        .shape = AVEN_GRAPH_PLANE_GEOMETRY_SHAPE_SQUARE,
        .roundness = 1.0f,
    };
    vec4_copy(outline_node_info.color, info->outline_color);

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

    for (uint32_t v = 0; v < embedding.len; v += 1) {
        if (maybe_frame->valid) {
            AvenGraphPlanePohFrame *frame = &maybe_frame->value;
            if (v == frame->u) {
                aven_graph_plane_geometry_push_vertex(
                    rounded_geometry,
                    embedding,
                    v,
                    trans,
                    &active_node_info
                );
            } else if (v == frame->z) {
                aven_graph_plane_geometry_push_vertex(
                    rounded_geometry,
                    embedding,
                    v,
                    trans,
                    &below_node_info
                );
            } else if (v == frame->y) {
                aven_graph_plane_geometry_push_vertex(
                    rounded_geometry,
                    embedding,
                    v,
                    trans,
                    &face_node_info
                );
            } else if (v == frame->x) {
                aven_graph_plane_geometry_push_vertex(
                    rounded_geometry,
                    embedding,
                    v,
                    trans,
                    &below_node_info
                );
            }
        }

        aven_graph_plane_geometry_push_vertex(
            rounded_geometry,
            embedding,
            v,
            trans,
            &outline_node_info
        );

        int32_t mark = get(ctx->vertex_info, v).mark;
        uint32_t color =  mark > 0 ? (uint32_t)mark : 0;
        aven_graph_plane_geometry_push_vertex(
            rounded_geometry,
            embedding,
            v,
            trans,
            &get(color_node_infos, color)
        );
    }

    typedef Slice(bool) AvenGraphPlanePohTikzDrawSlice;
    typedef Slice(AvenGraphPlanePohTikzDrawSlice)
        AvenGraphPlanePohTikzDrawGraph;

    AvenGraphPlanePohTikzDrawGraph draw_graph = aven_arena_create_slice(
        AvenGraphPlanePohTikzDrawSlice,
        &temp_arena,
        ctx->vertex_info.len
    );

    for (uint32_t v = 0; v < ctx->vertex_info.len; v += 1) {
        get(draw_graph, v).len = get(ctx->vertex_info, v).len;
        get(draw_graph, v).ptr = aven_arena_create_array(
            bool,
            &temp_arena,
            get(draw_graph, v).len
        );

        for (size_t i = 0; i < get(draw_graph, v).len; i += 1) {
            get(get(draw_graph, v), i) = false;
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

    AvenGraphPlaneGeometryEdge active_edge_info = {
        .thickness = info->edge_thickness + info->border_thickness,
    };
    vec4_copy(active_edge_info.color, info->active_color);

    AvenGraphPlaneGeometryEdge face_edge_info = {
        .thickness = info->edge_thickness + info->border_thickness,
    };
    vec4_copy(face_edge_info.color, info->face_color);

    AvenGraphPlaneGeometryEdge below_edge_info = {
        .thickness = info->edge_thickness + info->border_thickness,
    };
    vec4_copy(below_edge_info.color, info->below_color);

    AvenGraphPlaneGeometryEdge simple_edge_info = {
        .thickness = info->edge_thickness,
    };
    vec4_copy(simple_edge_info.color, info->outline_color);

    AvenGraphPlaneGeometryEdge inactive_edge_info = {
        .thickness = max(
            info->edge_thickness / 2.0f,
            info->edge_thickness - info->border_thickness
        ),
    };
    vec4_copy(inactive_edge_info.color, info->outline_color);

    AvenGraphPlaneGeometryEdge done_edge_info = {
        .thickness = max(
            info->edge_thickness / 2.0f,
            info->edge_thickness - info->border_thickness
        ),
    };
    vec4_copy(done_edge_info.color, info->done_color);

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

    if (maybe_frame->valid) {
        AvenGraphPlanePohFrame *frame = &maybe_frame->value;

        AvenGraphPlanePohVertex fu_info = get(ctx->vertex_info, frame->u);

        if (frame->edge_index < fu_info.len) {
            uint32_t n_index = frame->u_nb_first + frame->edge_index;
            if (n_index >= fu_info.len) {
                n_index -= fu_info.len;
            }

            if (n_index < fu_info.len) {
                uint32_t n = get(fu_info, n_index);
                aven_graph_plane_geometry_push_edge(
                    geometry,
                    embedding,
                    frame->u,
                    n,
                    trans,
                    &active_edge_info
                );
            }
        }
    }

    for (uint32_t v = 0; v < ctx->vertex_info.len; v += 1) {
        AvenGraphPlanePohVertex v_info = get(ctx->vertex_info, v);
        if (v_info.mark <= 0) {
            continue;
        }
        for (uint32_t i = 0; i < v_info.len; i += 1) {
            uint32_t u = get(v_info, i);
            AvenGraphPlanePohVertex u_info = get(ctx->vertex_info, u);
            if (u > v and u_info.mark == v_info.mark) {
                get(get(draw_graph, v), i) = true;
                aven_graph_plane_geometry_push_edge(
                    geometry,
                    embedding,
                    v,
                    u,
                    trans,
                    &get(color_edge_infos, (size_t)v_info.mark - 1)
                );
            }
        }
    }

    Optional(AvenGraphPlanePohFrame *)maybe_cur_frame = { 0 };
    uint32_t frame_index = 0;
    if (maybe_frame->valid) {
        maybe_cur_frame.value = &maybe_frame->value;
        maybe_cur_frame.valid = true;
        frame_index -= (uint32_t)1;
    } else {
        if (ctx->frames.len > 0) {
            maybe_cur_frame.value = &get(ctx->frames, frame_index);
            maybe_cur_frame.valid = true;
        }
    }
    while (maybe_cur_frame.valid) {
        AvenGraphPlanePohFrame *cur_frame = maybe_cur_frame.value;
        maybe_cur_frame.valid = false;

        frame_index += 1;
        queue_clear(vertices);

        uint32_t mark = frame_index + 1;

        get(visited, cur_frame->u) = mark;

        AvenGraphPlanePohVertex cfu_info = get(ctx->vertex_info, cur_frame->u);
        AvenGraphPlanePohTikzDrawSlice cfu_drawn = get(
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
                get(cfu_drawn, n_index) = true;
                if (frame_index == 0) {
                    aven_graph_plane_geometry_push_edge(
                        geometry,
                        embedding,
                        cur_frame->u,
                        n,
                        trans,
                        &simple_edge_info
                    );
                } else {
                    aven_graph_plane_geometry_push_edge(
                        geometry,
                        embedding,
                        cur_frame->u,
                        n,
                        trans,
                        &inactive_edge_info
                    );
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
            AvenGraphPlanePohTikzDrawSlice v_drawn = get(draw_graph, v);
            AvenGraphPlanePohVertex v_info = get(ctx->vertex_info, v);

            for (uint32_t i = 0; i < v_info.len; i += 1) {
                uint32_t u = get(v_info, i);
                AvenGraphPlanePohVertex u_info = get(ctx->vertex_info, u); 

                if (u_info.mark <= 0) {
                    if (get(visited, u) != mark) {
                        queue_push(vertices) = u;
                        get(visited, u) = mark;
                    }
                }

                if (u > v and !get(v_drawn, i)) {
                    get(v_drawn, i) = true;
                    if (frame_index == 0) {
                        aven_graph_plane_geometry_push_edge(
                            geometry,
                            embedding,
                            v,
                            u,
                            trans,
                            &simple_edge_info
                        );
                    } else {
                        aven_graph_plane_geometry_push_edge(
                            geometry,
                            embedding,
                            v,
                            u,
                            trans,
                            &inactive_edge_info
                        );
                    }
                }
            }
        }

        for (uint32_t v = 0; v < ctx->vertex_info.len; v += 1) {
            AvenGraphPlanePohTikzDrawSlice v_drawn = get(draw_graph, v);
            AvenGraphPlanePohVertex v_info = get(ctx->vertex_info, v);

            for (uint32_t i = 0; i < v_info.len; i += 1) {
                uint32_t u = get(v_info, i);
                if (u < v or get(v_drawn, i)) {
                    continue;
                }

                uint32_t i_prev = aven_graph_adj_prev(
                    (AvenGraphAdjList){ .len = v_info.len, .ptr = v_info.ptr },
                    i
                );
                uint32_t i_next = aven_graph_adj_next(
                    (AvenGraphAdjList){ .len = v_info.len, .ptr = v_info.ptr },
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
                    bool valid = false;
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
                            valid = true;
                            break;
                        }
                    }

                    if (!valid) {
                        if (u == cur_frame->u) {
                            u_visited = false;
                        }
                        if (u_prev == cur_frame->u) {
                            u_prev_visited = false;
                        }
                        if (u_next == cur_frame->u) {
                            u_next_visited = false;
                        }
                    }
                }

                if (u_visited or u_prev_visited or u_next_visited) {
                    get(v_drawn, i) = true;
                    if (frame_index == 0) {
                        aven_graph_plane_geometry_push_edge(
                            geometry,
                            embedding,
                            v,
                            u,
                            trans,
                            &simple_edge_info
                        );
                    } else {
                        aven_graph_plane_geometry_push_edge(
                            geometry,
                            embedding,
                            v,
                            u,
                            trans,
                            &inactive_edge_info
                        );
                    }
                }
            }
        }

        if (frame_index < ctx->frames.len) {
            maybe_cur_frame.value = &get(ctx->frames, frame_index);
            maybe_cur_frame.valid = true;
        }
    }

    for (uint32_t v = 0; v < ctx->vertex_info.len; v += 1) {
        AvenGraphPlanePohTikzDrawSlice v_drawn = get(draw_graph, v);
        AvenGraphPlanePohVertex v_info = get(ctx->vertex_info, v);

        for (uint32_t i = 0; i < v_info.len; i += 1) {
            uint32_t u = get(v_info, i);
            if (u < v or get(v_drawn, i)) {
                continue;
            }
            get(v_drawn, i) = true;
            aven_graph_plane_geometry_push_edge(
                geometry,
                embedding,
                v,
                u,
                trans,
                &done_edge_info
            );
        }
    }
}

#endif // AVEN_GRAPH_PLANE_POH_GEOMETRY_H
