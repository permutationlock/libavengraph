#ifndef AVEN_THREAD_POOL_H
#define AVEN_THREAD_POOL_H

#ifdef _MSC_VER
    #error "pthreads not supported in MSVC"
#endif

#include <aven.h>
#include <aven/arena.h>

#include <pthread.h>

typedef void (*AvenThreadPoolJobFn)(void *);

typedef struct {
    AvenThreadPoolJobFn fn;
    void *args;
} AvenThreadPoolJob;

typedef struct {
    Slice(AvenThreadPoolJob) job_indices;
    size_t front;
    size_t back;
    size_t len;
} AvenThreadPoolQueue;

typedef struct {
    AvenThreadPoolQueue job_queue;
    Slice(pthread_t) workers;
    pthread_cond_t job_cond;
    pthread_cond_t done_cond;
    pthread_mutex_t lock;
    size_t jobs_in_progress;
    size_t active_threads;
    bool done;
} AvenThreadPool;

static AvenThreadPoolJob aven_thread_pool_queue_pop(AvenThreadPoolQueue *queue) {
    assert(queue->len > 0);
    queue->len -= 1;
    size_t old_front = queue->front;
    queue->front = (queue->front + 1) % queue->job_indices.len;
    return slice_get(queue->job_indices, old_front);
}

static void aven_thread_pool_queue_push(
    AvenThreadPoolQueue *queue,
    AvenThreadPoolJob job
) {
    assert(queue->len < queue->job_indices.len);
    slice_get(queue->job_indices, queue->back) = job;
    queue->back = (queue->back + 1) % queue->job_indices.len;
    queue->len += 1;
}

static void *aven_thread_pool_worker_internal_fn(void *args) {
    AvenThreadPool *thread_pool = args;

    pthread_mutex_lock(&thread_pool->lock);
    thread_pool->active_threads += 1;
    pthread_mutex_unlock(&thread_pool->lock);

    for (;;) {
        pthread_mutex_lock(&thread_pool->lock);
        while (thread_pool->job_queue.len == 0 and !thread_pool->done) {
            pthread_cond_wait(&thread_pool->job_cond, &thread_pool->lock);
        }
        if (thread_pool->done) {
            break;
        }
        thread_pool->jobs_in_progress += 1;
        AvenThreadPoolJob job = aven_thread_pool_queue_pop(
            &thread_pool->job_queue
        );
        pthread_mutex_unlock(&thread_pool->lock);
        
        job.fn(job.args);

        pthread_mutex_lock(&thread_pool->lock);
        thread_pool->jobs_in_progress -= 1;
        if (
            !thread_pool->done and
            thread_pool->jobs_in_progress == 0 and
            thread_pool->job_queue.len == 0
        ) {
            pthread_cond_signal(&thread_pool->done_cond);
        }
        pthread_mutex_unlock(&thread_pool->lock);
    }

    thread_pool->active_threads -= 1;
    pthread_cond_signal(&thread_pool->done_cond);
    pthread_mutex_unlock(&thread_pool->lock);
    return NULL;
}

static AvenThreadPool aven_thread_pool_init(
    size_t nthreads,
    size_t njobs,
    AvenArena *arena
) {
    AvenThreadPool thread_pool = {
        .job_queue = {
            .job_indices = aven_arena_create_slice(
                AvenThreadPoolJob,
                arena,
                njobs
            ),
        },
        .workers = aven_arena_create_slice(
            pthread_t,
            arena,
            nthreads
        ),
    };

    int error = pthread_mutex_init(&thread_pool.lock, NULL);
    if (error != 0) {
        aven_panic("pthread_mutex_init failed");
    }

    error = pthread_cond_init(&thread_pool.job_cond, NULL);
    if (error != 0) {
        aven_panic("pthread_cond_init failed");
    }

    error = pthread_cond_init(&thread_pool.done_cond, NULL);
    if (error != 0) {
        aven_panic("pthread_cond_init failed");
    }

    return thread_pool;
}

static void aven_thread_pool_run(AvenThreadPool *thread_pool) {
    for (size_t i = 0; i < thread_pool->workers.len; i += 1) {
        int error = pthread_create(
            &slice_get(thread_pool->workers, i),
            NULL,
            aven_thread_pool_worker_internal_fn,
            thread_pool
        );
        if (error != 0) {
            aven_panic("pthread_create failed");
        }
    }
}

static void aven_thread_pool_submit(
    AvenThreadPool *thread_pool,
    AvenThreadPoolJobFn fn,
    void *args
) {
    pthread_mutex_lock(&thread_pool->lock);
    aven_thread_pool_queue_push(
        &thread_pool->job_queue,
        (AvenThreadPoolJob){ .fn = fn, .args = args }
    );
    pthread_cond_broadcast(&thread_pool->job_cond);
    pthread_mutex_unlock(&thread_pool->lock);
}

static void aven_thread_pool_wait(AvenThreadPool *thread_pool) {
    pthread_mutex_lock(&thread_pool->lock);
    while (
        thread_pool->job_queue.len > 0 or
        (!thread_pool->done and thread_pool->jobs_in_progress > 0) or
        (thread_pool->done and thread_pool->active_threads > 0)
    ) {
        pthread_cond_wait(&thread_pool->done_cond, &thread_pool->lock);
    }
    pthread_mutex_unlock(&thread_pool->lock);
}

static void aven_thread_pool_halt_and_destroy(AvenThreadPool *thread_pool) {
    pthread_mutex_lock(&thread_pool->lock);
    thread_pool->done = true;
    thread_pool->job_queue = (AvenThreadPoolQueue){ 0 };
    pthread_cond_broadcast(&thread_pool->job_cond);
    pthread_mutex_unlock(&thread_pool->lock);

    aven_thread_pool_wait(thread_pool);

    pthread_mutex_destroy(&thread_pool->lock);
    pthread_cond_destroy(&thread_pool->job_cond);
    pthread_cond_destroy(&thread_pool->done_cond);
}

#endif // AVEN_THREAD_POOL_H
