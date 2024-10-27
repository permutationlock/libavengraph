#ifndef AVEN_GRAPH_PLANE_POH_H
#define AVEN_GRAPH_PLANE_POH_H

typedef struct {
    uint32_t u;
    uint32_t w;
    uint32_t t;
    uint32_t edge_index;
    uint32_t face_mark;
    uint32_t below_mark;
    uint8_t q_color;
    uint8_t p_color;
    bool above_path;
    bool last_uncolored;
} AvenGraphPlanePohFrame;

typedef struct {
    AvenGraph graph;
    AvenGraphPropUint8 coloring;
    AvenGraphPropUint32 marks;
    Slice(uint32_t) first_edges;
    List(AvenGraphPlanePohFrame) frames;
} AvenGraphPlanePohCtx;

static inline AvenGraphPlanePohCtx aven_graph_plane_poh_init(
    AvenGraph graph,
    uint32_t p1,
    uint32_t q1,
    uint32_t q2,
    AvenArena *arena
) {
    AvenGraphPlanePohCtx ctx = {
        .graph = graph,
        .coloring = { .len = graph.len },
        .marks = { .len = graph.len },
        .first_edges = { .len = graph.len },
        .frames = { .cap = graph.len - 2 },
    };

    ctx.coloring.ptr = aven_arena_create_array(
        uint8_t,
        arena,
        ctx.coloring.len
    );

    for (uint32_t i = 0; i < ctx.coloring.len; i += 1) {
        slice_get(ctx.coloring, i) = 0;
    }

    slice_get(ctx.coloring, p1) = 1;
    slice_get(ctx.coloring, q1) = 2;
    slice_get(ctx.coloring, q2) = 2;

    ctx.marks.ptr = aven_arena_create_array(
        uint32_t,
        arena,
        ctx.marks.len
    );

    for (uint32_t i = 0; i < ctx.marks.len; i += 1) {
        slice_get(ctx.marks, i) = 0;
    }

    slice_get(ctx.marks, p1) = 1;

    ctx.first_edges.ptr = aven_arena_create_array(
        uint32_t,
        arena,
        ctx.first_edges.len
    );

    slice_get(ctx.first_edges, p1) = aven_graph_neighbor_index(
        graph,
        p1,
        q1
    );

    ctx.frames.ptr = aven_arena_create_array(
        AvenGraphPlanePohFrame,
        arena,
        ctx.frames.cap
    );

    list_push(ctx.frames) = (AvenGraphPlanePohFrame){
        .p_color = 3,
        .q_color = 2,
        .u = p1,
        .w = p1,
        .t = p1,
        .face_mark = 1,
        .below_mark = 2,
    };

    return ctx;
}

static inline bool aven_graph_plane_poh_check(AvenGraphPlanePohCtx *ctx) {
    return ctx->frames.len == 0;
}

static inline bool aven_graph_plane_poh_step(AvenGraphPlanePohCtx *ctx) {
    AvenGraphPlanePohFrame *frame = &list_get(ctx->frames, ctx->frames.len - 1);

//    int printf(const char *, ...);
//    printf(
//        "frames[%lu] = { .u = %u, .w = %u, .t = %u, .edge_index = %u, .p_color = %u, .q_color = %u, .face_mark = %u, .below_mark = %u }\n",
//        ctx->frames.len - 1,
//        frame->u,
//        frame->w,
//        frame->t,
//        frame->edge_index,
//        frame->p_color,
//        frame->q_color,
//        frame->face_mark,
//        frame->below_mark
//    );

    uint8_t path_color = frame->p_color ^ frame->q_color;

    AvenGraphAdjList u_adj = slice_get(ctx->graph, frame->u);

    if (frame->edge_index == u_adj.len) {
        if (frame->t == frame->u) {
            assert(frame->w == frame->u);
            ctx->frames.len -= 1;
            return true;
        }

        if (frame->w == frame->u) {
            frame->w = frame->t;
        }
        frame->u = frame->t;
        frame->edge_index = 0;
        frame->above_path = false;
        return false;
    }

    uint32_t n_index = slice_get(ctx->first_edges, frame->u) +
        frame->edge_index;
    if (n_index >= u_adj.len) {
        n_index -= (uint32_t)u_adj.len;
    }
    uint32_t n = slice_get(u_adj, n_index);
    uint32_t n_color = slice_get(ctx->coloring, n);
    uint32_t n_mark = slice_get(ctx->marks, n);

//    printf("\tn = %u, n_index = %u, n_color = %u, n_mark = %u\n", n, n_index, n_color, n_mark);

    frame->edge_index += 1;

    if (frame->above_path) {
        if (n_color == 0) {
            frame->last_uncolored = true;
            return false;
        }

        if (!frame->last_uncolored) {
            return false;
        }

        if (frame->last_uncolored) {
            assert(n_color != frame->q_color);
            uint32_t l = slice_get(
                u_adj,
                (n_index + u_adj.len - 1) % u_adj.len
            );

            slice_get(ctx->coloring, l) = frame->q_color;
            slice_get(ctx->first_edges, l) = aven_graph_neighbor_index(
                ctx->graph,
                l,
                frame->u
            );
            
            list_push(ctx->frames) = (AvenGraphPlanePohFrame){
                .q_color = path_color,
                .p_color = frame->p_color,
                .u = l,
                .t = l,
                .w = l,
                .face_mark = frame->face_mark,
                .below_mark = frame->below_mark,
            };
        }

        frame->last_uncolored = false;
    } else {
        if (n == frame->w) {
            return false;
        } else if (n_color != 0) {
            if (n_color == frame->p_color) {
                frame->above_path = true;
            }
            if (frame->w == frame->u) {
                return false;
            }

            list_push(ctx->frames) = (AvenGraphPlanePohFrame){
                .q_color = frame->q_color,
                .p_color = path_color,
                .u = frame->w,
                .w = frame->w,
                .t = frame->w,
                .face_mark = frame->below_mark,
                .below_mark = frame->below_mark + 1,
            };

            frame->w = frame->u;
        } else if (n_mark == frame->face_mark) {
            slice_get(ctx->coloring, n) = path_color;
            slice_get(ctx->first_edges, n) = aven_graph_next_neighbor_index(
                ctx->graph,
                n,
                frame->u
            );

            frame->t = n;
            frame->above_path = true;
        } else {
            slice_get(ctx->marks, n) = frame->below_mark;

            if (frame->w == frame->u) {
                frame->w = n;

                slice_get(ctx->coloring, n) = frame->p_color;
                slice_get(ctx->first_edges, n) = aven_graph_next_neighbor_index(
                    ctx->graph,
                    n,
                    frame->u
                );
            }
        }
    }

    return false;
}

static inline AvenGraphPropUint8 aven_graph_plane_poh_result(
    AvenGraphPlanePohCtx *ctx,
    AvenArena *arena
) {
    AvenGraphPropUint8 coloring = { .len = ctx->coloring.len };
    coloring.ptr = aven_arena_create_array(uint8_t, arena, coloring.len);

    slice_copy(coloring, ctx->coloring);

    return coloring;
}

#endif // AVEN_GRAPH_PLANE_POH_H
