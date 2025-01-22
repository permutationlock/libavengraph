#ifndef AVEN_GRAPH_PLANE_HARTMAN_H
#define AVEN_GRAPH_PLANE_HARTMAN_H

#include <aven.h>
#include <aven/arena.h>

#include "../../graph.h"

typedef struct {
    uint8_t len;
    uint8_t ptr[3];
} AvenGraphPlaneHartmanList;
typedef Slice(AvenGraphPlaneHartmanList) AvenGraphPlaneHartmanListProp;

typedef struct {
    uint32_t first;
    uint32_t last;
} AvenGraphPlaneHartmanNeighbors;

typedef struct {
    AvenGraphPlaneHartmanNeighbors nb;
    uint32_t mark;
} AvenGraphPlaneHartmanVertexLoc;

typedef struct {
    AvenGraphAugAdjList adj;
    AvenGraphPlaneHartmanVertexLoc loc;
    AvenGraphPlaneHartmanList colors;
} AvenGraphPlaneHartmanVertex;

typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t z;
    AvenGraphPlaneHartmanVertexLoc x_loc;
    AvenGraphPlaneHartmanVertexLoc y_loc;
    AvenGraphPlaneHartmanVertexLoc z_loc;
} AvenGraphPlaneHartmanFrame;
typedef Optional(AvenGraphPlaneHartmanFrame) AvenGraphPlaneHartmanFrameOptional;

typedef struct {
    Slice(AvenGraphPlaneHartmanVertex) vertex_info;
    Slice(uint32_t) marks;
    List(AvenGraphPlaneHartmanFrame) frames;
    uint32_t next_mark;
} AvenGraphPlaneHartmanCtx;

static inline AvenGraphPlaneHartmanCtx aven_graph_plane_hartman_init(
    AvenGraphAug graph,
    AvenGraphPlaneHartmanListProp color_lists,
    AvenGraphSubset cwise_outer_face,
    AvenArena *arena
) {
    AvenGraphPlaneHartmanCtx ctx = {
        .vertex_info = { .len = graph.len },
        // Each vertex receives a new unique mark at most 4 times:
        //  - when added to the outer face
        //  - when assigned to be the new x
        //  - when assigned to be the new y
        //  - when swapped x to y or y to x (removed afterwards so happens once)
        .marks = { .len = 4 * graph.len },
        // A new frame only occurs when splitting across an edge
        .frames = { .cap = 3 * graph.len - 6 },
        .next_mark = 1,
    };

    ctx.vertex_info.ptr = aven_arena_create_array(
        AvenGraphPlaneHartmanVertex,
        arena,
        ctx.vertex_info.len
    );
    ctx.marks.ptr = aven_arena_create_array(
        uint32_t,
        arena,
        ctx.marks.len
    );
    ctx.frames.ptr = aven_arena_create_array(
        AvenGraphPlaneHartmanFrame,
        arena,
        ctx.frames.cap
    );

    for (uint32_t v = 0; v < ctx.vertex_info.len; v += 1) {
        get(ctx.vertex_info, v) = (AvenGraphPlaneHartmanVertex){
            .adj = get(graph, v),
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
        AvenGraphAugAdjList v_adj = get(ctx.vertex_info, v).adj;
        
        uint32_t vu_index = aven_graph_aug_adj_neighbor_index(v_adj, u);
        uint32_t uv_index = get(v_adj, vu_index).back_index;
        
        get(ctx.vertex_info, v).loc.nb.first = vu_index;
        get(ctx.vertex_info, u).loc.nb.last = uv_index;

        get(ctx.vertex_info, v).loc.mark = face_mark;

        u = v;
    }

    uint32_t xyv = get(cwise_outer_face, 0);
    AvenGraphPlaneHartmanVertexLoc *xyv_loc = &get(ctx.vertex_info, xyv).loc;
    xyv_loc->mark = ctx.next_mark++;

    AvenGraphPlaneHartmanList *xyv_colors = &get(ctx.vertex_info, xyv).colors;
    assert(xyv_colors->len > 0);
    xyv_colors->len = 1;

    list_push(ctx.frames) = (AvenGraphPlaneHartmanFrame){
        .x = xyv,
        .y = xyv,
        .z = xyv,
        .x_loc = *xyv_loc,
    };

    return ctx;
}

static inline AvenGraphPlaneHartmanVertexLoc *aven_graph_plane_hartman_vloc(
    AvenGraphPlaneHartmanCtx *ctx,
    AvenGraphPlaneHartmanFrame *frame,
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

static bool aven_graph_plane_hartman_has_color(
    AvenGraphPlaneHartmanList *list,
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

static void aven_graph_plane_hartman_remove_color(
    AvenGraphPlaneHartmanList *list,
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

static void aven_graph_plane_hartman_color_differently(
    AvenGraphPlaneHartmanList *list,
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

static inline AvenGraphPlaneHartmanFrameOptional aven_graph_plane_hartman_next_frame(
    AvenGraphPlaneHartmanCtx *ctx
) {
    if (ctx->frames.len == 0) {
        return (AvenGraphPlaneHartmanFrameOptional){ 0 };
    }

    return (AvenGraphPlaneHartmanFrameOptional){
        .value = list_pop(ctx->frames),
        .valid = true
    }; 
}

static inline bool aven_graph_plane_hartman_frame_step(
    AvenGraphPlaneHartmanCtx *ctx,
    AvenGraphPlaneHartmanFrame *frame
) {
    AvenGraphAugAdjList z_adj = get(ctx->vertex_info, frame->z).adj;
    AvenGraphPlaneHartmanVertexLoc *z_loc = aven_graph_plane_hartman_vloc(
        ctx,
        frame,
        frame->z
    );
    AvenGraphPlaneHartmanList *z_colors = &get(
        ctx->vertex_info,
        frame->z
    ).colors;
    uint8_t z_color = get(*z_colors, 0);

    uint32_t zu_index = z_loc->nb.first;
    AvenGraphAugAdjListNode zu = get(z_adj, zu_index);

    uint32_t u = zu.vertex;
    AvenGraphPlaneHartmanVertexLoc *u_loc = aven_graph_plane_hartman_vloc(
        ctx,
        frame,
        u
    );

    if (zu_index == z_loc->nb.last) {
        if (frame->x == frame->y) {
            assert(frame->z == frame->x);
            aven_graph_plane_hartman_color_differently(
                &get(ctx->vertex_info, u).colors,
                z_color
            );
        }
        return true;
    }

    if (u == frame->y) {
        assert(frame->z == frame->x);

        AvenGraphPlaneHartmanVertexLoc x_loc = frame->x_loc;
        frame->x_loc = frame->y_loc;
        frame->y_loc = x_loc;
        frame->y = frame->x;
        frame->x = u;

        if (get(get(ctx->vertex_info, u).colors, 0) == z_color) {
            frame->z = u;
        }

        frame->x_loc.mark = ctx->next_mark++;
        frame->y_loc.mark = ctx->next_mark++;

        return false;
    }

    AvenGraphAugAdjList u_adj = get(ctx->vertex_info, u).adj;
    u_loc->nb.last = aven_graph_aug_adj_prev(u_adj, u_loc->nb.last);
    z_loc->nb.first = aven_graph_aug_adj_next(z_adj, z_loc->nb.first);

    if (frame->z == frame->x) {
        aven_graph_plane_hartman_color_differently(
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

        u_loc = aven_graph_plane_hartman_vloc(ctx, frame, u);
        z_loc = aven_graph_plane_hartman_vloc(ctx, frame, frame->z);
    }

    uint32_t zv_index = aven_graph_aug_adj_next(z_adj, zu_index);
    AvenGraphAugAdjListNode zv = get(z_adj, zv_index);

    uint32_t v = zv.vertex;
    AvenGraphAugAdjList v_adj = get(ctx->vertex_info, v).adj;
    AvenGraphPlaneHartmanVertexLoc *v_loc = aven_graph_plane_hartman_vloc(
        ctx,
        frame,
        v
    );
    AvenGraphPlaneHartmanList *v_colors = &get(ctx->vertex_info, v).colors;

    if (v_loc->mark == 0) {
        *v_loc = (AvenGraphPlaneHartmanVertexLoc){
            .mark = frame->x_loc.mark,
            .nb = {
                .first = aven_graph_aug_adj_next(v_adj, zv.back_index),
                .last = zv.back_index,
            },
        };
        aven_graph_plane_hartman_remove_color(v_colors, z_color);
    } else if (v_loc->mark == frame->x_loc.mark) {
        if (zv_index == z_loc->nb.last) {
            assert(frame->z == frame->y);
            assert(v == frame->x);
            v_loc->nb.first = aven_graph_aug_adj_next(v_adj, zv.back_index),
            v_loc->mark = ctx->next_mark++;
            frame->y = frame->x;
            frame->z = frame->x;
        } else {
            uint32_t new_mark = ctx->next_mark++;
            list_push(ctx->frames) = (AvenGraphPlaneHartmanFrame){
                .x = v,
                .y = v,
                .z = v,
                .x_loc = {
                    .mark = new_mark,
                    .nb = {
                        .first = aven_graph_aug_adj_next(v_adj, zv.back_index),
                        .last = v_loc->nb.last,
                    },
                },
            };
            v_loc->nb.last = zv.back_index;
        }
    } else if (get(ctx->marks, v_loc->mark) == frame->y_loc.mark) {
        if (v_loc->nb.first != zv.back_index) {
            uint32_t new_mark = ctx->next_mark++;
            list_push(ctx->frames) = (AvenGraphPlaneHartmanFrame){
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

        v_loc->nb.first = aven_graph_aug_adj_next(v_adj, zv.back_index);

        if (aven_graph_plane_hartman_has_color(v_colors, z_color)) {
            get(*v_colors, 0) = z_color;
            v_colors->len = 1;

            frame->z = v;
            frame->z_loc = *v_loc;
        } else {
            get(ctx->marks, frame->x_loc.mark) = frame->y_loc.mark;
            frame->z = frame->x;
        }
    } else {
        aven_graph_plane_hartman_color_differently(v_colors, z_color);
        if (v_loc->nb.first != zv.back_index) {
            // Another possible call ordering:
            // list_push(ctx->frames) = (AvenGraphPlaneHartmanFrame){
            //     .x = frame->x,
            //     .y = v,
            //     .z = frame->x,
            //     .x_loc = frame->x_loc,
            //     .y_loc = {
            //         .mark = frame->x_loc.mark,
            //         .nb = {
            //             .first = aven_graph_aug_adj_next(v_adj, zv.back_index),
            //             .last = v_loc->nb.last,
            //         },
            //     },
            // };

            // v_loc->nb.last = zv.back_index;
            // frame->x = v;
            // frame->x_loc = *v_loc;
            // frame->x_loc.mark = ctx->next_mark++;

            list_push(ctx->frames) = (AvenGraphPlaneHartmanFrame){
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
            v_loc->nb.first = aven_graph_aug_adj_next(v_adj, zv.back_index);

            frame->z = frame->x;
            frame->y = v;
            frame->y_loc = *v_loc;
        } else {
            assert(frame->z == frame->y);
            v_loc->nb.first = aven_graph_aug_adj_next(v_adj, zv.back_index),
            v_loc->mark = frame->x_loc.mark;
            frame->y = v;
            frame->y_loc = *v_loc;

            frame->z = frame->x;
        }
    }

    return false;
}

static inline AvenGraphPropUint8 aven_graph_plane_hartman(
    AvenGraphAug aug_graph,
    AvenGraphPlaneHartmanListProp color_lists,
    AvenGraphSubset outer_face,
    AvenArena *arena
) {
    AvenGraphPropUint8 coloring = { .len = aug_graph.len };
    coloring.ptr = aven_arena_create_array(uint8_t, arena, coloring.len);

    AvenArena temp_arena = *arena;
    AvenGraphPlaneHartmanCtx ctx = aven_graph_plane_hartman_init(
        aug_graph,
        color_lists,
        outer_face,
        &temp_arena
    );

    AvenGraphPlaneHartmanFrameOptional frame =
        aven_graph_plane_hartman_next_frame(&ctx);

    do {
        while (!aven_graph_plane_hartman_frame_step(&ctx, &frame.value)) {}
        frame = aven_graph_plane_hartman_next_frame(&ctx);
    } while (frame.valid);

    for (uint32_t v = 0; v < coloring.len; v += 1) {
        assert(get(ctx.vertex_info, v).colors.len == 1);
        get(coloring, v) = get(get(ctx.vertex_info, v).colors, 0);
    }

    return coloring;
}

typedef enum {
    AVEN_GRAPH_PLANE_HARTMAN_CASE_BASE = 0,
    AVEN_GRAPH_PLANE_HARTMAN_CASE_1,
    AVEN_GRAPH_PLANE_HARTMAN_CASE_2,
    AVEN_GRAPH_PLANE_HARTMAN_CASE_3_1,
    AVEN_GRAPH_PLANE_HARTMAN_CASE_3_2_1_A,
    AVEN_GRAPH_PLANE_HARTMAN_CASE_3_2_1_B,
    AVEN_GRAPH_PLANE_HARTMAN_CASE_3_2_2_A,
    AVEN_GRAPH_PLANE_HARTMAN_CASE_3_2_2_B,
    AVEN_GRAPH_PLANE_HARTMAN_CASE_3_2_3_1_A,
    AVEN_GRAPH_PLANE_HARTMAN_CASE_3_2_3_1_B,
    AVEN_GRAPH_PLANE_HARTMAN_CASE_3_2_3_2_A,
    AVEN_GRAPH_PLANE_HARTMAN_CASE_3_2_3_2_B,
} AvenGraphPlaneHartmanCase;

static inline AvenGraphPlaneHartmanCase aven_graph_plane_hartman_frame_case(
    AvenGraphPlaneHartmanCtx *ctx,
    AvenGraphPlaneHartmanFrame *frame
) {
    AvenGraphAugAdjList z_adj = get(ctx->vertex_info, frame->z).adj;
    AvenGraphPlaneHartmanVertexLoc *z_loc = aven_graph_plane_hartman_vloc(
        ctx,
        frame,
        frame->z
    );
    AvenGraphPlaneHartmanList *z_colors = &get(
        ctx->vertex_info,
        frame->z
    ).colors;
    uint32_t z_color = get(*z_colors, 0);

    uint32_t zu_index = z_loc->nb.first;
    AvenGraphAugAdjListNode zu = get(z_adj, zu_index);

    uint32_t u = zu.vertex;

    if (zu_index == z_loc->nb.last) {
        return AVEN_GRAPH_PLANE_HARTMAN_CASE_BASE;
    }

    if (u == frame->y) {
        return AVEN_GRAPH_PLANE_HARTMAN_CASE_1;
    }

    if (frame->z == frame->x) {
        return AVEN_GRAPH_PLANE_HARTMAN_CASE_2;
    }

    uint32_t zv_index = aven_graph_aug_adj_next(z_adj, zu_index);
    AvenGraphAugAdjListNode zv = get(z_adj, zv_index);

    uint32_t v = zv.vertex;
    AvenGraphPlaneHartmanVertexLoc *v_loc = aven_graph_plane_hartman_vloc(
        ctx,
        frame,
        v
    );
    AvenGraphPlaneHartmanList *v_colors = &get(
        ctx->vertex_info,
        v
    ).colors;

    if (v_loc->mark == 0) {
        return AVEN_GRAPH_PLANE_HARTMAN_CASE_3_1;
    } else if (v_loc->mark == frame->x_loc.mark) {
        if (
            zv.back_index == v_loc->nb.first or
            zv.back_index == v_loc->nb.last
        ) {
            return AVEN_GRAPH_PLANE_HARTMAN_CASE_3_2_1_A;
        } else {
            return AVEN_GRAPH_PLANE_HARTMAN_CASE_3_2_1_B;
        }
    } else if (get(ctx->marks, v_loc->mark) == frame->y_loc.mark) {
        if (aven_graph_plane_hartman_has_color(v_colors, z_color)) {
            if (
                zv.back_index == v_loc->nb.first or
                zv.back_index == v_loc->nb.last
            ) {
                return AVEN_GRAPH_PLANE_HARTMAN_CASE_3_2_3_1_A;
            } else {
                return AVEN_GRAPH_PLANE_HARTMAN_CASE_3_2_3_1_B;
            }
        } else {
            if (
                zv.back_index == v_loc->nb.first or
                zv.back_index == v_loc->nb.last
            ) {
                return AVEN_GRAPH_PLANE_HARTMAN_CASE_3_2_3_2_A;
            } else {
                return AVEN_GRAPH_PLANE_HARTMAN_CASE_3_2_3_2_B;
            }        }
    } else {
        if (
            zv.back_index == v_loc->nb.first or
            zv.back_index == v_loc->nb.last
        ) {
            return AVEN_GRAPH_PLANE_HARTMAN_CASE_3_2_2_A;
        } else {
            return AVEN_GRAPH_PLANE_HARTMAN_CASE_3_2_2_B;
        }
    }

    assert(false);
    return AVEN_GRAPH_PLANE_HARTMAN_CASE_BASE;
}

#endif // AVEN_GRAPH_PLANE_HARTMAN_H
