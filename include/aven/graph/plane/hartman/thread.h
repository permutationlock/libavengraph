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

int printf(const char *fmt, ...);

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
        aven_panic("vertices won't fit in 30 bits");
    }

    AvenGraphPlaneHartmanThreadCtx ctx = {
        .graph = graph,
        .color_lists = color_lists,
        .nthreads = nthreads,
        .entry_pool = aven_arena_create_pool(
            AvenGraphPlaneHartmanThreadEntry,
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
    atomic_init(&ctx.valid_entries.len, 0);
    atomic_init(&ctx.frames_active, 0);
    atomic_init(&ctx.next_mark, 1);
    atomic_init(&ctx.lock, false);

    for (uint32_t v = 0; v < ctx.vertex_info.len; v += 1) {
        slice_get(ctx.vertex_info, v) = (AvenGraphPlaneHartmanVertex){ 0 };
    }

    for (uint32_t i = 0; i < ctx.marks.len; i += 1) {
        slice_get(ctx.marks, i) = i;
    }

    uint32_t face_mark = (uint32_t)ctx.next_mark;
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
    xyv_info->mark = (uint32_t)ctx.next_mark;
    ctx.next_mark += 1;

    AvenGraphPlaneHartmanList *xyv_colors = &slice_get(ctx.color_lists, xyv);
    assert(xyv_colors->len > 0);
    xyv_colors->len = 1;

    uint32_t entry_index = (uint32_t)pool_create(ctx.entry_pool);
    pool_get(ctx.entry_pool, entry_index) = (AvenGraphPlaneHartmanThreadEntry){
        .frame = {
            .v = xyv,
            .x = xyv,
            .y = xyv,
            .x_info = *xyv_info,
            .y_info = *xyv_info,
        },
    };
    list_push(ctx.valid_entries) = entry_index;

    //thread_spin_init(&ctx.lock, THREAD_PROCESS_PRIVATE);

    return ctx;
}

static inline void aven_graph_plane_hartman_thread_lock(
    AvenGraphPlaneHartmanThreadCtx *thread_ctx
) {    
    //thread_spin_lock(&thread_ctx->lock);
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
    //thread_spin_unlock(&thread_ctx->lock);
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
    // printf("%u - looking for new frame\n", thread_index);

    for (;;) {
        size_t available_entries = atomic_load_explicit(
            &ctx->valid_entries.len,
            memory_order_relaxed
        );
        if (
            available_entries > 0 or
            frames_active == 0
        ) {
            // printf("%u - done waiting: %u / %lu\n", thread_index, thread_ctx->new_frames, ctx->frames.len);
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
                // printf("%u - killed\n", thread_index);
                aven_graph_plane_hartman_thread_unlock(ctx);
                return (AvenGraphPlaneHartmanFrameOptional){ 0 };
            }

            aven_graph_plane_hartman_thread_unlock(ctx);
        }

        // printf("%u - waiting\n", thread_index);
#if __has_builtin(__builtin_ia32_pause)
        __builtin_ia32_pause();
#endif
        frames_active = atomic_load_explicit(
            &ctx->frames_active,
            memory_order_relaxed
        );

        // printf("%u - done waiting\n", thread_index);
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
        if (list->data[i] == color) {
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
        if (list->data[i] == color) {
            assert((list->len & 0x3) > 1);
            list->data[i] = list->data[list->len - 1];
            list->len -= 1;
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

    if (v == frame->v) {
        return &frame->v_info;
    }

    return &slice_get(ctx->vertex_info, v);
}

static void aven_graph_plane_hartman_thread_color_differently(
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
    // printf("v: %u, x: %u, y: %u\n", frame->v, frame->x, frame->y);
    uint32_t v = frame->v;
    AvenGraphAugAdjList v_adj = slice_get(ctx->graph, frame->v);
    AvenGraphPlaneHartmanVertex *v_info = aven_graph_plane_hartman_thread_vinfo(
        ctx,
        frame,
        v
    );
    uint32_t vu_index = aven_graph_plane_hartman_thread_vinfo(
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
        aven_graph_plane_hartman_thread_vinfo(
            ctx,
            frame,
            u
        );
    uint32_t u_mark = u_info->mark;

    AvenGraphPlaneHartmanList *u_colors = &slice_get(
        ctx->color_lists,
        u
    );

    bool done = false;

    AvenGraphPlaneHartmanFrameOptional maybe_frame;
    maybe_frame.valid = false;

    if (u_mark == 0) {
        aven_graph_plane_hartman_thread_remove_color(u_colors, v_color);
        assert(u_colors->len > 1);

        u_info->nb.first = (uint32_t)((uv_index + 1) % u_adj.len);
        u_info->nb.last =(uint32_t)((uv_index + u_adj.len - 1) % u_adj.len);
        u_info->mark = frame->x_info.mark;
    } else if (u_info->nb.first == u_info->nb.last) {
        if (u != frame->x and u != frame->y) {
            aven_graph_plane_hartman_thread_color_differently(
                u_colors,
                v_color
            );
        }

        done = true;
    } else if (
        u == frame->y or
        (u != frame->x and slice_get(ctx->marks, u_mark) == frame->y_info.mark)
    ) {
        uint32_t v_nb_old_last = v_info->nb.last;
        uint32_t u_nb_old_first = u_info->nb.first;

        u_info->nb.first = (uint32_t)((uv_index + 1) % u_adj.len);

        if (aven_graph_plane_hartman_thread_has_color(u_colors, v_color)) {
            if (u_colors->len != 1) {
                u_colors->data[0] = v_color;
                u_colors->len = ((u_colors->len & 0xfffffffc) | 1);
            }
            frame->v = u;
            frame->v_info = *u_info;
        } else {
            if (v != frame->x) {
                slice_get(ctx->marks, frame->x_info.mark) = frame->y_info.mark;
                frame->v = frame->x;
                frame->x_info.mark = aven_graph_plane_hartman_thread_next_mark(
                    ctx,
                    mark_set
                );
            }
        }

        if (u_nb_old_first != uv_index) {
            uint32_t new_x_mark = aven_graph_plane_hartman_thread_next_mark(
                ctx,
                mark_set
            );

            uint32_t new_y_mark = aven_graph_plane_hartman_thread_next_mark(
                ctx,
                mark_set
            );

            if (u_info->nb.last == uv_index) {
                if (u != frame->y) { 
                    u_colors->len = ((u_colors->len & 0xfffffffc) | 1);
                }

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
        aven_graph_plane_hartman_thread_color_differently(u_colors, v_color);

        assert(uv_index == u_info->nb.last);

        u_info->nb.last = (uint32_t)((uv_index + u_adj.len - 1) % u_adj.len);
        u_info->mark = aven_graph_plane_hartman_thread_next_mark(
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
    } else if (
        u == frame->x or
        u_mark == frame->x_info.mark
    ) {
        assert(
            !aven_graph_plane_hartman_thread_has_color(
                u_colors,
                v_color
            )
        );

        if (u_info->nb.last != uv_index) {
            AvenGraphPlaneHartmanVertex new_u_info = {
                .mark = u_info->mark,
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

            if (u == frame->x) {
                frame->x_info.mark = aven_graph_plane_hartman_thread_next_mark(
                    ctx,
                    mark_set
                );
            }

            done = last_neighbor;
        } else {
            u_info->nb.last = (uint32_t)(
                (uv_index + u_adj.len - 1) % u_adj.len
            );
        }
    } else {
        aven_graph_plane_hartman_thread_color_differently(u_colors, v_color);

        if (u_info->nb.first != uv_index) {
            assert(!last_neighbor);
            u_info->mark = frame->x_info.mark;
            uint32_t next_mark = aven_graph_plane_hartman_thread_next_mark(
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
            u_info->mark = aven_graph_plane_hartman_thread_next_mark(
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
        slice_get(workers, i) = (AvenGraphPlaneHartmanThreadWorker) {
            .thread_ctx = &ctx,
            .thread_index = i
        };
    }

    for (uint32_t i = 0; i < thread_pool->workers.len; i += 1) {
        aven_thread_pool_submit(
            thread_pool,
            aven_graph_plane_hartman_thread_worker,
            &slice_get(workers, i + 1)
        );
    }

    aven_graph_plane_hartman_thread_worker(&slice_get(workers, 0));

    aven_thread_pool_wait(thread_pool);
}

#endif // AVEN_GRAPH_PLANE_HARTMAN_THREAD_H
