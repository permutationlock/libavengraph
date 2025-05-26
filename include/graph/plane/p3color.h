#ifndef GRAPH_PLANE_P3COLOR_H
    #define GRAPH_PLANE_P3COLOR_H

    #include <aven.h>
    #include <aven/arena.h>

    #include "../../graph.h"

    typedef struct {
        uint32_t u;
        uint32_t u_nb_first;
        uint32_t x;
        uint32_t x_nb_first;
        uint32_t y;
        uint32_t z;
        uint32_t edge_index;
        int32_t face_mark;
        uint8_t q_color;
        uint8_t p_color;
        bool above_path;
        bool last_colored;
    } GraphPlaneP3ColorFrame;

    typedef Optional(GraphPlaneP3ColorFrame) GraphPlaneP3ColorFrameOptional;
    typedef Slice(GraphPlaneP3ColorFrameOptional)
        GraphPlaneP3ColorFrameOptionalSlice;

    typedef struct {
        GraphAdj adj;
        int32_t mark;
    } GraphPlaneP3ColorVertex;

    typedef struct {
        GraphNbSlice nb;
        Slice(GraphPlaneP3ColorVertex) vertex_info;
        List(GraphPlaneP3ColorFrame) frames;
    } GraphPlaneP3ColorCtx;

    static inline GraphPlaneP3ColorCtx graph_plane_p3color_init(
        Graph graph,
        GraphSubset p,
        GraphSubset q,
        AvenArena *arena
    ) {
        uint32_t p1 = get(p, 0);
        uint32_t q1 = get(q, 0);

        GraphPlaneP3ColorCtx ctx = {
            .nb = graph.nb,
            .vertex_info = { .len = graph.adj.len },
            .frames = { .cap = graph.adj.len - 2 },
        };

        ctx.vertex_info.ptr = aven_arena_create_array(
            GraphPlaneP3ColorVertex,
            arena,
            ctx.vertex_info.len
        );
        ctx.frames.ptr = aven_arena_create_array(
            GraphPlaneP3ColorFrame,
            arena,
            ctx.frames.cap
        );

        for (uint32_t v = 0; v < ctx.vertex_info.len; v += 1) {
            get(ctx.vertex_info, v) = (GraphPlaneP3ColorVertex){
                .adj = get(graph.adj, v),
            };
        }

        for (uint32_t i = 0; i < p.len; i += 1) {
            get(ctx.vertex_info, get(p, i)).mark = -1;
        }

        get(ctx.vertex_info, p1).mark = 1;

        for (uint32_t i = 0; i < q.len; i += 1) {
            get(ctx.vertex_info, get(q, i)).mark = 2;
        }

        list_push(ctx.frames) = (GraphPlaneP3ColorFrame){
            .p_color = 3,
            .q_color = 2,
            .u = p1,
            .u_nb_first = graph_nb_index(graph.nb, get(graph.adj, p1), q1),
            .x = p1,
            .y = p1,
            .z = p1,
            .face_mark = -1,
        };

        return ctx;
    }

    static inline GraphPlaneP3ColorFrameOptional graph_plane_p3color_next_frame(
        GraphPlaneP3ColorCtx *ctx
    ) {
        if (ctx->frames.len == 0) {
            return (GraphPlaneP3ColorFrameOptional){ 0 };
        }

        return (GraphPlaneP3ColorFrameOptional){
            .value = list_pop(ctx->frames),
            .valid = true,
        };
    }

    static inline bool graph_plane_p3color_frame_step(
        GraphPlaneP3ColorCtx *ctx,
        GraphPlaneP3ColorFrame *frame
    ) {
        uint8_t path_color = frame->p_color ^ frame->q_color;

        GraphPlaneP3ColorVertex *u_info = &get(ctx->vertex_info, frame->u);

        if (frame->edge_index == u_info->adj.len) {
            assert(frame->z == frame->u);

            if (frame->y == frame->u) {
                assert(frame->x == frame->u);
                return true;
            }

            if (frame->x == frame->u) {
                frame->x = frame->y;
            }

            GraphPlaneP3ColorVertex *y_info = &get(ctx->vertex_info, frame->y);
            frame->u_nb_first = graph_adj_next(
                y_info->adj,
                graph_nb_index(ctx->nb, y_info->adj, frame->u)
            );
            frame->u = frame->y;
            frame->z = frame->y;
            frame->edge_index = 0;
            frame->above_path = false;
            frame->last_colored = false;
            return false;
        }

        uint32_t v_index = frame->u_nb_first + frame->edge_index;
        if (v_index >= u_info->adj.len) {
            v_index -= u_info->adj.len;
        }

        uint32_t v = graph_nb(ctx->nb, u_info->adj, v_index);
        GraphPlaneP3ColorVertex *v_info = &get(ctx->vertex_info, v);

        frame->edge_index += 1;

        if (frame->above_path) {
            if (v_info->mark <= 0) {
                if (frame->last_colored) {
                    frame->z = v;
                    v_info->mark = (int32_t)frame->q_color;
                } else {
                    v_info->mark = frame->face_mark - 1;
                }
                frame->last_colored = false;
            } else {
                frame->last_colored = true;
                if (frame->z != frame->u) {
                    GraphPlaneP3ColorVertex *z_info = &get(
                        ctx->vertex_info,
                        frame->z
                    );
                    list_push(ctx->frames) = (GraphPlaneP3ColorFrame){
                        .p_color = path_color,
                        .q_color = frame->p_color,
                        .u = frame->z,
                        .u_nb_first = graph_adj_next(
                            z_info->adj,
                            graph_nb_index(ctx->nb, z_info->adj, frame->u)
                        ),
                        .x = frame->z,
                        .y = frame->z,
                        .z = frame->z,
                        .face_mark = frame->face_mark - 1,
                    };
                    frame->z = frame->u;
                }
            }
        } else if (v != frame->x) {
            if (v_info->mark > 0) {
                if (v_info->mark == (int32_t)frame->p_color) {
                    frame->above_path = true;
                    frame->last_colored = true;
                }
                if (frame->x != frame->u) {
                    list_push(ctx->frames) = (GraphPlaneP3ColorFrame){
                        .p_color = path_color,
                        .q_color = frame->q_color,
                        .u = frame->x,
                        .u_nb_first = frame->x_nb_first,
                        .x = frame->x,
                        .y = frame->x,
                        .z = frame->x,
                        .face_mark = frame->face_mark - 1,
                    };

                    frame->x = frame->u;
                }
            } else if (v_info->mark == frame->face_mark) {
                v_info->mark = (int32_t)path_color;
                frame->y = v;
                frame->above_path = true;
            } else {
                if (v_info->mark <= 0) {
                    v_info->mark = frame->face_mark - 1;
                }

                if (frame->x == frame->u) {
                    frame->x = v;
                    frame->x_nb_first = graph_adj_next(
                        v_info->adj,
                        graph_nb_index(ctx->nb, v_info->adj, frame->u)
                    );

                    v_info->mark = (int32_t)frame->p_color;
                }
            }
        }

        return false;
    }

    static inline GraphPropUint8 graph_plane_p3color(
        Graph graph,
        GraphSubset p,
        GraphSubset q,
        AvenArena *arena
    ) {
        GraphPropUint8 coloring = { .len = graph.adj.len };
        coloring.ptr = aven_arena_create_array(uint8_t, arena, coloring.len);

        AvenArena temp_arena = *arena;
        GraphPlaneP3ColorCtx ctx = graph_plane_p3color_init(
            graph,
            p,
            q,
            &temp_arena
        );

        GraphPlaneP3ColorFrameOptional cur_frame =
            graph_plane_p3color_next_frame(&ctx);

        do {
            while (!graph_plane_p3color_frame_step(&ctx, &cur_frame.value)) {}
            cur_frame = graph_plane_p3color_next_frame(&ctx);
        } while (cur_frame.valid);

        for (uint32_t v = 0; v < coloring.len; v += 1) {
            int32_t v_mark = get(ctx.vertex_info, v).mark;
            assert(v_mark > 0 and v_mark <= 3);
            get(coloring, v) = (uint8_t)v_mark;
        }

        return coloring;
    }

    typedef enum {
        GRAPH_PLANE_P3COLOR_CASE_1_A = 0,
        GRAPH_PLANE_P3COLOR_CASE_1_B,
        GRAPH_PLANE_P3COLOR_CASE_2_A,
        GRAPH_PLANE_P3COLOR_CASE_2_B,
        GRAPH_PLANE_P3COLOR_CASE_2_C,
        GRAPH_PLANE_P3COLOR_CASE_2_D,
        GRAPH_PLANE_P3COLOR_CASE_2_E,
        GRAPH_PLANE_P3COLOR_CASE_2_F,
        GRAPH_PLANE_P3COLOR_CASE_3_A,
        GRAPH_PLANE_P3COLOR_CASE_3_B,
        GRAPH_PLANE_P3COLOR_CASE_3_C,
        GRAPH_PLANE_P3COLOR_CASE_MAX,
    } GraphPlaneP3ColorCase;

    static inline GraphPlaneP3ColorCase graph_plane_p3color_frame_case(
        GraphPlaneP3ColorCtx *ctx,
        GraphPlaneP3ColorFrame *frame
    ) {
        GraphPlaneP3ColorVertex u_info = get(ctx->vertex_info, frame->u);

        if (frame->edge_index == u_info.adj.len) {
            assert(frame->z == frame->u);

            if (frame->y == frame->u) {
                assert(frame->x == frame->u);
                return GRAPH_PLANE_P3COLOR_CASE_1_A;
            }

            return GRAPH_PLANE_P3COLOR_CASE_1_B;
        }

        uint32_t v_index = frame->u_nb_first + frame->edge_index;
        if (v_index >= u_info.adj.len) {
            v_index -= u_info.adj.len;
        }

        uint32_t v = graph_nb(ctx->nb, u_info.adj, v_index);
        GraphPlaneP3ColorVertex v_info = get(ctx->vertex_info, v);

        if (frame->above_path) {
            if (v_info.mark <= 0) {
                if (frame->last_colored) {
                    return GRAPH_PLANE_P3COLOR_CASE_3_A;
                } else {
                    return GRAPH_PLANE_P3COLOR_CASE_3_B;
                }
            } else {
                if (frame->z != frame->u) {
                    return GRAPH_PLANE_P3COLOR_CASE_3_C;
                }
            }
        } else if (v != frame->x) {
            if (v_info.mark > 0) {
                if (v_info.mark == (int32_t)frame->p_color) {
                    return GRAPH_PLANE_P3COLOR_CASE_2_A;
                }
                if (frame->x != frame->u) {
                    return GRAPH_PLANE_P3COLOR_CASE_2_B;
                }
            } else if (v_info.mark == frame->face_mark) {
                return GRAPH_PLANE_P3COLOR_CASE_2_C;
            } else {
                if (frame->x == frame->u) {
                    return GRAPH_PLANE_P3COLOR_CASE_2_D;
                }
                return GRAPH_PLANE_P3COLOR_CASE_2_E;
            }
        }

        return GRAPH_PLANE_P3COLOR_CASE_2_F;
    }
#endif // GRAPH_PLANE_P3COLOR_H
