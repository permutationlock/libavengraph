#ifndef AVEN_GRAPH_PLANE_HARTMAN_H
#define AVEN_GRAPH_PLANE_HARTMAN_H

#include <aven.h>
#include <aven/arena.h>

#include "../../graph.h"

typedef struct {
    uint32_t len;
    uint32_t ptr[3];
} AvenGraphPlaneHartmanList;
typedef Slice(AvenGraphPlaneHartmanList) AvenGraphPlaneHartmanListProp;

typedef struct {
    uint32_t first;
    uint32_t last;
} AvenGraphPlaneHartmanNeighbors;

typedef struct {
    AvenGraphPlaneHartmanNeighbors nb;
    uint32_t mark;
} AvenGraphPlaneHartmanVertex;

typedef struct {
    uint32_t x;
    uint32_t y;
    uint32_t z;
    AvenGraphPlaneHartmanVertex x_info;
    AvenGraphPlaneHartmanVertex y_info;
    AvenGraphPlaneHartmanVertex z_info;
} AvenGraphPlaneHartmanFrame;
typedef Optional(AvenGraphPlaneHartmanFrame) AvenGraphPlaneHartmanFrameOptional;

typedef struct {
    AvenGraphAug graph;
    AvenGraphPlaneHartmanListProp color_lists;
    Slice(AvenGraphPlaneHartmanVertex) vertex_info;
    Slice(uint32_t) marks;
    List(AvenGraphPlaneHartmanFrame) frames;
    uint32_t next_mark;
} AvenGraphPlaneHartmanCtx;

static inline AvenGraphPlaneHartmanCtx aven_graph_plane_hartman_init(
    AvenGraphPlaneHartmanListProp color_lists,
    AvenGraphAug graph,
    AvenGraphSubset cwise_outer_face,
    AvenArena *arena
) {
    AvenGraphPlaneHartmanCtx ctx = {
        .graph = graph,
        .color_lists = color_lists,
        .vertex_info = { .len = graph.len },
        .marks = { .len = 4 * graph.len - 6 },
        .frames = { .cap = 2 * graph.len - 4 },
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
        get(ctx.vertex_info, v) = (AvenGraphPlaneHartmanVertex){ 0 };
    }

    for (uint32_t i = 0; i < ctx.marks.len; i += 1) {
        get(ctx.marks, i) = i;
    }

    uint32_t face_mark = ctx.next_mark++;

    uint32_t u = get(cwise_outer_face, cwise_outer_face.len - 1);
    for (uint32_t i = 0; i < cwise_outer_face.len; i += 1) {
        uint32_t v = get(cwise_outer_face, i);
        AvenGraphAugAdjList v_adj = get(ctx.graph, v);
        
        uint32_t vu_index = aven_graph_aug_neighbor_index(ctx.graph, v, u);
        uint32_t uv_index = get(v_adj, vu_index).back_index;
        
        get(ctx.vertex_info, v).nb.first = vu_index;
        get(ctx.vertex_info, u).nb.last = uv_index;

        get(ctx.vertex_info, v).mark = face_mark;

        u = v;
    }

    uint32_t xyv = get(cwise_outer_face, 0);
    AvenGraphPlaneHartmanVertex *xyv_info = &get(ctx.vertex_info, xyv);
    xyv_info->mark = ctx.next_mark++;

    AvenGraphPlaneHartmanList *xyv_colors = &get(ctx.color_lists, xyv);
    assert(xyv_colors->len > 0);
    xyv_colors->len = 1;

    list_push(ctx.frames) = (AvenGraphPlaneHartmanFrame){
        .x = xyv,
        .y = xyv,
        .z = xyv,
        .x_info = *xyv_info,
    };

    return ctx;
}

static inline AvenGraphPlaneHartmanVertex *aven_graph_plane_hartman_vinfo(
    AvenGraphPlaneHartmanCtx *ctx,
    AvenGraphPlaneHartmanFrame *frame,
    uint32_t v
) {
    if (v == frame->x) {
        return &frame->x_info;
    }

    if (v == frame->y) {
        return &frame->y_info;
    }

    if (v == frame->z) {
        return &frame->z_info;
    }

    return &get(ctx->vertex_info, v);
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
    AvenGraphAugAdjList z_adj = get(ctx->graph, frame->z);
    AvenGraphPlaneHartmanVertex *z_info = aven_graph_plane_hartman_vinfo(
        ctx,
        frame,
        frame->z
    );
    AvenGraphPlaneHartmanList *z_colors = &get(ctx->color_lists, frame->z);
    uint32_t z_color = get(*z_colors, 0);

    uint32_t zu_index = z_info->nb.first;
    AvenGraphAugAdjListNode zu = get(z_adj, zu_index);

    uint32_t u = zu.vertex;
    AvenGraphPlaneHartmanVertex *u_info = aven_graph_plane_hartman_vinfo(
        ctx,
        frame,
        u
    );

    if (zu_index == z_info->nb.last) {
        if (frame->x == frame->y) {
            assert(frame->z == frame->x);
            aven_graph_plane_hartman_color_differently(
                &get(ctx->color_lists, u),
                z_color
            );
        }
        return true;
    }

    if (u == frame->y) {
        assert(frame->z == frame->x);

        AvenGraphPlaneHartmanVertex x_info = frame->x_info;
        frame->x_info = frame->y_info;
        frame->y_info = x_info;
        frame->y = frame->x;
        frame->x = u;

        //if (
        //    get(get(ctx->color_lists, frame->x), 0) ==
        //    get(get(ctx->color_lists, frame->y), 0)
        //) {
        //    frame->z = u;
        //}

        frame->x_info.mark = ctx->next_mark++;
        frame->y_info.mark = ctx->next_mark++;

        return false;
    }

    AvenGraphAugAdjList u_adj = get(ctx->graph, u);
    u_info->nb.last = aven_graph_aug_adj_prev(u_adj, u_info->nb.last);
    z_info->nb.first = aven_graph_aug_adj_next(z_adj, z_info->nb.first);

    if (frame->z == frame->x) {
        aven_graph_plane_hartman_color_differently(
            &get(ctx->color_lists, u),
            z_color
        );

        if (frame->z == frame->y) {
            frame->y_info = frame->x_info;
        } else {
            frame->z_info = frame->x_info;
        }

        frame->x = u;
        frame->x_info = *u_info;
        frame->x_info.mark = ctx->next_mark++;

        u_info = aven_graph_plane_hartman_vinfo(ctx, frame, u);
        z_info = aven_graph_plane_hartman_vinfo(ctx, frame, frame->z);
    }

    uint32_t zv_index = aven_graph_aug_adj_next(z_adj, zu_index);
    AvenGraphAugAdjListNode zv = get(z_adj, zv_index);

    uint32_t v = zv.vertex;
    AvenGraphAugAdjList v_adj = get(ctx->graph, v);
    AvenGraphPlaneHartmanVertex *v_info = aven_graph_plane_hartman_vinfo(
        ctx,
        frame,
        v
    );
    AvenGraphPlaneHartmanList *v_colors = &get(ctx->color_lists, v);

    if (v_info->mark == 0) {
        *v_info = (AvenGraphPlaneHartmanVertex){
            .mark = frame->x_info.mark,
            .nb = {
                .first = aven_graph_aug_adj_next(v_adj, zv.back_index),
                .last = zv.back_index,
            },
        };
        aven_graph_plane_hartman_remove_color(v_colors, z_color);
    } else if (v_info->mark == frame->x_info.mark) {
        if (zv_index == z_info->nb.last) {
            assert(frame->z == frame->y);
            assert(v == frame->x);
            v_info->nb.first = aven_graph_aug_adj_next(v_adj, zv.back_index),
            v_info->mark = ctx->next_mark++;
            frame->y = frame->x;
            frame->z = frame->x;
        } else {
            uint32_t new_mark = ctx->next_mark++;
            list_push(ctx->frames) = (AvenGraphPlaneHartmanFrame){
                .x = v,
                .y = v,
                .z = v,
                .x_info = {
                    .mark = new_mark,
                    .nb = {
                        .first = aven_graph_aug_adj_next(v_adj, zv.back_index),
                        .last = v_info->nb.last,
                    },
                },
            };
            v_info->nb.last = zv.back_index;
        }
    } else if (get(ctx->marks, v_info->mark) == frame->y_info.mark) {
        if (v_info->nb.first != zv.back_index) {
            uint32_t new_mark = ctx->next_mark++;
            list_push(ctx->frames) = (AvenGraphPlaneHartmanFrame){
                .x = v,
                .y = frame->z,
                .z = v,
                .x_info = {
                    .mark = new_mark,
                    .nb = {
                        .first = v_info->nb.first,
                        .last = zv.back_index,
                    },
                },
                .y_info = {
                    .mark = new_mark,
                    .nb = {
                        .first = zv_index,
                        .last = z_info->nb.last,
                    },
                },
            };
        }

        v_info->nb.first = aven_graph_aug_adj_next(v_adj, zv.back_index);

        if (aven_graph_plane_hartman_has_color(v_colors, z_color)) {
            get(*v_colors, 0) = z_color;
            v_colors->len = 1;

            frame->z = v;
            frame->z_info = *v_info;
        } else {
            get(ctx->marks, frame->x_info.mark) = frame->y_info.mark;
            frame->z = frame->x;
        }
    } else {
        aven_graph_plane_hartman_color_differently(v_colors, z_color);
        if (v_info->nb.first != zv.back_index) {
            //list_push(ctx->frames) = (AvenGraphPlaneHartmanFrame){
            //    .x = frame->x,
            //    .y = v,
            //    .z = frame->x,
            //    .x_info = frame->x_info,
            //    .y_info = {
            //        .mark = frame->x_info.mark,
            //        .nb = {
            //            .first = aven_graph_aug_adj_next(v_adj, zv.back_index),
            //            .last = v_info->nb.last,
            //        },
            //    },
            //};

            //v_info->nb.last = zv.back_index;
            //frame->x = v;
            //frame->x_info = *v_info;
            //frame->x_info.mark = ctx->next_mark++;

            list_push(ctx->frames) = (AvenGraphPlaneHartmanFrame){
                .x = v,
                .y = frame->y,
                .z = frame->z,
                .x_info = {
                    .mark = ctx->next_mark++,
                    .nb = {
                        .first = v_info->nb.first,
                        .last = zv.back_index,
                    },
                },
                .y_info = frame->y_info,
                .z_info = frame->z_info,
            };

            v_info->mark = frame->x_info.mark;
            v_info->nb.first = aven_graph_aug_adj_next(v_adj, zv.back_index);

            frame->z = frame->x;
            frame->y = v;
            frame->y_info = *v_info;
        } else {
            assert(frame->z == frame->y);
            v_info->nb.first = aven_graph_aug_adj_next(v_adj, zv.back_index),
            v_info->mark = frame->x_info.mark;
            frame->y = v;
            frame->y_info = *v_info;

            frame->z = frame->x;
        }
    }

    return false;
}

static inline void aven_graph_plane_hartman(
    AvenGraphPlaneHartmanListProp color_lists,
    AvenGraph graph,
    AvenGraphSubset outer_face,
    AvenArena arena
) {
    AvenGraphAug aug_graph = aven_graph_aug(graph, &arena);
    AvenGraphPlaneHartmanCtx ctx = aven_graph_plane_hartman_init(
        color_lists,
        aug_graph,
        outer_face,
        &arena
    );

    AvenGraphPlaneHartmanFrameOptional frame =
        aven_graph_plane_hartman_next_frame(&ctx);

    do {
        while (!aven_graph_plane_hartman_frame_step(&ctx, &frame.value)) {}
        frame = aven_graph_plane_hartman_next_frame(&ctx);
    } while (frame.valid);
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
    AvenGraphAugAdjList z_adj = get(ctx->graph, frame->z);
    AvenGraphPlaneHartmanVertex *z_info = aven_graph_plane_hartman_vinfo(
        ctx,
        frame,
        frame->z
    );
    AvenGraphPlaneHartmanList *z_colors = &get(ctx->color_lists, frame->z);
    uint32_t z_color = get(*z_colors, 0);

    uint32_t zu_index = z_info->nb.first;
    AvenGraphAugAdjListNode zu = get(z_adj, zu_index);

    uint32_t u = zu.vertex;

    if (zu_index == z_info->nb.last) {
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
    AvenGraphPlaneHartmanVertex *v_info = aven_graph_plane_hartman_vinfo(
        ctx,
        frame,
        v
    );
    AvenGraphPlaneHartmanList *v_colors = &get(ctx->color_lists, v);

    if (v_info->mark == 0) {
        return AVEN_GRAPH_PLANE_HARTMAN_CASE_3_1;
    } else if (v_info->mark == frame->x_info.mark) {
        if (
            zv.back_index == v_info->nb.first or
            zv.back_index == v_info->nb.last
        ) {
            return AVEN_GRAPH_PLANE_HARTMAN_CASE_3_2_1_A;
        } else {
            return AVEN_GRAPH_PLANE_HARTMAN_CASE_3_2_1_B;
        }
    } else if (get(ctx->marks, v_info->mark) == frame->y_info.mark) {
        if (aven_graph_plane_hartman_has_color(v_colors, z_color)) {
            if (
                zv.back_index == v_info->nb.first or
                zv.back_index == v_info->nb.last
            ) {
                return AVEN_GRAPH_PLANE_HARTMAN_CASE_3_2_3_1_A;
            } else {
                return AVEN_GRAPH_PLANE_HARTMAN_CASE_3_2_3_1_B;
            }
        } else {
            if (
                zv.back_index == v_info->nb.first or
                zv.back_index == v_info->nb.last
            ) {
                return AVEN_GRAPH_PLANE_HARTMAN_CASE_3_2_3_2_A;
            } else {
                return AVEN_GRAPH_PLANE_HARTMAN_CASE_3_2_3_2_B;
            }        }
    } else {
        if (
            zv.back_index == v_info->nb.first or
            zv.back_index == v_info->nb.last
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
