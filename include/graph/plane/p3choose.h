#ifndef GRAPH_PLANE_P3CHOOSE_H
#define GRAPH_PLANE_P3CHOOSE_H

#include <aven.h>
#include <aven/arena.h>

#include "../../graph.h"

typedef struct {
    uint8_t len;
    uint8_t ptr[3];
} GraphPlaneP3ChooseList;
typedef Slice(GraphPlaneP3ChooseList) GraphPlaneP3ChooseListProp;

typedef struct {
    uint32_t first;
    uint32_t last;
} GraphPlaneP3ChooseNeighbors;

typedef struct {
    GraphPlaneP3ChooseNeighbors nb;
    uint32_t mark;
} GraphPlaneP3ChooseVertexLoc;

typedef struct {
    GraphAdj adj;
    GraphPlaneP3ChooseVertexLoc loc;
    GraphPlaneP3ChooseList colors;
} GraphPlaneP3ChooseVertex;

typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t z;
    GraphPlaneP3ChooseVertexLoc x_loc;
    GraphPlaneP3ChooseVertexLoc y_loc;
    GraphPlaneP3ChooseVertexLoc z_loc;
} GraphPlaneP3ChooseFrame;
typedef Optional(GraphPlaneP3ChooseFrame) GraphPlaneP3ChooseFrameOptional;
typedef Slice(GraphPlaneP3ChooseFrameOptional)
    GraphPlaneP3ChooseFrameOptionalSlice;

typedef struct {
    GraphAugNbSlice nb;
    Slice(GraphPlaneP3ChooseVertex) vertex_info;
    Slice(uint32_t) marks;
    List(GraphPlaneP3ChooseFrame) frames;
    uint32_t next_mark;
} GraphPlaneP3ChooseCtx;

static inline GraphPlaneP3ChooseCtx graph_plane_p3choose_init(
    GraphAug graph,
    GraphPlaneP3ChooseListProp color_lists,
    GraphSubset cwise_outer_face,
    AvenArena *arena
) {
    GraphPlaneP3ChooseCtx ctx = {
        .nb = graph.nb,
        .vertex_info = { .len = graph.adj.len },
        // Each new unique results from a different edge of the graph:
        .marks = { .len = (3 * graph.adj.len - 6) + 1 },
        // A new frame only occurs when splitting across an edge
        .frames = { .cap = 3 * graph.adj.len - 6 },
        .next_mark = 1,
    };

    ctx.frames.ptr = aven_arena_create_array(
        GraphPlaneP3ChooseFrame,
        arena,
        ctx.frames.cap
    );
    ctx.vertex_info.ptr = aven_arena_create_array(
        GraphPlaneP3ChooseVertex,
        arena,
        ctx.vertex_info.len
    );
    ctx.marks.ptr = aven_arena_create_array(
        uint32_t,
        arena,
        ctx.marks.len
    );

    for (uint32_t v = 0; v < ctx.vertex_info.len; v += 1) {
        get(ctx.vertex_info, v) = (GraphPlaneP3ChooseVertex){
            .adj = get(graph.adj, v),
            .colors = get(color_lists, v),
        };
    }

    for (uint32_t i = 0; i < ctx.marks.len; i += 1) {
        get(ctx.marks, i) = i;
    }

    uint32_t face_mark = ctx.next_mark++;

    uint32_t u = get(cwise_outer_face, cwise_outer_face.len - 1);
    for (uint32_t i = 0; i < cwise_outer_face.len; i += 1) {
        uint32_t v = get(cwise_outer_face, i);
        GraphAdj v_adj = get(ctx.vertex_info, v).adj;

        uint32_t vu_index = graph_aug_nb_index(graph.nb, v_adj, u);
        uint32_t uv_index = graph_aug_nb(graph.nb, v_adj, vu_index).back_index;

        get(ctx.vertex_info, v).loc.nb.first = vu_index;
        get(ctx.vertex_info, u).loc.nb.last = uv_index;

        get(ctx.vertex_info, v).loc.mark = face_mark;

        u = v;
    }

    uint32_t xyv = get(cwise_outer_face, 0);
    GraphPlaneP3ChooseVertexLoc *xyv_loc = &get(ctx.vertex_info, xyv).loc;
    xyv_loc->mark = ctx.next_mark++;

    GraphPlaneP3ChooseList *xyv_colors = &get(ctx.vertex_info, xyv).colors;
    assert(xyv_colors->len > 0);
    xyv_colors->len = 1;

    list_push(ctx.frames) = (GraphPlaneP3ChooseFrame){
        .x = xyv,
        .y = xyv,
        .z = xyv,
        .x_loc = *xyv_loc,
    };

    return ctx;
}

static inline GraphPlaneP3ChooseVertexLoc *graph_plane_p3choose_vloc(
    GraphPlaneP3ChooseCtx *ctx,
    GraphPlaneP3ChooseFrame *frame,
    uint32_t v
) {
    if (v == frame->x) {
        return &frame->x_loc;
    }

    if (v == frame->y) {
        return &frame->y_loc;
    }

    if (v == frame->z) {
        return &frame->z_loc;
    }

    return &get(ctx->vertex_info, v).loc;
}

static bool graph_plane_p3choose_has_color(
    GraphPlaneP3ChooseList *list,
    uint32_t color
) {
    assert(list->len > 0);
    for (size_t i = 0; i < list->len; i += 1) {
        if (get(*list, i) == color) {
            return true;
        }
    }

    return false;
}

static void graph_plane_p3choose_remove_color(
    GraphPlaneP3ChooseList *list,
    uint32_t color
) {
    for (size_t i = 0; i < list->len; i += 1) {
        if (get(*list, i) == color) {
            assert(list->len > 1);
            get(*list, i) = get(*list, list->len - 1);
            list->len -= 1;
            break;
        }
    }
}

static void graph_plane_p3choose_color_differently(
    GraphPlaneP3ChooseList *list,
    uint32_t color
) {
    for (size_t i = 0; i < list->len; i += 1) {
        if (get(*list, i) != color) {
            get(*list, 0) = get(*list, i);
            list->len = 1;
            break;
        }
    }
    assert(get(*list, 0) != color);
}

static inline GraphPlaneP3ChooseFrameOptional graph_plane_p3choose_next_frame(
    GraphPlaneP3ChooseCtx *ctx
) {
    if (ctx->frames.len == 0) {
        return (GraphPlaneP3ChooseFrameOptional){ 0 };
    }

    return (GraphPlaneP3ChooseFrameOptional){
        .value = list_pop(ctx->frames),
        .valid = true
    };
}

static inline bool graph_plane_p3choose_frame_step(
    GraphPlaneP3ChooseCtx *ctx,
    GraphPlaneP3ChooseFrame *frame
) {
    GraphAdj z_adj = get(ctx->vertex_info, frame->z).adj;
    GraphPlaneP3ChooseVertexLoc *z_loc = graph_plane_p3choose_vloc(
        ctx,
        frame,
        frame->z
    );
    GraphPlaneP3ChooseList *z_colors = &get(
        ctx->vertex_info,
        frame->z
    ).colors;
    uint8_t z_color = get(*z_colors, 0);

    uint32_t zu_index = z_loc->nb.first;
    GraphAugNb zu = graph_aug_nb(ctx->nb, z_adj, zu_index);

    uint32_t u = zu.vertex;
    GraphPlaneP3ChooseVertexLoc *u_loc = graph_plane_p3choose_vloc(
        ctx,
        frame,
        u
    );

    if (zu_index == z_loc->nb.last) {
        if (frame->x == frame->y) {
            assert(frame->z == frame->x);
            graph_plane_p3choose_color_differently(
                &get(ctx->vertex_info, u).colors,
                z_color
            );
        }
        return true;
    }

    if (u == frame->y) {
        assert(frame->z == frame->x);

        GraphPlaneP3ChooseVertexLoc x_loc = frame->x_loc;
        frame->x_loc = frame->y_loc;
        frame->y_loc = x_loc;
        frame->y = frame->x;
        frame->x = u;
        frame->z = u;

        frame->x_loc.mark = ctx->next_mark++;
        frame->y_loc.mark = frame->x_loc.mark;

        return false;
    }

    GraphAdj u_adj = get(ctx->vertex_info, u).adj;
    u_loc->nb.last = graph_adj_prev(u_adj, u_loc->nb.last);
    z_loc->nb.first = graph_adj_next(z_adj, z_loc->nb.first);

    if (frame->z == frame->x) {
        graph_plane_p3choose_color_differently(
            &get(ctx->vertex_info, u).colors,
            z_color
        );

        if (frame->z == frame->y) {
            frame->y_loc = frame->x_loc;
        } else {
            frame->z_loc = frame->x_loc;
        }

        frame->x = u;
        frame->x_loc = *u_loc;
        frame->x_loc.mark = ctx->next_mark++;

        u_loc = graph_plane_p3choose_vloc(ctx, frame, u);
        z_loc = graph_plane_p3choose_vloc(ctx, frame, frame->z);
    }

    uint32_t zv_index = graph_adj_next(z_adj, zu_index);
    GraphAugNb zv = graph_aug_nb(ctx->nb, z_adj, zv_index);

    uint32_t v = zv.vertex;
    GraphAdj v_adj = get(ctx->vertex_info, v).adj;
    GraphPlaneP3ChooseVertexLoc *v_loc = graph_plane_p3choose_vloc(
        ctx,
        frame,
        v
    );
    GraphPlaneP3ChooseList *v_colors = &get(ctx->vertex_info, v).colors;

    if (v_loc->mark == 0) {
        *v_loc = (GraphPlaneP3ChooseVertexLoc){
            .mark = frame->x_loc.mark,
            .nb = {
                .first = graph_adj_next(v_adj, zv.back_index),
                .last = zv.back_index,
            },
        };
        graph_plane_p3choose_remove_color(v_colors, z_color);
    } else if (v_loc->mark == frame->x_loc.mark) {
        if (zv_index == z_loc->nb.last) {
            assert(frame->z == frame->y);
            assert(v == frame->x);
            v_loc->nb.first = graph_adj_next(v_adj, zv.back_index),
            v_loc->mark = ctx->next_mark++;
            frame->y = frame->x;
            frame->z = frame->x;
        } else {
            uint32_t new_mark = ctx->next_mark++;
            list_push(ctx->frames) = (GraphPlaneP3ChooseFrame){
                .x = v,
                .y = v,
                .z = v,
                .x_loc = {
                    .mark = new_mark,
                    .nb = {
                        .first = graph_adj_next(v_adj, zv.back_index),
                        .last = v_loc->nb.last,
                    },
                },
            };
            v_loc->nb.last = zv.back_index;
        }
    } else if (get(ctx->marks, v_loc->mark) == frame->y_loc.mark) {
        if (v_loc->nb.first != zv.back_index) {
            uint32_t new_mark = ctx->next_mark++;
            list_push(ctx->frames) = (GraphPlaneP3ChooseFrame){
                .x = v,
                .y = frame->z,
                .z = v,
                .x_loc = {
                    .mark = new_mark,
                    .nb = {
                        .first = v_loc->nb.first,
                        .last = zv.back_index,
                    },
                },
                .y_loc = {
                    .mark = new_mark,
                    .nb = {
                        .first = zv_index,
                        .last = z_loc->nb.last,
                    },
                },
            };
        }

        v_loc->nb.first = graph_adj_next(v_adj, zv.back_index);

        if (graph_plane_p3choose_has_color(v_colors, z_color)) {
            get(*v_colors, 0) = z_color;
            v_colors->len = 1;

            frame->z = v;
            frame->z_loc = *v_loc;
        } else {
            get(ctx->marks, frame->x_loc.mark) = frame->y_loc.mark;
            frame->z = frame->x;
        }
    } else {
        graph_plane_p3choose_color_differently(v_colors, z_color);
        if (v_loc->nb.first != zv.back_index) {
            // Another possible call ordering:
            // list_push(ctx->frames) = (GraphPlaneP3ChooseFrame){
            //     .x = frame->x,
            //     .y = v,
            //     .z = frame->x,
            //     .x_loc = frame->x_loc,
            //     .y_loc = {
            //         .mark = frame->x_loc.mark,
            //         .nb = {
            //             .first = graph_aug_adj_next(v_adj, zv.back_index),
            //             .last = v_loc->nb.last,
            //         },
            //     },
            // };

            // v_loc->nb.last = zv.back_index;
            // frame->x = v;
            // frame->x_loc = *v_loc;
            // frame->x_loc.mark = ctx->next_mark++;

            list_push(ctx->frames) = (GraphPlaneP3ChooseFrame){
                .x = v,
                .y = frame->y,
                .z = frame->z,
                .x_loc = {
                    .mark = ctx->next_mark++,
                    .nb = {
                        .first = v_loc->nb.first,
                        .last = zv.back_index,
                    },
                },
                .y_loc = frame->y_loc,
                .z_loc = frame->z_loc,
            };

            v_loc->mark = frame->x_loc.mark;
            v_loc->nb.first = graph_adj_next(v_adj, zv.back_index);

            frame->z = frame->x;
            frame->y = v;
            frame->y_loc = *v_loc;
        } else {
            assert(frame->z == frame->y);
            v_loc->nb.first = graph_adj_next(v_adj, zv.back_index),
            v_loc->mark = frame->x_loc.mark;
            frame->y = v;
            frame->y_loc = *v_loc;

            frame->z = frame->x;
        }
    }

    return false;
}

static inline GraphPropUint8 graph_plane_p3choose(
    GraphAug aug_graph,
    GraphPlaneP3ChooseListProp color_lists,
    GraphSubset outer_face,
    AvenArena *arena
) {
    GraphPropUint8 coloring = { .len = aug_graph.adj.len };
    coloring.ptr = aven_arena_create_array(uint8_t, arena, coloring.len);

    AvenArena temp_arena = *arena;
    GraphPlaneP3ChooseCtx ctx = graph_plane_p3choose_init(
        aug_graph,
        color_lists,
        outer_face,
        &temp_arena
    );

    GraphPlaneP3ChooseFrameOptional frame =
        graph_plane_p3choose_next_frame(&ctx);

    do {
        while (!graph_plane_p3choose_frame_step(&ctx, &frame.value)) {}
        frame = graph_plane_p3choose_next_frame(&ctx);
    } while (frame.valid);

    for (uint32_t v = 0; v < coloring.len; v += 1) {
        assert(get(ctx.vertex_info, v).colors.len == 1);
        get(coloring, v) = get(get(ctx.vertex_info, v).colors, 0);
    }

    return coloring;
}

typedef enum {
    GRAPH_PLANE_P3CHOOSE_CASE_BASE = 0,
    GRAPH_PLANE_P3CHOOSE_CASE_1,
    GRAPH_PLANE_P3CHOOSE_CASE_2,
    GRAPH_PLANE_P3CHOOSE_CASE_3_1,
    GRAPH_PLANE_P3CHOOSE_CASE_3_2_1_A,
    GRAPH_PLANE_P3CHOOSE_CASE_3_2_1_B,
    GRAPH_PLANE_P3CHOOSE_CASE_3_2_2_A,
    GRAPH_PLANE_P3CHOOSE_CASE_3_2_2_B,
    GRAPH_PLANE_P3CHOOSE_CASE_3_2_3_1_A,
    GRAPH_PLANE_P3CHOOSE_CASE_3_2_3_1_B,
    GRAPH_PLANE_P3CHOOSE_CASE_3_2_3_2_A,
    GRAPH_PLANE_P3CHOOSE_CASE_3_2_3_2_B,
} GraphPlaneP3ChooseCase;

static inline GraphPlaneP3ChooseCase graph_plane_p3choose_frame_case(
    GraphPlaneP3ChooseCtx *ctx,
    GraphPlaneP3ChooseFrame *frame
) {
    GraphAdj z_adj = get(ctx->vertex_info, frame->z).adj;
    GraphPlaneP3ChooseVertexLoc *z_loc = graph_plane_p3choose_vloc(
        ctx,
        frame,
        frame->z
    );
    GraphPlaneP3ChooseList *z_colors = &get(
        ctx->vertex_info,
        frame->z
    ).colors;
    uint32_t z_color = get(*z_colors, 0);

    uint32_t zu_index = z_loc->nb.first;
    GraphAugNb zu = graph_aug_nb(ctx->nb, z_adj, zu_index);

    uint32_t u = zu.vertex;

    if (zu_index == z_loc->nb.last) {
        return GRAPH_PLANE_P3CHOOSE_CASE_BASE;
    }

    if (u == frame->y) {
        return GRAPH_PLANE_P3CHOOSE_CASE_1;
    }

    if (frame->z == frame->x) {
        return GRAPH_PLANE_P3CHOOSE_CASE_2;
    }

    uint32_t zv_index = graph_adj_next(z_adj, zu_index);
    GraphAugNb zv = graph_aug_nb(ctx->nb, z_adj, zv_index);

    uint32_t v = zv.vertex;
    GraphPlaneP3ChooseVertexLoc *v_loc = graph_plane_p3choose_vloc(
        ctx,
        frame,
        v
    );
    GraphPlaneP3ChooseList *v_colors = &get(
        ctx->vertex_info,
        v
    ).colors;

    if (v_loc->mark == 0) {
        return GRAPH_PLANE_P3CHOOSE_CASE_3_1;
    } else if (v_loc->mark == frame->x_loc.mark) {
        if (
            zv.back_index == v_loc->nb.first or
            zv.back_index == v_loc->nb.last
        ) {
            return GRAPH_PLANE_P3CHOOSE_CASE_3_2_1_A;
        } else {
            return GRAPH_PLANE_P3CHOOSE_CASE_3_2_1_B;
        }
    } else if (get(ctx->marks, v_loc->mark) == frame->y_loc.mark) {
        if (graph_plane_p3choose_has_color(v_colors, z_color)) {
            if (
                zv.back_index == v_loc->nb.first or
                zv.back_index == v_loc->nb.last
            ) {
                return GRAPH_PLANE_P3CHOOSE_CASE_3_2_3_1_A;
            } else {
                return GRAPH_PLANE_P3CHOOSE_CASE_3_2_3_1_B;
            }
        } else {
            if (
                zv.back_index == v_loc->nb.first or
                zv.back_index == v_loc->nb.last
            ) {
                return GRAPH_PLANE_P3CHOOSE_CASE_3_2_3_2_A;
            } else {
                return GRAPH_PLANE_P3CHOOSE_CASE_3_2_3_2_B;
            }        }
    } else {
        if (
            zv.back_index == v_loc->nb.first or
            zv.back_index == v_loc->nb.last
        ) {
            return GRAPH_PLANE_P3CHOOSE_CASE_3_2_2_A;
        } else {
            return GRAPH_PLANE_P3CHOOSE_CASE_3_2_2_B;
        }
    }

    assert(false);
    return GRAPH_PLANE_P3CHOOSE_CASE_BASE;
}

#endif // GRAPH_PLANE_P3CHOOSE_H
