#ifndef AVEN_GRAPH_PLANE_HARTMAN_THREAD_H
#define AVEN_GRAPH_PLANE_HARTMAN_THREAD_H

#include <aven.h>
#include <aven/arena.h>
#include <aven/thread_pool.h>

#if !defined(__STDC_VERSION__) or __STDC_VERSION__ < 201112L
    #error "C11 or later is required"
#endif

#include <stdatomic.h>

#include "../../../graph.h"
#include "../hartman.h"

typedef struct {
    AvenGraphPlaneHartmanFrame frame;
    uint32_t parent;
} AvenGraphPlaneHartmanThreadEntry;

typedef struct {
    uint32_t *ptr;
    atomic_size_t len;
    size_t cap;
} AvenGraphPlaneHartmanThreadUint32List;

typedef struct {
    AvenGraphAug graph;
    AvenGraphPlaneHartmanListProp color_lists;
    Slice(AvenGraphPlaneHartmanVertex) vertex_info;
    Slice(uint32_t) marks;
    AvenGraphPlaneHartmanThreadUint32List valid_entries;
    Pool(AvenGraphPlaneHartmanThreadEntry) entry_pool;
    size_t nthreads;
    atomic_int frames_active;
    atomic_uint_least32_t next_mark;
    atomic_bool lock;
} AvenGraphPlaneHartmanThreadCtx;

static inline AvenGraphPlaneHartmanThreadCtx
aven_graph_plane_hartman_thread_init(
    AvenGraphPlaneHartmanListProp color_lists,
    AvenGraphAug graph,
    AvenGraphSubset cwise_outer_face,
    size_t nthreads,
    AvenArena *arena
) {
    if (graph.len > 0x3fffffff) {
        aven_panic("vertex representation won't fit in 30 bits");
    }

    AvenGraphPlaneHartmanThreadCtx ctx = {
        .graph = graph,
        .color_lists = color_lists,
        .nthreads = nthreads,
        .entry_pool = { .cap = 2 * graph.len - 4 },
        .valid_entries = { .cap = 2 * graph.len - 4 },
        .vertex_info = { .len = graph.len },
        .marks = { .len = (3 + nthreads) * graph.len },
    };

    ctx.entry_pool.ptr = (void *)aven_arena_create_array(
        PoolEntry(AvenGraphPlaneHartmanThreadEntry),
        arena,
        ctx.entry_pool.cap
    );
    ctx.valid_entries.ptr = aven_arena_create_array(
        uint32_t,
        arena,
        ctx.valid_entries.cap
    );
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
    
    atomic_init(&ctx.valid_entries.len, 0);
    atomic_init(&ctx.frames_active, 0);
    atomic_init(&ctx.next_mark, 1);
    atomic_init(&ctx.lock, false);

    for (uint32_t v = 0; v < ctx.vertex_info.len; v += 1) {
        get(ctx.vertex_info, v) = (AvenGraphPlaneHartmanVertex){ 0 };
    }

    for (uint32_t i = 0; i < ctx.marks.len; i += 1) {
        get(ctx.marks, i) = i;
    }

    uint32_t face_mark = (uint32_t)ctx.next_mark;
    ctx.next_mark += 1;

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
    xyv_info->mark = (uint32_t)ctx.next_mark;
    ctx.next_mark += 1;

    AvenGraphPlaneHartmanList *xyv_colors = &get(ctx.color_lists, xyv);
    assert(xyv_colors->len > 0);
    xyv_colors->len = 1;

    uint32_t entry_index = (uint32_t)pool_create(ctx.entry_pool);
    pool_get(ctx.entry_pool, entry_index) = (AvenGraphPlaneHartmanThreadEntry){
        .frame = {
            .z = xyv,
            .x = xyv,
            .y = xyv,
            .x_info = *xyv_info,
            .y_info = *xyv_info,
        },
    };
    list_push(ctx.valid_entries) = entry_index;

    return ctx;
}

static inline void aven_graph_plane_hartman_thread_lock(
    AvenGraphPlaneHartmanThreadCtx *thread_ctx
) {    
    for (;;) {
        if (
            !atomic_exchange_explicit(
                &thread_ctx->lock,
                true,
                memory_order_acquire
            )
        ) {
            return;
        }
        while (atomic_load_explicit(&thread_ctx->lock, memory_order_relaxed)) {
#if __has_builtin(__builtin_ia32_pause)
            __builtin_ia32_pause();
#endif
        }
    }
}

static inline void aven_graph_plane_hartman_thread_unlock(
    AvenGraphPlaneHartmanThreadCtx *thread_ctx
) {
    atomic_store_explicit(&thread_ctx->lock, false, memory_order_release);
}

static inline AvenGraphPlaneHartmanFrameOptional
aven_graph_plane_hartman_thread_next_frame(
    AvenGraphPlaneHartmanThreadCtx *ctx,
    uint32_t thread_index
) {
    (void)thread_index;

    int frames_active = atomic_fetch_sub_explicit(
        &ctx->frames_active,
        1,
        memory_order_relaxed
    ) + 1;

    for (;;) {
        size_t available_entries = atomic_load_explicit(
            &ctx->valid_entries.len,
            memory_order_relaxed
        );
        if (
            available_entries > 0 or
            frames_active == 0
        ) {
            aven_graph_plane_hartman_thread_lock(ctx);
            if (ctx->valid_entries.len > 0) {
                uint32_t entry_index = list_back(ctx->valid_entries);
                atomic_fetch_sub_explicit(
                    &ctx->valid_entries.len,
                    1,
                    memory_order_relaxed
                );
                AvenGraphPlaneHartmanFrame frame = pool_get(
                    ctx->entry_pool,
                    entry_index
                ).frame;
                pool_delete(ctx->entry_pool, entry_index);
                atomic_fetch_add_explicit(
                    &ctx->frames_active,
                    1,
                    memory_order_relaxed
                );
                aven_graph_plane_hartman_thread_unlock(ctx);

                return (AvenGraphPlaneHartmanFrameOptional){
                    .value = frame,
                    .valid = true,
                };
            }
            frames_active = atomic_load_explicit(
                &ctx->frames_active,
                memory_order_relaxed
            );

            if (ctx->entry_pool.used == 0 and frames_active == 0) {
                aven_graph_plane_hartman_thread_unlock(ctx);
                return (AvenGraphPlaneHartmanFrameOptional){ 0 };
            }

            aven_graph_plane_hartman_thread_unlock(ctx);
        }

#if __has_builtin(__builtin_ia32_pause)
        __builtin_ia32_pause();
#endif
        frames_active = atomic_load_explicit(
            &ctx->frames_active,
            memory_order_relaxed
        );
    }
}

static inline void aven_graph_plane_hartman_thread_push_entries(
    AvenGraphPlaneHartmanThreadCtx *ctx,
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

    aven_graph_plane_hartman_thread_lock(ctx);
    if (parent_index != 0 and (v_colors->len & 0x3) == 1) {
        do {
            assert(ctx->valid_entries.len < ctx->valid_entries.cap);
            atomic_fetch_add_explicit(
                &ctx->valid_entries.len,
                1,
                memory_order_relaxed
            );
            list_back(ctx->valid_entries) = parent_index - 1;
            parent_index = pool_get(ctx->entry_pool, parent_index - 1).parent;
        } while (parent_index != 0);
        v_colors->len = 1;
    }
    if (maybe_frame->valid) {
        uint32_t entry_index = (uint32_t)pool_create(ctx->entry_pool);
        pool_get(ctx->entry_pool, entry_index) =
            (AvenGraphPlaneHartmanThreadEntry){
                .frame = maybe_frame->value,
                .parent = parent_index,
            };
        if (v_colors->len == 1) {
            assert(ctx->valid_entries.len < ctx->valid_entries.cap);
            atomic_fetch_add_explicit(
                &ctx->valid_entries.len,
                1,
                memory_order_relaxed
            );
            list_back(ctx->valid_entries) = entry_index;
        } else {
            v_colors->len = (((entry_index + 1) << 2) | (v_colors->len & 0x3));
        }
    }
    aven_graph_plane_hartman_thread_unlock(ctx);
}

static bool aven_graph_plane_hartman_thread_has_color(
    AvenGraphPlaneHartmanList *list,
    uint32_t color
) {
    assert(list->len > 0);
    for (size_t i = 0; i < (list->len & 0x3); i += 1) {
        if (list->ptr[i] == color) {
            return true;
        }
    }

    return false;
}

static void aven_graph_plane_hartman_thread_remove_color(
    AvenGraphPlaneHartmanList *list,
    uint32_t color
) {
    for (size_t i = 0; i < (list->len & 0x3); i += 1) {
        if (list->ptr[i] == color) {
            assert((list->len & 0x3) > 1);
            list->ptr[i] = list->ptr[list->len - 1];
            list->len -= 1;
            break;
        }
    }
}

static inline AvenGraphPlaneHartmanVertex *
aven_graph_plane_hartman_thread_vinfo(
    AvenGraphPlaneHartmanThreadCtx *ctx,
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

static void aven_graph_plane_hartman_thread_color_differently(
    AvenGraphPlaneHartmanList *list,
    uint32_t color
) {
    for (size_t i = 0; i < (list->len & 0x3); i += 1) {
        if (list->ptr[i] != color) {
            list->ptr[0] = list->ptr[i];
            list->len = ((list->len & 0xfffffffc) | 1);
            break;
        }
    }
    assert(list->ptr[0] != color);
}

typedef struct {
    uint32_t next_mark;
    uint32_t final_mark;
    uint32_t block_size;
} AvenGraphPlaneHartmanThreadMarkSet;

static inline uint32_t aven_graph_plane_hartman_thread_next_mark(
    AvenGraphPlaneHartmanThreadCtx *ctx,
    AvenGraphPlaneHartmanThreadMarkSet *mark_set
) {
    if (mark_set->next_mark == mark_set->final_mark) {
        mark_set->next_mark = (uint32_t)atomic_fetch_add_explicit(
            &ctx->next_mark,
            mark_set->block_size,
            memory_order_relaxed
        );
        mark_set->final_mark = mark_set->next_mark + mark_set->block_size;
    }

    uint32_t next_mark = mark_set->next_mark;
    mark_set->next_mark += 1;
    return next_mark;
}

static inline bool aven_graph_plane_hartman_thread_frame_step(
    AvenGraphPlaneHartmanThreadCtx *ctx,
    AvenGraphPlaneHartmanFrame *frame,
    AvenGraphPlaneHartmanThreadMarkSet *mark_set
) {
    uint32_t z = frame->z;
    AvenGraphAugAdjList z_adj = get(ctx->graph, z);
    AvenGraphPlaneHartmanVertex *z_info = aven_graph_plane_hartman_thread_vinfo(
        ctx,
        frame,
        z
    );
    uint32_t zu_index = aven_graph_plane_hartman_thread_vinfo(
        ctx,
        frame,
        z
    )->nb.first;
    AvenGraphAugAdjListNode zu = get(z_adj, zu_index);

    AvenGraphPlaneHartmanList *z_colors = &get(ctx->color_lists, z);
    assert(z_colors->len == 1);
    uint32_t z_color = z_colors->ptr[0];

    bool last_neighbor = (zu_index == z_info->nb.last);
    if (!last_neighbor) {
        z_info->nb.first = aven_graph_aug_adj_next(z_adj, z_info->nb.first);
    }

    uint32_t u = zu.vertex;
    AvenGraphAugAdjList u_adj = get(ctx->graph, u);
    uint32_t uz_index = zu.back_index;
    AvenGraphPlaneHartmanVertex *u_info =
        aven_graph_plane_hartman_thread_vinfo(
            ctx,
            frame,
            u
        );
    uint32_t u_mark = u_info->mark;

    AvenGraphPlaneHartmanList *u_colors = &get(
        ctx->color_lists,
        u
    );

    bool done = false;

    AvenGraphPlaneHartmanFrameOptional maybe_frame;
    maybe_frame.valid = false;

    if (u_mark == 0) {
        aven_graph_plane_hartman_thread_remove_color(u_colors, z_color);
        assert(u_colors->len > 1);

        u_info->nb.first = aven_graph_aug_adj_next(u_adj, uz_index);
        u_info->nb.last = aven_graph_aug_adj_prev(u_adj, uz_index);
        u_info->mark = frame->x_info.mark;
    } else if (u_info->nb.first == u_info->nb.last) {
        if (u != frame->x and u != frame->y) {
            aven_graph_plane_hartman_thread_color_differently(
                u_colors,
                z_color
            );
        }

        done = true;
    } else if (
        u == frame->y or
        (u != frame->x and get(ctx->marks, u_mark) == frame->y_info.mark)
    ) {
        uint32_t z_nb_old_last = z_info->nb.last;
        uint32_t u_nb_old_first = u_info->nb.first;

        u_info->nb.first = aven_graph_aug_adj_next(u_adj, uz_index);

        if (aven_graph_plane_hartman_thread_has_color(u_colors, z_color)) {
            if (u_colors->len != 1) {
                u_colors->ptr[0] = z_color;
                u_colors->len = ((u_colors->len & 0xfffffffc) | 1);
            }
            frame->z = u;
            frame->z_info = *u_info;
        } else {
            if (z != frame->x) {
                get(ctx->marks, frame->x_info.mark) = frame->y_info.mark;
                frame->z = frame->x;
                frame->x_info.mark = aven_graph_plane_hartman_thread_next_mark(
                    ctx,
                    mark_set
                );
            }
        }

        if (u_nb_old_first != uz_index) {
            uint32_t new_x_mark = aven_graph_plane_hartman_thread_next_mark(
                ctx,
                mark_set
            );

            uint32_t new_y_mark = aven_graph_plane_hartman_thread_next_mark(
                ctx,
                mark_set
            );

            if (u_info->nb.last == uz_index) {
                if (u != frame->y) { 
                    u_colors->len = ((u_colors->len & 0xfffffffc) | 1);
                }

                frame->y = z;
                frame->z = u;
                frame->x = u;
                frame->x_info = (AvenGraphPlaneHartmanVertex){
                    .mark = new_x_mark,
                    .nb = { .first = u_nb_old_first, .last = uz_index },
                };
                frame->y_info = (AvenGraphPlaneHartmanVertex){
                    .mark = new_y_mark,
                    .nb = { .first = zu_index, .last = z_nb_old_last },
                };
            } else {
                maybe_frame.value = (AvenGraphPlaneHartmanFrame){
                    .z = u,
                    .x = u,
                    .y = z,
                    .x_info = {
                        .mark = new_x_mark,
                        .nb = { .first = u_nb_old_first, .last = uz_index },
                    },
                    .y_info = {
                        .mark = new_y_mark,
                        .nb = { .first = zu_index, .last = z_nb_old_last },
                    },
                };
                maybe_frame.valid = true;
            }
        }
    } else if (z == frame->x) {
        aven_graph_plane_hartman_thread_color_differently(u_colors, z_color);

        assert(uz_index == u_info->nb.last);

        u_info->nb.last = aven_graph_aug_adj_prev(u_adj, uz_index);
        u_info->mark = aven_graph_plane_hartman_thread_next_mark(
            ctx,
            mark_set
        );

        if (z == frame->y) {
            frame->y_info = frame->x_info;
        } else {
            frame->z_info = frame->x_info;
        }

        frame->x = u;
        frame->x_info = *u_info;
    } else if (
        u == frame->x or
        u_mark == frame->x_info.mark
    ) {
        assert(
            !aven_graph_plane_hartman_thread_has_color(
                u_colors,
                z_color
            )
        );

        if (u_info->nb.last != uz_index) {
            AvenGraphPlaneHartmanVertex new_u_info = {
                .mark = u_info->mark,
                .nb = {
                    .first = aven_graph_aug_adj_next(u_adj, uz_index),
                    .last = u_info->nb.last,
                },
            };
            maybe_frame.value = (AvenGraphPlaneHartmanFrame){
                .z = u,
                .x = u,
                .y = u,
                .x_info = new_u_info,
                .y_info = new_u_info,
            };
            maybe_frame.valid = true;

            u_info->nb.last = aven_graph_aug_adj_prev(u_adj, uz_index);

            if (u == frame->x) {
                frame->x_info.mark = aven_graph_plane_hartman_thread_next_mark(
                    ctx,
                    mark_set
                );
            }

            done = last_neighbor;
        } else {
            u_info->nb.last = aven_graph_aug_adj_prev(u_adj, uz_index);
        }
    } else {
        aven_graph_plane_hartman_thread_color_differently(u_colors, z_color);

        if (u_info->nb.first != uz_index) {
            assert(!last_neighbor);
            u_info->mark = frame->x_info.mark;
            uint32_t next_mark = aven_graph_plane_hartman_thread_next_mark(
                ctx,
                mark_set
            );
            maybe_frame.value = (AvenGraphPlaneHartmanFrame){
                .z = frame->x,
                .x = frame->x,
                .y = u,
                .x_info = {
                    .nb = frame->x_info.nb,
                    .mark = next_mark,
                },
                .y_info = {
                    .mark = u_info->mark,
                    .nb = {
                        .first = aven_graph_aug_adj_next(u_adj, uz_index),
                        .last = u_info->nb.last,
                    },
                },
            };
            maybe_frame.valid = true;

            u_info->nb.last = aven_graph_aug_adj_prev(u_adj, uz_index);
            u_info->mark = aven_graph_plane_hartman_thread_next_mark(
                ctx,
                mark_set
            );

            frame->x = u;
            frame->x_info = *u_info;
        } else {
            assert(last_neighbor);
            assert(frame->y == z);

            u_info->nb.first = aven_graph_aug_adj_next(u_adj, uz_index);
            u_info->mark = frame->x_info.mark;

            frame->y = u;
            frame->y_info = *u_info;

            frame->z = frame->x;
        }
    }

    aven_graph_plane_hartman_thread_push_entries(
        ctx,
        u_colors,
        &maybe_frame
    );

    return done;
}

typedef struct {
    AvenGraphPlaneHartmanThreadCtx *thread_ctx;
    uint32_t thread_index;
} AvenGraphPlaneHartmanThreadWorker;

static inline void aven_graph_plane_hartman_thread_worker(void *args) {
    AvenGraphPlaneHartmanThreadWorker *worker = args;

    atomic_fetch_add_explicit(
        &worker->thread_ctx->frames_active,
        1,
        memory_order_relaxed
    );

    AvenGraphPlaneHartmanFrameOptional frame =
        aven_graph_plane_hartman_thread_next_frame(
            worker->thread_ctx,
            worker->thread_index
        );

    AvenGraphPlaneHartmanThreadMarkSet mark_set = {
        .block_size = min(
            2048,
            (uint32_t)worker->thread_ctx->graph.len
        ),
    };
    while (frame.valid) {
        while (
            !aven_graph_plane_hartman_thread_frame_step(
                worker->thread_ctx,
                &frame.value,
                &mark_set
            )
        ) {}
        frame = aven_graph_plane_hartman_thread_next_frame(
            worker->thread_ctx,
            worker->thread_index
        );
    }
}

static inline void aven_graph_plane_hartman_thread(
    AvenGraphPlaneHartmanListProp color_lists,
    AvenGraph graph,
    AvenGraphSubset outer_face,
    AvenThreadPool *thread_pool,
    AvenArena arena
) {
    AvenGraphAug aug_graph = aven_graph_aug(graph, &arena);
    AvenGraphPlaneHartmanThreadCtx ctx = aven_graph_plane_hartman_thread_init(
        color_lists,
        aug_graph,
        outer_face,
        thread_pool->workers.len + 1,
        &arena
    );

    Slice(AvenGraphPlaneHartmanThreadWorker) workers = aven_arena_create_slice(
        AvenGraphPlaneHartmanThreadWorker,
        &arena,
        thread_pool->workers.len + 1
    );

    for (uint32_t i = 0; i < thread_pool->workers.len + 1; i += 1) {
        get(workers, i) = (AvenGraphPlaneHartmanThreadWorker) {
            .thread_ctx = &ctx,
            .thread_index = i
        };
    }

    for (uint32_t i = 0; i < thread_pool->workers.len; i += 1) {
        aven_thread_pool_submit(
            thread_pool,
            aven_graph_plane_hartman_thread_worker,
            &get(workers, i + 1)
        );
    }

    aven_graph_plane_hartman_thread_worker(&get(workers, 0));

    aven_thread_pool_wait(thread_pool);
}

#endif // AVEN_GRAPH_PLANE_HARTMAN_THREAD_H
