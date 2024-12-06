#ifndef AVEN_GRAPH_PLANE_HARTMAN_H
#define AVEN_GRAPH_PLANE_HARTMAN_H

#include <aven.h>
#include <aven/arena.h>

#include "../../graph.h"

typedef struct {
    uint32_t len;
    uint32_t data[3];
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
    uint32_t v;
    uint32_t x;
    uint32_t y;
    AvenGraphPlaneHartmanVertex v_info;
    AvenGraphPlaneHartmanVertex x_info;
    AvenGraphPlaneHartmanVertex y_info;
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
        .frames = { .cap = 2 * graph.len - 4 },
        .marks = { .len = 4 * graph.len - 6 },
        .next_mark = 1,
    };

    ctx.frames.ptr = aven_arena_create_array(
        AvenGraphPlaneHartmanFrame,
        arena,
        ctx.frames.cap
    );

    ctx.vertex_info.ptr = aven_arena_create_array(
        AvenGraphPlaneHartmanVertex,
        arena,
        ctx.vertex_info.len
    );

    for (uint32_t v = 0; v < ctx.vertex_info.len; v += 1) {
        slice_get(ctx.vertex_info, v) = (AvenGraphPlaneHartmanVertex){ 0 };
    }

    ctx.marks.ptr = aven_arena_create_array(
        uint32_t,
        arena,
        ctx.marks.len
    );

    for (uint32_t i = 0; i < ctx.marks.len; i += 1) {
        slice_get(ctx.marks, i) = i;
    }

    uint32_t face_mark = ctx.next_mark;
    ctx.next_mark += 1;

    uint32_t u = slice_get(cwise_outer_face, cwise_outer_face.len - 1);
    for (uint32_t i = 0; i < cwise_outer_face.len; i += 1) {
        uint32_t v = slice_get(cwise_outer_face, i);
        AvenGraphAugAdjList v_adj = slice_get(ctx.graph, v);
        
        uint32_t vu_index = aven_graph_aug_neighbor_index(ctx.graph, v, u);
        uint32_t uv_index = slice_get(v_adj, vu_index).back_index;
        
        slice_get(ctx.vertex_info, v).nb.first = vu_index;
        slice_get(ctx.vertex_info, u).nb.last = uv_index;

        slice_get(ctx.vertex_info, v).mark = face_mark;

        u = v;
    }

    uint32_t xyv = slice_get(cwise_outer_face, 0);
    AvenGraphPlaneHartmanVertex *xyv_info = &slice_get(ctx.vertex_info, xyv);
    xyv_info->mark = ctx.next_mark;
    ctx.next_mark += 1;

    AvenGraphPlaneHartmanList *xyv_colors = &slice_get(ctx.color_lists, xyv);
    assert(xyv_colors->len > 0);
    xyv_colors->len = 1;

    list_push(ctx.frames) = (AvenGraphPlaneHartmanFrame){
        .v = xyv,
        .x = xyv,
        .y = xyv,
        .x_info = *xyv_info,
        .y_info = *xyv_info,
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

    if (v == frame->v) {
        return &frame->v_info;
    }

    return &slice_get(ctx->vertex_info, v);
}

static bool aven_graph_plane_hartman_has_color(
    AvenGraphPlaneHartmanList *list,
    uint32_t color
) {
    assert(list->len > 0);
    for (size_t i = 0; i < list->len; i += 1) {
        if (list->data[i] == color) {
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
        if (list->data[i] == color) {
            assert(list->len > 1);
            list->data[i] = list->data[list->len - 1];
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
        if (list->data[i] != color) {
            list->data[0] = list->data[i];
            list->len = 1;
            break;
        }
    }
    assert(list->data[0] != color);
}

static inline AvenGraphPlaneHartmanFrameOptional aven_graph_plane_hartman_next_frame(
    AvenGraphPlaneHartmanCtx *ctx
) {
    if (ctx->frames.len == 0) {
        return (AvenGraphPlaneHartmanFrameOptional){ 0 };
    }

    AvenGraphPlaneHartmanFrame *frame = &list_get(ctx->frames, ctx->frames.len - 1);
    ctx->frames.len -= 1;

    return (AvenGraphPlaneHartmanFrameOptional){ .value = *frame, .valid = true }; 
}

static inline bool aven_graph_plane_hartman_frame_step(
    AvenGraphPlaneHartmanCtx *ctx,
    AvenGraphPlaneHartmanFrame *frame
) {
    uint32_t v = frame->v;
    AvenGraphAugAdjList v_adj = slice_get(ctx->graph, frame->v);
    AvenGraphPlaneHartmanVertex *v_info = aven_graph_plane_hartman_vinfo(ctx, frame, v);
    uint32_t vu_index = aven_graph_plane_hartman_vinfo(ctx, frame, v)->nb.first;
    AvenGraphAugAdjListNode vu = slice_get(v_adj, vu_index);

    AvenGraphPlaneHartmanList *v_colors = &slice_get(ctx->color_lists, v);
    assert(v_colors->len == 1);
    uint32_t v_color = v_colors->data[0];

    bool last_neighbor = (vu_index == v_info->nb.last);
    if (!last_neighbor) {
        v_info->nb.first = (uint32_t)((v_info->nb.first + 1) % v_adj.len);
    }

    uint32_t u = vu.vertex;
    AvenGraphAugAdjList u_adj = slice_get(ctx->graph, u);
    uint32_t uv_index = vu.back_index;
    AvenGraphPlaneHartmanVertex *u_info = aven_graph_plane_hartman_vinfo(
        ctx,
        frame,
        u
    );
    uint32_t u_mark = slice_get(ctx->marks, u_info->mark);

    AvenGraphPlaneHartmanList *u_colors = &slice_get(
        ctx->color_lists,
        u
    );

    bool done = false;

    if (u_mark == 0) {
        aven_graph_plane_hartman_remove_color(u_colors, v_color);
        assert(u_colors->len > 0);

        u_info->nb.first = (uint32_t)((uv_index + 1) % u_adj.len);
        u_info->nb.last =(uint32_t)((uv_index + u_adj.len - 1) % u_adj.len);
        u_info->mark = slice_get(ctx->marks, frame->x_info.mark);
    } else if (u_info->nb.first == u_info->nb.last) {
        if (u != frame->x and u != frame->y) {
            aven_graph_plane_hartman_color_differently(u_colors, v_color);
        }

        done = true;
    } else if (u_mark == slice_get(ctx->marks, frame->y_info.mark)) {
        uint32_t v_nb_old_last = v_info->nb.last;
        uint32_t u_nb_old_first = u_info->nb.first;

        u_info->nb.first = (uint32_t)((uv_index + 1) % u_adj.len);

        if (aven_graph_plane_hartman_has_color(u_colors, v_color)) {
            u_colors->data[0] = v_color;
            u_colors->len = 1;
            frame->v = u;
            frame->v_info = *u_info;
        } else {
            if (v != frame->x) {
                slice_get(ctx->marks, frame->x_info.mark) =
                    slice_get(ctx->marks, frame->y_info.mark);
                frame->v = frame->x;
                frame->x_info.mark = ctx->next_mark;
                ctx->next_mark += 1;
            }
        }

        if (u_nb_old_first != uv_index) {
            done = (u_info->nb.last == uv_index);
            if (done) {
                u_colors->len = 1;
            }

            uint32_t new_x_mark = ctx->next_mark;
            ctx->next_mark += 1;

            uint32_t new_y_mark = ctx->next_mark;
            ctx->next_mark += 1;

            list_push(ctx->frames) = (AvenGraphPlaneHartmanFrame){
                .v = u,
                .x = u,
                .y = v,
                .x_info = {
                    .mark = new_x_mark,
                    .nb = { .first = u_nb_old_first, .last = uv_index },
                },
                .y_info = {
                    .mark = new_y_mark,
                    .nb = { .first = vu_index, .last = v_nb_old_last },
                },
            };
        }
    } else if (v == frame->x) {
        aven_graph_plane_hartman_color_differently(u_colors, v_color);

        assert(uv_index == u_info->nb.last);

        u_info->nb.last = (uint32_t)((uv_index + u_adj.len - 1) % u_adj.len);
        u_info->mark = ctx->next_mark;
        ctx->next_mark += 1;

        if (v == frame->y) {
            frame->y_info = frame->x_info;
        } else {
            frame->v_info = frame->x_info;
        }

        frame->x = u;
        frame->x_info = *u_info;
    } else if (u_mark == slice_get(ctx->marks, frame->x_info.mark)) {
        assert(!aven_graph_plane_hartman_has_color(u_colors, v_color));

        if (u_info->nb.last != uv_index) {
            AvenGraphPlaneHartmanVertex new_u_info = {
                .mark = ctx->next_mark,
                .nb = {
                    .first = (uint32_t)((uv_index + 1) % u_adj.len),
                    .last = u_info->nb.last,
                },
            };
            ctx->next_mark += 1;
            list_push(ctx->frames) = (AvenGraphPlaneHartmanFrame){
                .v = u,
                .x = u,
                .y = u,
                .x_info = new_u_info,
                .y_info = new_u_info,
            };

            u_info->nb.last = (uint32_t)(
                (uv_index + u_adj.len - 1) % u_adj.len
            );
            done = last_neighbor;
        } else {
            u_info->nb.last = (uint32_t)(
                (uv_index + u_adj.len - 1) % u_adj.len
            );
        }
    } else {
        aven_graph_plane_hartman_color_differently(u_colors, v_color);

        if (u_info->nb.first != uv_index) {
            assert(!last_neighbor);
            u_info->mark = frame->x_info.mark;
            list_push(ctx->frames) = (AvenGraphPlaneHartmanFrame){
                .v = frame->x,
                .x = frame->x,
                .y = u,
                .x_info = {
                    .nb = frame->x_info.nb,
                    .mark = ctx->next_mark,
                },
                .y_info = {
                    .mark = u_info->mark,
                    .nb = {
                        .first = (uint32_t)((uv_index + 1) % u_adj.len),
                        .last = u_info->nb.last,
                    },
                },
            };

            ctx->next_mark += 1;

            u_info->nb.last = (uint32_t)(
                (uv_index + u_adj.len - 1) % u_adj.len
            );
            u_info->mark = ctx->next_mark;
            ctx->next_mark += 1;

            frame->x = u;
            frame->x_info = *u_info;
        } else {
            assert(last_neighbor);
            assert(frame->y == v);

            u_info->nb.first = (uint32_t)((uv_index + 1) % u_adj.len);
            u_info->mark = frame->x_info.mark;

            frame->y = u;
            frame->y_info = *u_info;

            frame->v = frame->x;
        }
    }

    return done;
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

#endif // AVEN_GRAPH_PLANE_HARTMAN_H
