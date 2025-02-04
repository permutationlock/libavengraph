#ifndef GRAPH_PLANE_HARTMAN_THREAD_H
#define GRAPH_PLANE_HARTMAN_THREAD_H

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
    GraphAugAdjList adj;
    GraphPlaneHartmanVertexLoc loc;
    GraphPlaneHartmanList colors;
    uint32_t entry_index;
} GraphPlaneHartmanThreadVertex;

typedef List(GraphPlaneHartmanFrame)
    GraphPlaneHartmanThreadFrameList;

typedef struct {
    GraphPlaneHartmanFrame frame;
    uint32_t parent;
} GraphPlaneHartmanThreadEntry;

typedef struct {
    uint32_t *ptr;
    atomic_size_t len;
    size_t cap;
} GraphPlaneHartmanThreadUint32List;

typedef struct {
    Slice(GraphPlaneHartmanThreadVertex) vertex_info;
    Slice(uint32_t) marks;
    GraphPlaneHartmanThreadUint32List valid_entries;
    Pool(GraphPlaneHartmanThreadEntry) entry_pool;
    size_t nthreads;
    atomic_int threads_active;
    atomic_int frames_active;
    atomic_uint_least32_t next_mark;
    atomic_bool lock;
} GraphPlaneHartmanThreadCtx;

static inline GraphPlaneHartmanThreadCtx
graph_plane_hartman_thread_init(
    GraphAug graph,
    GraphPlaneHartmanListProp color_lists,
    GraphSubset cwise_outer_face,
    size_t nthreads,
    AvenArena *arena
) {
   GraphPlaneHartmanThreadCtx ctx = {
        .vertex_info = { .len = graph.len },
        // Each vertex receives a new unique mark at most 4 times:
        //  - when added to the outer face
        //  - when assigned to be the new x
        //  - when assigned to be the new y
        //  - when swapped x to y or y to x (removed afterwards so happens once)
        .marks = { .len = nthreads * 4 * graph.len },
        // A new frame only occurs when splitting across an edge
        .entry_pool = { .cap = 3 * graph.len - 6 },
        .valid_entries = { .cap = 3 * graph.len - 6 },
        .nthreads = nthreads,
        .next_mark = 1,
    };

    ctx.vertex_info.ptr = aven_arena_create_array(
        GraphPlaneHartmanThreadVertex,
        arena,
        ctx.vertex_info.len
    );
    ctx.marks.ptr = aven_arena_create_array(
        uint32_t,
        arena,
        ctx.marks.len
    );
    ctx.entry_pool.ptr = (void *)aven_arena_create_array(
        PoolEntry(GraphPlaneHartmanThreadEntry),
        arena,
        ctx.entry_pool.cap
    );
    ctx.valid_entries.ptr = aven_arena_create_array(
        uint32_t,
        arena,
        ctx.valid_entries.cap
    );

    atomic_init(&ctx.valid_entries.len, 0);
    atomic_init(&ctx.threads_active, 0);
    atomic_init(&ctx.frames_active, 0);
    atomic_init(&ctx.next_mark, 1);
    atomic_init(&ctx.lock, false);

    for (uint32_t v = 0; v < ctx.vertex_info.len; v += 1) {
        get(ctx.vertex_info, v) = (GraphPlaneHartmanThreadVertex){
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
        GraphAugAdjList v_adj = get(ctx.vertex_info, v).adj;
        
        uint32_t vu_index = graph_aug_adj_neighbor_index(v_adj, u);
        uint32_t uv_index = get(v_adj, vu_index).back_index;
        
        get(ctx.vertex_info, v).loc.nb.first = vu_index;
        get(ctx.vertex_info, u).loc.nb.last = uv_index;

        get(ctx.vertex_info, v).loc.mark = face_mark;

        u = v;
    }

    uint32_t xyv = get(cwise_outer_face, 0);
    GraphPlaneHartmanVertexLoc *xyv_loc = &get(ctx.vertex_info, xyv).loc;
    xyv_loc->mark = ctx.next_mark++;

    GraphPlaneHartmanList *xyv_colors = &get(ctx.vertex_info, xyv).colors;
    assert(xyv_colors->len > 0);
    xyv_colors->len = 1;

    uint32_t entry_index = (uint32_t)pool_create(ctx.entry_pool);
    pool_get(ctx.entry_pool, entry_index) = (GraphPlaneHartmanThreadEntry){
        .frame = {
            .z = xyv,
            .x = xyv,
            .y = xyv,
            .x_loc = *xyv_loc,
        },
    };
    list_push(ctx.valid_entries) = entry_index;

    return ctx;
}

static inline void graph_plane_hartman_thread_lock(
    GraphPlaneHartmanThreadCtx *thread_ctx
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

static inline void graph_plane_hartman_thread_unlock(
    GraphPlaneHartmanThreadCtx *thread_ctx
) {
    atomic_store_explicit(&thread_ctx->lock, false, memory_order_release);
}

static inline void graph_plane_hartman_thread_pop_internal(
    GraphPlaneHartmanThreadCtx *ctx,
    GraphPlaneHartmanThreadFrameList *local_frames
) {
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
        if (available_entries > 0 or frames_active == 0) {
            graph_plane_hartman_thread_lock(ctx);
            available_entries = atomic_load_explicit(
                &ctx->valid_entries.len,
                memory_order_relaxed
            );
            if (available_entries > 0) {
                size_t frames_moved = min(
                    local_frames->cap / 2,
                    available_entries
                );
                size_t valid_index = atomic_fetch_sub_explicit(
                    &ctx->valid_entries.len,
                    frames_moved,
                    memory_order_relaxed
                ) - frames_moved;
                for (size_t i = 0; i < frames_moved; i += 1) {
                    uint32_t entry_index = ctx->valid_entries.ptr[
                        valid_index + i
                    ];
                    list_push(*local_frames) = pool_get(
                        ctx->entry_pool,
                        entry_index
                    ).frame;
                    pool_delete(ctx->entry_pool, entry_index);
                }
                atomic_fetch_add_explicit(
                    &ctx->frames_active,
                    1,
                    memory_order_relaxed
                );
                graph_plane_hartman_thread_unlock(ctx);

                return;
            }
            frames_active = atomic_load_explicit(
                &ctx->frames_active,
                memory_order_relaxed
            );

            if (ctx->entry_pool.used == 0 and frames_active == 0) {
                graph_plane_hartman_thread_unlock(ctx);
                return;
            }

            graph_plane_hartman_thread_unlock(ctx);
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

static inline void graph_plane_hartman_thread_push_entries(
    GraphPlaneHartmanThreadCtx *ctx,
    GraphPlaneHartmanThreadFrameList *local_frames,
    uint32_t v,
    GraphPlaneHartmanFrameOptional *maybe_frame,
    uint32_t u
) {
    GraphPlaneHartmanThreadVertex *v_info = &get(ctx->vertex_info, v);
    GraphPlaneHartmanThreadVertex *u_info = &get(ctx->vertex_info, u);
    bool v_push = (v_info->entry_index != 0) and (v_info->colors.len == 1);
    bool u_push = (v != u) and
        (u_info->entry_index != 0) and
        (u_info->colors.len == 1);
    bool frame_wait = (maybe_frame->valid) and (v_info->colors.len != 1);

    if (
        v_push or
        u_push or
        frame_wait or
        local_frames->len == local_frames->cap
    ) {
        graph_plane_hartman_thread_lock(ctx);
        if (local_frames->len > (local_frames->cap / 2)) {
            size_t frames_over = local_frames->len - (local_frames->cap / 2);
            size_t len = atomic_fetch_add_explicit(
                &ctx->valid_entries.len,
                frames_over,
                memory_order_relaxed
            );
            assert((len + frames_over) <= ctx->valid_entries.cap);
            for (
                size_t i = 0;
                i < frames_over;
                i += 1
            ) {
                uint32_t entry_index = (uint32_t)pool_create(ctx->entry_pool);
                pool_get(ctx->entry_pool, entry_index) =
                    (GraphPlaneHartmanThreadEntry){
                        .frame = list_pop(*local_frames),
                    };
                ctx->valid_entries.ptr[len + i] = entry_index;
            }
        }
        if (v_push) {
            do {
                assert(ctx->valid_entries.len < ctx->valid_entries.cap);
                size_t back_index = atomic_fetch_add_explicit(
                    &ctx->valid_entries.len,
                    1,
                    memory_order_relaxed
                );
                ctx->valid_entries.ptr[back_index] = v_info->entry_index - 1;
                v_info->entry_index = pool_get(
                    ctx->entry_pool,
                    v_info->entry_index - 1
                ).parent;
            } while (v_info->entry_index != 0);
        }
        if (u_push) {
            do {
                assert(ctx->valid_entries.len < ctx->valid_entries.cap);
                size_t back_index = atomic_fetch_add_explicit(
                    &ctx->valid_entries.len,
                    1,
                    memory_order_relaxed
                );
                ctx->valid_entries.ptr[back_index] = u_info->entry_index - 1;
                u_info->entry_index = pool_get(
                    ctx->entry_pool,
                    u_info->entry_index - 1
                ).parent;
            } while (u_info->entry_index != 0);
        }
        if (frame_wait) {
            uint32_t entry_index = (uint32_t)pool_create(ctx->entry_pool);
            pool_get(ctx->entry_pool, entry_index) =
                (GraphPlaneHartmanThreadEntry){
                    .frame = maybe_frame->value,
                    .parent = v_info->entry_index,
                };
            v_info->entry_index = entry_index + 1;
        }
        graph_plane_hartman_thread_unlock(ctx);
    }
    if (maybe_frame->valid and !frame_wait) {
        list_push(*local_frames) = maybe_frame->value;
    }
}

static inline GraphPlaneHartmanVertexLoc *
graph_plane_hartman_thread_vloc(
    GraphPlaneHartmanThreadCtx *ctx,
    GraphPlaneHartmanFrame *frame,
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

typedef struct {
    uint32_t next_mark;
    uint32_t final_mark;
    uint32_t block_size;
} GraphPlaneHartmanThreadMarkSet;

static inline uint32_t graph_plane_hartman_thread_next_mark(
    GraphPlaneHartmanThreadCtx *ctx,
    GraphPlaneHartmanThreadMarkSet *mark_set
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

static inline bool graph_plane_hartman_thread_frame_step(
    GraphPlaneHartmanThreadCtx *ctx,
    GraphPlaneHartmanThreadFrameList *local_frames,
    GraphPlaneHartmanThreadMarkSet *mark_set,
    GraphPlaneHartmanFrame *frame
) {
    GraphAugAdjList z_adj = get(ctx->vertex_info, frame->z).adj;
    GraphPlaneHartmanVertexLoc *z_loc =
        graph_plane_hartman_thread_vloc(ctx, frame, frame->z);
    GraphPlaneHartmanList *z_colors = &get(
        ctx->vertex_info,
        frame->z
    ).colors;
    uint8_t z_color = get(*z_colors, 0);

    uint32_t zu_index = z_loc->nb.first;
    GraphAugAdjListNode zu = get(z_adj, zu_index);

    uint32_t u = zu.vertex;
    GraphPlaneHartmanVertexLoc *u_loc =
        graph_plane_hartman_thread_vloc(ctx, frame, u);

    if (zu_index == z_loc->nb.last) {
        if (frame->x == frame->y) {
            assert(frame->z == frame->x);
            graph_plane_hartman_color_differently(
                &get(ctx->vertex_info, u).colors,
                z_color
            );
            graph_plane_hartman_thread_push_entries(
                ctx,
                local_frames,
                u,
                &(GraphPlaneHartmanFrameOptional){ 0 },
                u
            );
        }
        return true;
    }

    if (u == frame->y) {
        assert(frame->z == frame->x);

        GraphPlaneHartmanVertexLoc x_loc = frame->x_loc;
        frame->x_loc = frame->y_loc;
        frame->y_loc = x_loc;
        frame->y = frame->x;
        frame->x = u;
        frame->z = u;

        frame->x_loc.mark = graph_plane_hartman_thread_next_mark(
            ctx,
            mark_set
        );
        frame->y_loc.mark = graph_plane_hartman_thread_next_mark(
            ctx,
            mark_set
        );

        return false;
    }

    GraphAugAdjList u_adj = get(ctx->vertex_info, u).adj;
    u_loc->nb.last = graph_aug_adj_prev(u_adj, u_loc->nb.last);
    z_loc->nb.first = graph_aug_adj_next(z_adj, z_loc->nb.first);

    bool u_colored = false;

    if (frame->z == frame->x) {
        graph_plane_hartman_color_differently(
            &get(ctx->vertex_info, u).colors,
            z_color
        );
        u_colored = true;

        if (frame->z == frame->y) {
            frame->y_loc = frame->x_loc;
        } else {
            frame->z_loc = frame->x_loc;
        }

        frame->x = u;
        frame->x_loc = *u_loc;
        frame->x_loc.mark = graph_plane_hartman_thread_next_mark(
            ctx,
            mark_set
        );

        u_loc = graph_plane_hartman_thread_vloc(ctx, frame, u);
        z_loc = graph_plane_hartman_thread_vloc(ctx, frame, frame->z);
    }

    uint32_t zv_index = graph_aug_adj_next(z_adj, zu_index);
    GraphAugAdjListNode zv = get(z_adj, zv_index);

    uint32_t v = zv.vertex;
    GraphAugAdjList v_adj = get(ctx->vertex_info, v).adj;
    GraphPlaneHartmanVertexLoc *v_loc =
        graph_plane_hartman_thread_vloc(ctx, frame, v);
    GraphPlaneHartmanList *v_colors = &get(ctx->vertex_info, v).colors;

    GraphPlaneHartmanFrameOptional maybe_frame = { 0 };

    if (v_loc->mark == 0) {
        *v_loc = (GraphPlaneHartmanVertexLoc){
            .mark = frame->x_loc.mark,
            .nb = {
                .first = graph_aug_adj_next(v_adj, zv.back_index),
                .last = zv.back_index,
            },
        };
        graph_plane_hartman_remove_color(v_colors, z_color);
    } else if (v_loc->mark == frame->x_loc.mark) {
        if (zv_index == z_loc->nb.last) {
            assert(frame->z == frame->y);
            assert(v == frame->x);
            v_loc->nb.first = graph_aug_adj_next(v_adj, zv.back_index),
            v_loc->mark = graph_plane_hartman_thread_next_mark(
                ctx,
                mark_set
            );
            frame->y = frame->x;
            frame->z = frame->x;
        } else {
            uint32_t new_mark = graph_plane_hartman_thread_next_mark(
                ctx,
                mark_set
            );
            maybe_frame.valid = true;
            maybe_frame.value = (GraphPlaneHartmanFrame){
                .x = v,
                .y = v,
                .z = v,
                .x_loc = {
                    .mark = new_mark,
                    .nb = {
                        .first = graph_aug_adj_next(v_adj, zv.back_index),
                        .last = v_loc->nb.last,
                    },
                },
            };
            v_loc->nb.last = zv.back_index;

            if (v == frame->x) {
                v_loc->mark = graph_plane_hartman_thread_next_mark(
                    ctx,
                    mark_set
                );
            }
        }
    } else if (get(ctx->marks, v_loc->mark) == frame->y_loc.mark) {
        if (v_loc->nb.first != zv.back_index) {
            uint32_t new_mark = graph_plane_hartman_thread_next_mark(
                ctx,
                mark_set
            );
            maybe_frame.valid = true;
            maybe_frame.value = (GraphPlaneHartmanFrame){
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

        v_loc->nb.first = graph_aug_adj_next(v_adj, zv.back_index);

        if (graph_plane_hartman_has_color(v_colors, z_color)) {
            if (v_colors->len > 1) {
                get(*v_colors, 0) = z_color;
                v_colors->len = 1;
            }

            frame->z = v;
            frame->z_loc = *v_loc;
        } else {
            get(ctx->marks, frame->x_loc.mark) = frame->y_loc.mark;
            frame->z = frame->x;
        }
    } else {
        graph_plane_hartman_color_differently(v_colors, z_color);
        if (v_loc->nb.first != zv.back_index) {
            maybe_frame.valid = true;
            maybe_frame.value = (GraphPlaneHartmanFrame){
                .x = frame->x,
                .y = v,
                .z = frame->x,
                .x_loc = frame->x_loc,
                .y_loc = {
                    .mark = frame->x_loc.mark,
                    .nb = {
                        .first = graph_aug_adj_next(v_adj, zv.back_index),
                        .last = v_loc->nb.last,
                    },
                },
            };

            v_loc->nb.last = zv.back_index;
            frame->x = v;
            frame->x_loc = *v_loc;
            frame->x_loc.mark = graph_plane_hartman_thread_next_mark(
                ctx,
                mark_set
            );
        } else {
            assert(frame->z == frame->y);
            v_loc->nb.first = graph_aug_adj_next(v_adj, zv.back_index),
            v_loc->mark = frame->x_loc.mark;
            frame->y = v;
            frame->y_loc = *v_loc;

            frame->z = frame->x;
        }
    }

    graph_plane_hartman_thread_push_entries(
        ctx,
        local_frames,
        v,
        &maybe_frame,
        u_colored ? u : v
    );

    return false;
}

typedef struct {
    GraphPropUint8 coloring;
    GraphPlaneHartmanThreadCtx *thread_ctx;
    uint32_t start_vertex;
    uint32_t end_vertex;
    uint32_t thread_index;
} GraphPlaneHartmanThreadWorker;

static inline void graph_plane_hartman_thread_worker(void *args) {
    GraphPlaneHartmanThreadWorker *worker = args;
    GraphPlaneHartmanThreadCtx *ctx = worker->thread_ctx;

    atomic_fetch_add_explicit(
        &ctx->threads_active,
        1,
        memory_order_relaxed
    );
    atomic_fetch_add_explicit(
        &ctx->frames_active,
        1,
        memory_order_relaxed
    );

    GraphPlaneHartmanFrame local_frame_data[16];
    GraphPlaneHartmanThreadFrameList local_frames = list_array(
        local_frame_data
    );

    GraphPlaneHartmanThreadMarkSet mark_set = {
        .block_size = min(
            256,
            (uint32_t)ctx->vertex_info.len
        ),
    };

    graph_plane_hartman_thread_pop_internal(ctx, &local_frames);

    while (local_frames.len > 0) {
        GraphPlaneHartmanFrame cur_frame = list_pop(local_frames);
        while (
            !graph_plane_hartman_thread_frame_step(
                ctx,
                &local_frames,
                &mark_set,
                &cur_frame
            )
        ) {}

        if (local_frames.len == 0) {
            graph_plane_hartman_thread_pop_internal(ctx, &local_frames);
        }
    }

    // synchronize all threads writes to the marks array
    atomic_fetch_sub_explicit(
        &ctx->threads_active,
        1,
        memory_order_release
    );

    for (;;) {
        int threads_active = atomic_load_explicit(
            &ctx->threads_active,
            memory_order_acquire
        );
        if (threads_active == 0) {
            break;
        }
        while (threads_active != 0) {
            threads_active = atomic_load_explicit(
                &ctx->threads_active,
                memory_order_relaxed
            );
        }
    }

    for (uint32_t v = worker->start_vertex; v != worker->end_vertex; v += 1) {
        assert(get(ctx->vertex_info, v).colors.len == 1);
        get(worker->coloring, v) = get(
            get(ctx->vertex_info, v).colors,
            0
        );
    }
}

static inline GraphPropUint8 graph_plane_hartman_thread(
    GraphAug aug_graph,
    GraphPlaneHartmanListProp color_lists,
    GraphSubset outer_face,
    AvenThreadPool *thread_pool,
    size_t nthreads,
    AvenArena *arena
) {
    GraphPropUint8 coloring = { .len = aug_graph.len };
    coloring.ptr = aven_arena_create_array(uint8_t, arena, coloring.len);

    AvenArena temp_arena = *arena;
    GraphPlaneHartmanThreadCtx ctx = graph_plane_hartman_thread_init(
        aug_graph,
        color_lists,
        outer_face,
        nthreads,
        &temp_arena
    );

    Slice(GraphPlaneHartmanThreadWorker) workers = aven_arena_create_slice(
        GraphPlaneHartmanThreadWorker,
        &temp_arena,
        nthreads
    );

    uint32_t chunk_size = (uint32_t)(aug_graph.len / workers.len);
    for (uint32_t i = 0; i < workers.len; i += 1) {
        uint32_t start_vertex = i * chunk_size;
        uint32_t end_vertex = (i + 1) * chunk_size;
        if (i + 1 == workers.len) {
            end_vertex = (uint32_t)aug_graph.len;
        }

        get(workers, i) = (GraphPlaneHartmanThreadWorker) {
            .coloring = coloring,
            .thread_ctx = &ctx,
            .thread_index = i,
            .start_vertex = start_vertex,
            .end_vertex = end_vertex,
        };
    }

    for (uint32_t i = 0; i < workers.len - 1; i += 1) {
        aven_thread_pool_submit(
            thread_pool,
            graph_plane_hartman_thread_worker,
            &get(workers, i + 1)
        );
    }

    graph_plane_hartman_thread_worker(&get(workers, 0));

    aven_thread_pool_wait(thread_pool);

    return coloring;
}

#endif // GRAPH_PLANE_HARTMAN_THREAD_H
