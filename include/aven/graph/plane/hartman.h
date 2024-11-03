#ifndef AVEN_GRAPH_PLANE_HARTMAN_H
#define AVEN_GRAPH_PLANE_HARTMAN_H

typedef struct {
    uint32_t len;
    uint32_t colors[3];
} AvenGraphPlaneHartmanList;
typedef Slice(AvenGraphPlaneHartmanList) AvenGraphPlaneHartmanListProp;

typedef struct {
    uint32_t v;
    uint32_t x;
    uint32_t y;
    AvenGraphPlaneHartmanVertex v_info;
    AvenGraphPlaneHartmanVertex x_info;
    AvenGraphPlaneHartmanVertex y_info;
} AvenGraphPlaneHartmanFrame;

typedef struct {
    uint32_t mark;
    uint32_t first;
    uint32_t last;
} AvenGraphPlaneHartmanVertex;
typedef Slice(AvenGraphPlaneHartmanVertex) AvenGraphPlaneHartmanVertexProp;

typedef struct {
    AvenGraphAug graph;
    AvenGraphPlaneHartmanListProp color_lists;
    AvenGraphPlaneHartmanVertexProp vertex_info;
    List(uint32_t) mark_parent;
    List(AvenGraphPlaneHartmanFrame) frames;
} AvenGraphPlaneHartmanCtx;

static inline AvenGraphPlaneHartmanCtx aven_graph_plane_hartman_init(
    AvenGraphAug graph,
    AvenGraphPlaneHartmanListProp color_lists,
    AvenGraphSubset cwise_outer_face,
    AvenArena *arena
) {
    AvenGraphPlaneHartmanCtx ctx = {
        .graph = graph,
        .color_lists = color_lists,
        .vertex_info = { .len = graph.len },
        .mark_parent = { .len = graph.len },
        .frames = { .cap = 2 * graph.len - 4 },
    };

    ctx.vertex_info.ptr = aven_arena_create_array(
        AvenGraphPlaneHartmanVertexInfo,
        arena,
        ctx.vertex_info.len
    );

    for (uint32_t v = 0; v < ctx.vertex_info.len; v += 1) {
        slice_get(ctx.vertex_info, v) = (AvenGraphPlaneHartmanVertex){ 0 };
    }

    ctx.mark_parents.ptr = aven_arena_create_array(
        AvenGraphPlaneHartmanFrame,
        arena,
        ctx.mark_parents.len
    );

    for (uint32_t i = 0; i < ctx.mark_parents.len; i += 1) {
        slice_get(ctx.mark_parents, i) = 0;
    }

    ctx.frames.ptr = aven_arena_create_array(
        AvenGraphPlaneHartmanFrame,
        arena,
        ctx.frames.cap
    );

    uint32_t u = slice_get(cwise_outer_face, cwise_outer_face.len - 1);
    for (uint32_t i = 0; i < cwise_outer_face.len; i += 1) {
        uint32_t v = slice_get(cwise_outer_face, i);
        AvenGraphAugAdjList v_adj = slice_get(ctx.graph, v);
        
        uint32_t vu_index = aven_graph_aug_neighbor_index(ctx.graph, v, u);
        uint32_t uv_index = slice_get(v_adj, vu_index).back_index;
        
        slice_get(ctx.vertex_info, v).first = vu_index;
        slice_get(ctx.vertex_info, u).last = uv_index;

        slice_get(ctx.vertex_info, v).mark = 1;
    }

    list_push(ctx.frames) = (AvenGraphPlaneHartmanFrame){
        .x = slice_get(cwise_outer_face, 0),
        .y = slice_get(cwise_outer_face, cwise_outer_face.len - 1),
        .v = slice_get(cwise_outer_face, 0),
        .edge_index = 0,
        .py_mark = 1,
    };
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

static inline bool aven_graph_plane_hartman_step(
    AvenGraphPlaneHartmanCtx *ctx
) {
    AvenGraphPlaneHartmanFrame *frame = &list_get(
        ctx->frames,
        ctx->frames.len - 1
    );

    uint32_t v = frame->v;
    AvenGraphAugAdjList v_adj = slice_get(ctx->graph, ctx->v);
    uint32_t vu_index = v_info->first;
    AvenGraphAugAdjListNode vu = slice_get(v_adj, vu_index);
    AvenGraphPlaneHartmanList *v_colors = &slice_get(ctx->color_lists, v);

    bool last_neighbor = frame->v_info.first == frame->v_info.last;
    if (!last_neighbor) {
        frame->v_info.first = (frame->v_info.first + 1) % v_adj.len;
    }

    uint32_t u = vu.vertex;
    AvenGraphAugAdjList u_adj = slice_get(ctx->graph, u);
    uint32_t uv_index = vu.back_index;
    AvenGraphPlaneHartmanVertex *u_info = aven_graph_plane_hartman_vinfo(
        ctx,
        frame,
        u
    );
    AvenGraphPlaneHartmanList *u_colors = &slice_get(
        ctx->color_lists,
        u
    );

    bool done = false;

    if (frame->v == frame->x) {
        if (last_neighbor) {
            if (frame->v == frame->y and u_colors->data[0] == frame->p_color) {
                assert(u_colors->len > 1);
                u_colors->data[0] = u_colors->data[1];
            }

            u_colors.len = 1;

            ctx->frames.len -= 1;
            done = (ctx->frames.len == 0);
        } else {
            assert(uv_index == u_info->last);
            assert(uv_index != u_info->first);
            u_info->last = (uv_index + u_adj.len - 1) % u_adj.len;
            u_info->mark = // ?? TODO: mark tracking

            frame->x = u;
            frame->x_info = *u_info;
        }
    } else if (u_info.mark == 0) {
        
    } else if (u_info->mark == xp_mark) {
        AvenGraphPlaneHartmanList u_list = slice_get(ctx->color_lists, u);
        for (uint32_t i = 0; i < u_list.len; i += 1) {
            assert(u_list.colors[i] != ctx->p_color);
        }
    } else if (u_info->mark == py_mark) {
        
    } else if (u_info->mark == yx_mark) {
        
    }

    if (vu_index == v_info.last) {
        if (u_info->colors[0] == p_color) {
            frame->v = u;
            frame->edge_index = 0;
        } else {
            AvenGraphPlaneHartmanList *x_colors = slice_get(
                ctx->color_lists,
                ctx->x
            );
            if (x_colors->len != 1 or x_colors->colors[0] != ctx->p_color) {
                x_colors->len = 1;

                frame->v = x;
                frame->edge_index = 0;
                frame->p_color = x_info.colors[0];
                frame->py_mark = frame->xp_mark;
                frame->xp_mark = 0;
            }
        }
    }

    return frame_done;
}

#endif // AVEN_GRAPH_PLANE_HARTMAN_H
