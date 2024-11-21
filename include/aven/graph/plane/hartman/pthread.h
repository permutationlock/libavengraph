#ifndef AVEN_GRAPH_PLANE_HARTMAN_PTHREAD_H
#define AVEN_GRAPH_PLANE_HARTMAN_PTHREAD_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/thread_pool.h>

#include "../../../graph.h"
#include "../hartman.h"

int printf(const char *fmt, ...);

typedef struct {
    AvenGraphPlaneHartmanFrame frame;
    uint32_t parent;
} AvenGraphPlaneHartmanPthreadEntry;

typedef struct {
    AvenGraphAug graph;
    AvenGraphPlaneHartmanListProp color_lists;
    Slice(AvenGraphPlaneHartmanVertex) vertex_info;
    Slice(uint32_t) marks;
    List(uint32_t) valid_entries;
    Pool(AvenGraphPlaneHartmanPthreadEntry) entry_pool;
    uint32_t next_mark;
    size_t nthreads;
    size_t frames_active;
    bool lock;
} AvenGraphPlaneHartmanPthreadCtx;

static inline AvenGraphPlaneHartmanPthreadCtx
aven_graph_plane_hartman_pthread_init(
    AvenGraphPlaneHartmanListProp color_lists,
    AvenGraphAug graph,
    AvenGraphSubset cwise_outer_face,
    size_t nthreads,
    AvenArena *arena
) {
    if (graph.len > 0x3fffffff) {
        aven_panic("vertices won't fit in 30 bits");
    }

    AvenGraphPlaneHartmanPthreadCtx ctx = {
        .graph = graph,
        .color_lists = color_lists,
        .next_mark = 1,
        .nthreads = nthreads,
        .entry_pool = aven_arena_create_pool(
            AvenGraphPlaneHartmanPthreadEntry,
            arena,
            2 * graph.len - 4
        ),
        .valid_entries = aven_arena_create_list(
            uint32_t,
            arena,
            2 * graph.len - 4
        ),
        .vertex_info = aven_arena_create_slice(
            AvenGraphPlaneHartmanVertex,
            arena,
            graph.len
        ),
        .marks = aven_arena_create_slice(
            uint32_t,
            arena,
            (3 + nthreads) * graph.len
        ),
    };

    for (uint32_t v = 0; v < ctx.vertex_info.len; v += 1) {
        slice_get(ctx.vertex_info, v) = (AvenGraphPlaneHartmanVertex){ 0 };
    }

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

    uint32_t entry_index = (uint32_t)pool_create(ctx.entry_pool);
    pool_get(ctx.entry_pool, entry_index) = (AvenGraphPlaneHartmanPthreadEntry){
        .frame = {
            .v = xyv,
            .x = xyv,
            .y = xyv,
            .x_info = *xyv_info,
            .y_info = *xyv_info,
        },
    };
    list_push(ctx.valid_entries) = entry_index;

    return ctx;
}

static inline void aven_graph_plane_hartman_pthread_lock(
    AvenGraphPlaneHartmanPthreadCtx *pthread_ctx
) {    
    for (;;) {
        if (
            !__atomic_exchange_n(&pthread_ctx->lock, true, __ATOMIC_ACQUIRE)
        ) {
            return;
        }
        while (__atomic_load_n(&pthread_ctx->lock, __ATOMIC_RELAXED)) {
            __builtin_ia32_pause();
        }
    }
}

static inline void aven_graph_plane_hartman_pthread_unlock(
    AvenGraphPlaneHartmanPthreadCtx *pthread_ctx
) {
    __atomic_store_n(&pthread_ctx->lock, false, __ATOMIC_RELEASE);
}

static inline AvenGraphPlaneHartmanFrameOptional
aven_graph_plane_hartman_pthread_next_frame(
    AvenGraphPlaneHartmanPthreadCtx *ctx,
    uint32_t thread_index
) {
    (void)thread_index;

    size_t frames_active = __atomic_sub_fetch(
        &ctx->frames_active,
        1,
        __ATOMIC_RELAXED
    );
    // printf("%u - looking for new frame\n", thread_index);

    for (;;) {
        if (
            ctx->valid_entries.len > 0 or
            frames_active == 0
        ) {
            // printf("%u - done waiting: %u / %lu\n", thread_index, pthread_ctx->new_frames, ctx->frames.len);
            aven_graph_plane_hartman_pthread_lock(ctx);
            if (ctx->valid_entries.len > 0) {
                uint32_t entry_index = list_pop(ctx->valid_entries);
                AvenGraphPlaneHartmanFrame frame = pool_get(
                    ctx->entry_pool,
                    entry_index
                ).frame;
                pool_delete(ctx->entry_pool, entry_index);
                __atomic_add_fetch(
                        &ctx->frames_active,
                        1,
                        __ATOMIC_RELAXED
                    );
                aven_graph_plane_hartman_pthread_unlock(ctx);

                return (AvenGraphPlaneHartmanFrameOptional){
                    .value = frame,
                    .valid = true,
                };
            }

            if (ctx->entry_pool.used == 0 and ctx->frames_active == 0) {
                // printf("%u - killed\n", thread_index);
                aven_graph_plane_hartman_pthread_unlock(ctx);
                return (AvenGraphPlaneHartmanFrameOptional){ 0 };
            }

            aven_graph_plane_hartman_pthread_unlock(ctx);
        }

        // printf("%u - waiting\n", thread_index);

        __builtin_ia32_pause();
        frames_active = __atomic_load_n(
            &ctx->frames_active,
            __ATOMIC_RELAXED
        );

        // printf("%u - done waiting\n", thread_index);
    }
}

static inline void aven_graph_plane_hartman_pthread_push_entries(
    AvenGraphPlaneHartmanPthreadCtx *ctx,
    AvenGraphPlaneHartmanList *v_colors,
    AvenGraphPlaneHartmanFrameOptional *maybe_frame
) {
    uint32_t parent_index = v_colors->len >> 2;
    if (!maybe_frame->valid) {
        if (parent_index == 0) {
            return;
        }
        if ((v_colors->len & 0x3) != 1) {
            return;
        }
    }

    aven_graph_plane_hartman_pthread_lock(ctx);
    if ((v_colors->len & 0x3) == 1) {
        while (parent_index != 0) {
            list_push(ctx->valid_entries) = parent_index - 1;
            parent_index = pool_get(ctx->entry_pool, parent_index - 1).parent;
        }
        v_colors->len = 1;
    }
    if (maybe_frame->valid) {
        uint32_t entry_index = (uint32_t)pool_create(ctx->entry_pool);
        pool_get(ctx->entry_pool, entry_index) =
            (AvenGraphPlaneHartmanPthreadEntry){
                .frame = maybe_frame->value,
                .parent = parent_index,
            };
        if ((v_colors->len & 0x3) == 1) {
            list_push(ctx->valid_entries) = entry_index;
        } else {
            v_colors->len = (((entry_index + 1) << 2) | (v_colors->len & 0x3));
        }
    }
    aven_graph_plane_hartman_pthread_unlock(ctx);
}

static bool aven_graph_plane_hartman_pthread_has_color(
    AvenGraphPlaneHartmanList *list,
    uint32_t color
) {
    assert(list->len > 0);
    for (size_t i = 0; i < (list->len & 0x3); i += 1) {
        if (list->data[i] == color) {
            return true;
        }
    }

    return false;
}

static void aven_graph_plane_hartman_pthread_remove_color(
    AvenGraphPlaneHartmanList *list,
    uint32_t color
) {
    for (size_t i = 0; i < (list->len & 0x3); i += 1) {
        if (list->data[i] == color) {
            assert((list->len & 0x3) > 1);
            list->data[i] = list->data[list->len - 1];
            list->len -= 1;
        }
    }
}

static inline AvenGraphPlaneHartmanVertex *
aven_graph_plane_hartman_pthread_vinfo(
    AvenGraphPlaneHartmanPthreadCtx *ctx,
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

static void aven_graph_plane_hartman_pthread_color_differently(
    AvenGraphPlaneHartmanList *list,
    uint32_t color
) {
    for (size_t i = 0; i < (list->len & 0x3); i += 1) {
        if (list->data[i] != color) {
            list->data[0] = list->data[i];
            list->len = ((list->len & 0xfffffffc) | 1);
        }
    }
    assert(list->data[0] != color);
}

typedef struct {
    uint32_t next_mark;
    uint32_t final_mark;
    uint32_t block_size;
} AvenGraphPlaneHartmanPthreadMarkSet;

static inline uint32_t aven_graph_plane_hartman_pthread_next_mark(
    AvenGraphPlaneHartmanPthreadCtx *ctx,
    AvenGraphPlaneHartmanPthreadMarkSet *mark_set
) {
    if (mark_set->next_mark == mark_set->final_mark) {
        mark_set->next_mark = __atomic_fetch_add(
            &ctx->next_mark,
            mark_set->block_size,
            __ATOMIC_RELAXED
        );
        mark_set->final_mark = mark_set->next_mark + mark_set->block_size;
    }

    uint32_t next_mark = mark_set->next_mark;
    mark_set->next_mark += 1;
    return next_mark;
}

static inline bool aven_graph_plane_hartman_pthread_frame_step(
    AvenGraphPlaneHartmanPthreadCtx *ctx,
    AvenGraphPlaneHartmanFrame *frame,
    AvenGraphPlaneHartmanPthreadMarkSet *mark_set
) {
    // printf("v: %u, x: %u, y: %u\n", frame->v, frame->x, frame->y);
    uint32_t v = frame->v;
    AvenGraphAugAdjList v_adj = slice_get(ctx->graph, frame->v);
    AvenGraphPlaneHartmanVertex *v_info = aven_graph_plane_hartman_pthread_vinfo(
        ctx,
        frame,
        v
    );
    uint32_t vu_index = aven_graph_plane_hartman_pthread_vinfo(
        ctx,
        frame,
        v
    )->nb.first;
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
    AvenGraphPlaneHartmanVertex *u_info =
        aven_graph_plane_hartman_pthread_vinfo(
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

    AvenGraphPlaneHartmanFrameOptional maybe_frame = { 0 };

    if (u_mark == 0) {
        aven_graph_plane_hartman_pthread_remove_color(u_colors, v_color);
        assert(u_colors->len > 1);

        u_info->nb.first = (uint32_t)((uv_index + 1) % u_adj.len);
        u_info->nb.last =(uint32_t)((uv_index + u_adj.len - 1) % u_adj.len);
        u_info->mark = slice_get(ctx->marks, frame->x_info.mark);
    } else if (u_info->nb.first == u_info->nb.last) {
        if (u != frame->x and u != frame->y) {
            aven_graph_plane_hartman_pthread_color_differently(
                u_colors,
                v_color
            );
        }

        done = true;
    } else if (u_mark == slice_get(ctx->marks, frame->y_info.mark)) {
        uint32_t v_nb_old_last = v_info->nb.last;
        uint32_t u_nb_old_first = u_info->nb.first;

        u_info->nb.first = (uint32_t)((uv_index + 1) % u_adj.len);

        if (aven_graph_plane_hartman_pthread_has_color(u_colors, v_color)) {
            u_colors->data[0] = v_color;
            u_colors->len = ((u_colors->len & 0xfffffffc) | 1);
            frame->v = u;
            frame->v_info = *u_info;
        } else {
            if (v != frame->x) {
                slice_get(ctx->marks, frame->x_info.mark) =
                    slice_get(ctx->marks, frame->y_info.mark);
                frame->v = frame->x;
                frame->x_info.mark = aven_graph_plane_hartman_pthread_next_mark(
                    ctx,
                    mark_set
                );
            }
        }

        if (u_nb_old_first != uv_index) {
            uint32_t new_x_mark = aven_graph_plane_hartman_pthread_next_mark(
                ctx,
                mark_set
            );

            uint32_t new_y_mark = aven_graph_plane_hartman_pthread_next_mark(
                ctx,
                mark_set
            );

            if (u_info->nb.last == uv_index) {
                u_colors->len = ((u_colors->len & 0xfffffffc) | 1);

                frame->v = u;
                frame->x = u;
                frame->y = v;
                frame->x_info = (AvenGraphPlaneHartmanVertex){
                    .mark = new_x_mark,
                    .nb = { .first = u_nb_old_first, .last = uv_index },
                };
                frame->y_info = (AvenGraphPlaneHartmanVertex){
                    .mark = new_y_mark,
                    .nb = { .first = vu_index, .last = v_nb_old_last },
                };
            } else {
                maybe_frame.value = (AvenGraphPlaneHartmanFrame){
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
                maybe_frame.valid = true;
            }
        }
    } else if (v == frame->x) {
        aven_graph_plane_hartman_pthread_color_differently(u_colors, v_color);

        assert(uv_index == u_info->nb.last);

        u_info->nb.last = (uint32_t)((uv_index + u_adj.len - 1) % u_adj.len);
        u_info->mark = aven_graph_plane_hartman_pthread_next_mark(
            ctx,
            mark_set
        );

        if (v == frame->y) {
            frame->y_info = frame->x_info;
        } else {
            frame->v_info = frame->x_info;
        }

        frame->x = u;
        frame->x_info = *u_info;
    } else if (u_mark == slice_get(ctx->marks, frame->x_info.mark)) {
        assert(
            !aven_graph_plane_hartman_pthread_has_color(
                u_colors,
                v_color
            )
        );

        if (u_info->nb.last != uv_index) {
            AvenGraphPlaneHartmanVertex new_u_info = {
                .mark = aven_graph_plane_hartman_pthread_next_mark(
                    ctx,
                    mark_set
                ),
                .nb = {
                    .first = (uint32_t)((uv_index + 1) % u_adj.len),
                    .last = u_info->nb.last,
                },
            };
            maybe_frame.value = (AvenGraphPlaneHartmanFrame){
                .v = u,
                .x = u,
                .y = u,
                .x_info = new_u_info,
                .y_info = new_u_info,
            };
            maybe_frame.valid = true;

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
        aven_graph_plane_hartman_pthread_color_differently(u_colors, v_color);

        if (u_info->nb.first != uv_index) {
            assert(!last_neighbor);
            u_info->mark = frame->x_info.mark;
            uint32_t next_mark = aven_graph_plane_hartman_pthread_next_mark(
                ctx,
                mark_set
            );
            maybe_frame.value = (AvenGraphPlaneHartmanFrame){
                .v = frame->x,
                .x = frame->x,
                .y = u,
                .x_info = {
                    .nb = frame->x_info.nb,
                    .mark = next_mark,
                },
                .y_info = {
                    .mark = u_info->mark,
                    .nb = {
                        .first = (uint32_t)((uv_index + 1) % u_adj.len),
                        .last = u_info->nb.last,
                    },
                },
            };
            maybe_frame.valid = true;

            u_info->nb.last = (uint32_t)(
                (uv_index + u_adj.len - 1) % u_adj.len
            );
            u_info->mark = aven_graph_plane_hartman_pthread_next_mark(
                ctx,
                mark_set
            );

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
            // PROBABLY NOT NEEDED
            // frame->x_info.mark = __atomic_fetch_add(
            //     &ctx->next_mark,
            //     1,
            //     __ATOMIC_RELAXED
            // );
        }
    }

    aven_graph_plane_hartman_pthread_push_entries(
        ctx,
        u_colors,
        &maybe_frame
    );

    return done;
}

typedef struct {
    AvenGraphPlaneHartmanPthreadCtx *pthread_ctx;
    uint32_t thread_index;
} AvenGraphPlaneHartmanPthreadWorker;

static inline void aven_graph_plane_hartman_pthread_worker(void *args) {
    AvenGraphPlaneHartmanPthreadWorker *worker = args;

    __atomic_add_fetch(
        &worker->pthread_ctx->frames_active,
        1,
        __ATOMIC_RELAXED
    );

    AvenGraphPlaneHartmanFrameOptional frame =
        aven_graph_plane_hartman_pthread_next_frame(
            worker->pthread_ctx,
            worker->thread_index
        );

    AvenGraphPlaneHartmanPthreadMarkSet mark_set = {
        .block_size = min(
            2048,
            (uint32_t)worker->pthread_ctx->graph.len
        ),
    };
    while (frame.valid) {
        while (
            !aven_graph_plane_hartman_pthread_frame_step(
                worker->pthread_ctx,
                &frame.value,
                &mark_set
            )
        ) {}
        frame = aven_graph_plane_hartman_pthread_next_frame(
            worker->pthread_ctx,
            worker->thread_index
        );
    }
}

static inline void aven_graph_plane_hartman_pthread(
    AvenGraphPlaneHartmanListProp color_lists,
    AvenGraph graph,
    AvenGraphSubset outer_face,
    AvenThreadPool *thread_pool,
    AvenArena arena
) {
    AvenGraphAug aug_graph = aven_graph_aug(graph, &arena);
    AvenGraphPlaneHartmanPthreadCtx ctx = aven_graph_plane_hartman_pthread_init(
        color_lists,
        aug_graph,
        outer_face,
        thread_pool->active_threads + 1,
        &arena
    );

    Slice(AvenGraphPlaneHartmanPthreadWorker) workers = aven_arena_create_slice(
        AvenGraphPlaneHartmanPthreadWorker,
        &arena,
        thread_pool->active_threads + 1
    );

    for (uint32_t i = 0; i < thread_pool->active_threads; i += 1) {
        slice_get(workers, i) = (AvenGraphPlaneHartmanPthreadWorker) {
            .pthread_ctx = &ctx,
            .thread_index = i + 1
        };
    }

    for (uint32_t i = 0; i < thread_pool->active_threads; i += 1) {
        aven_thread_pool_submit(
            thread_pool,
            aven_graph_plane_hartman_pthread_worker,
            &slice_get(workers, i)
        );
    }

    aven_graph_plane_hartman_pthread_worker(&slice_get(workers, 0));

    aven_thread_pool_wait(thread_pool);
}

#endif // AVEN_GRAPH_PLANE_HARTMAN_PTHREAD_H
