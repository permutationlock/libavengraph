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
    AvenGraphPlaneHartmanNeighbors v_nb;
    AvenGraphPlaneHartmanVertex x_info;
    AvenGraphPlaneHartmanVertex y_info;
} AvenGraphPlaneHartmanFrame;

typedef struct {
    AvenGraphAug graph;
    AvenGraphPlaneHartmanListProp color_lists;
    Slice(AvenGraphPlaneHartmanVertex) vertex_info;
    List(uint32_t) marks;
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
        .marks = { .len = graph.len },
        .frames = { .cap = 2 * graph.len - 4 },
        .next_mark = 0,
    };

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

    ctx.frames.ptr = aven_arena_create_array(
        AvenGraphPlaneHartmanFrame,
        arena,
        ctx.frames.cap
    );

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
    }

    uint32_t xyv = slice_get(cwise_outer_face, 0);
    AvenGraphPlaneHartmanVertex xyv_info = slice_get(ctx.vertex_info, xyv);

    AvenGraphPlaneHartmanList *xyv_colors = &slice_get(ctx.color_lists, xyv);
    assert(xyv_colors->len > 0);
    xyv_colors->len = 1;

    list_push(ctx.frames) = (AvenGraphPlaneHartmanFrame){
        .v = xyv,
        .x = xyv,
        .y = xyv,
        .v_nb = xyv_info.nb,
        .y_info = xyv_info,
    };

    return ctx;
}

static inline bool aven_graph_plane_hartman_check(
    AvenGraphPlaneHartmanCtx *ctx
) {
    return ctx->frames.len == 0;
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
        }
    }
}

static inline bool aven_graph_plane_hartman_step(
    AvenGraphPlaneHartmanCtx *ctx
) {
    AvenGraphPlaneHartmanFrame *frame = &list_get(
        ctx->frames,
        ctx->frames.len - 1
    );

    uint32_t v = frame->v;
    AvenGraphAugAdjList v_adj = slice_get(ctx->graph, frame->v);
    uint32_t vu_index = frame->v_nb.first;
    AvenGraphAugAdjListNode vu = slice_get(v_adj, vu_index);

    AvenGraphPlaneHartmanList *v_colors = &slice_get(ctx->color_lists, v);
    assert(v_colors->len == 1);
    uint32_t v_color = v_colors->data[0];

    bool last_neighbor = (vu_index == frame->v_nb.last);
    if (!last_neighbor) {
        frame->v_nb.first = (frame->v_nb.first + 1) % v_adj.len;
    }

    uint32_t u = vu.vertex;
    AvenGraphAugAdjList u_adj = slice_get(ctx->graph, u);
    uint32_t uv_index = vu.back_index;
    AvenGraphPlaneHartmanVertex *u_info = aven_graph_plane_hartman_vinfo(
        ctx,
        frame,
        u
    );
    u_info->mark = slice_get(ctx->marks, u_info->mark);

    AvenGraphPlaneHartmanList *u_colors = &slice_get(
        ctx->color_lists,
        u
    );

    bool done = false;

    if (frame->v == frame->x) {
        if (u_info->mark != frame->y_info.mark) {
            aven_graph_plane_hartman_remove_color(u_colors, v_color);
        }

        assert(u_colors->len > 0);
        u_colors->len = 1;

        if (last_neighbor) {
            ctx->frames.len -= 1;
            done = (ctx->frames.len == 0);
        } else {
            assert(uv_index == u_info->nb.last);
            assert(uv_index != u_info->nb.first);

            u_info->nb.last = (uint32_t)(uv_index + u_adj.len - 1) % u_adj.len;
            u_info->mark = ctx->next_mark;
            ctx->next_mark += 1;

            frame->x = u;
            frame->x_info = *u_info;
        }
    } else if (u_info->mark == 0) {
        aven_graph_plane_hartman_remove_color(u_colors, v_color);
        assert(u_colors->len > 0);

        u_info->nb.first = (uv_index + 1) % u_adj.len;
        u_info->nb.last = uv_index;
        u_info->mark = frame->x_info.mark;
    } else if (u_info->mark == slice_get(ctx->marks, frame->x_info.mark)) {
        assert(!aven_graph_plane_hartman_has_color(u_colors, v_color));

        list_push(ctx->frames) = *frame;

        AvenGraphPlaneHartmanVertex new_u_info = {
            .mark = ctx->next_mark,
            .nb = {
                .first = (uv_index + 1) % u_adj.len,
                .last = u_info->nb.last,
            },
        };
        ctx->next_mark += 1;

        u_info->nb.last = (uv_index + 1) % u_adj.len;
        
        *frame = (AvenGraphPlaneHartmanFrame){
            .v = u,
            .x = u,
            .y = u,
            .v_nb = new_u_info.nb,
            .y_info = new_u_info,
        };
    } else if (u_info->mark == slice_get(ctx->marks, frame->y_info.mark)) {
        if (aven_graph_plane_hartman_has_color(u_colors, v_color)) {
            u_colors->data[0] = v_color;
            u_info->mark = frame->x_info.mark;
        }
        u_colors->len = 1;

        list_push(ctx->frames) = (AvenGraphPlaneHartmanFrame){
            .v = u,
            .x = u,
            .y = v,
            .v_nb = { .first = u_info->nb.first, .last = uv_index },
            .y_info = {
                .mark = ctx->next_mark,
                .nb = { .first = vu_index, .last = frame->v_nb.last },
            },
        };
        ctx->next_mark += 1;

        frame->v_nb.last = vu_index;
    } else {
        aven_graph_plane_hartman_remove_color(u_colors, v_color);
        assert(u_colors)
        u_colors->len = 1;

        u_info->mark = frame->x_info.mark;

        list_push(ctx->frames) = (AvenGraphPlaneHartmanFrame){
            .v = frame->x,
            .x = frame->x,
            .y = u,
            .v_nb = frame->x_info.nb,
            .y_info = {
                .mark = u_info->mark,
                .nb = { .first = uv_index, .last = u_info->nb.last, },
            },
        };

        u_info->nb.last = uv_index;
        frame->x = u;
        frame->x_info = *u_info;
    }

    if (!done and last_neighbor) {
        if (aven_graph_plane_hartman_has_color(u_colors, v_color)) {
            assert(u_colors->len == 1);
            frame->v = u;
            frame->v_nb = u_info->nb;
        } else {
            slice_get(ctx->marks, frame->x_info.mark) =
                slice_get(ctx->marks, frame->y_info.mark);
            frame->v = frame->x;
            frame->v_nb = frame->x_info.nb;
        }
    }

    return done;
}

#endif // AVEN_GRAPH_PLANE_HARTMAN_H
