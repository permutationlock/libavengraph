#ifndef AVEN_H
#define AVEN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define or ||
#define and &&

#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))

#ifndef __has_attribute
    #define __has_attribute(unused) 0
#endif

#ifndef __has_builtin
    #define __has_builtin(unused) 0
#endif

#if defined(AVEN_UNREACHABLE_ASSERT) and __has_builtin(__builtin_unreachable)
    #define assert(c) while (!(c)) { __builtin_unreachable(); }
#else
    #include <assert.h>
#endif

#if __STDC_VERSION__ >= 201112L
    #include <stdnoreturn.h>
#elif __STDC_VERSION__ >= 199901L
    #ifndef noreturn
        #define noreturn
    #endif
#else
    #error "C99 or later is required"
#endif

#define countof(array) (sizeof(array) / sizeof(*array))

#define Optional(t) struct { t value; bool valid; }
#define Result(t) struct { t payload; int error; }
#define Slice(t) struct { t *ptr; size_t len; }
#define List(t) struct { t *ptr; size_t len; size_t cap; }

typedef Slice(unsigned char) ByteSlice;

#if defined(AVEN_UNREACHABLE_ASSERT) or !defined(NDEBUG)
    static inline size_t aven_assert_lt_internal_fn(size_t a, size_t b) {
        assert(a < b);
        return a;
    }

    #define aven_assert_lt_internal(a, b) aven_assert_lt_internal_fn(a, b)
#else
    #define aven_assert_lt_internal(a, b) a
#endif

#define slice_get(s, i) (s).ptr[aven_assert_lt_internal(i, (s).len)]
#define list_get(l, i) (l).ptr[aven_assert_lt_internal(i, (l).len)]
#define list_push(l) (l).ptr[aven_assert_lt_internal((l).len++, (l).cap)]

#define slice_array(a) { .ptr = a, .len = countof(a) }
#define slice_list(l) { .ptr = (l).ptr, .len = (l).len }
#define slice_head(s, i) { \
        .ptr = (s).ptr, \
        .len = aven_assert_lt_internal(i, (s).len) \
    }
#define slice_tail(s, i) { \
        .ptr = (s).ptr + i, \
        .len = (s).len - aven_assert_lt_internal(i, (s).len) \
    }
#define slice_range(s, i, j) { \
        .ptr = (s).ptr + i, \
        .len = aven_assert_lt_internal(j, (s).len) - \
            aven_assert_lt_internal(i, j) \
    }
#define list_array(a) { .ptr = a, .cap = countof(a) }

#define as_bytes(ref) (ByteSlice){ \
        .ptr = (unsigned char *)ref, \
        .len = sizeof(*ref) \
    }
#define array_as_bytes(arr) (ByteSlice){ \
        .ptr = (unsigned char *)arr, \
        .len = sizeof(arr)\
    }
#define slice_as_bytes(s) (ByteSlice){ \
        .ptr = (unsigned char *)(s).ptr, \
        .len = (s).len * sizeof(*(s).ptr), \
    }
#define list_as_bytes(l) (ByteSlice) { \
        .ptr = (unsigned char *)(l).ptr, \
        .len = (l).len * sizeof(*(l).ptr), \
    }

#if defined(_WIN32) and defined(_MSC_VER)
    void *memcpy(void *s1, const void *s2, size_t n);
#else
    void *memcpy(void *restrict s1, const void *restrict s2, size_t n);
#endif

#define slice_copy(d, s) memcpy( \
        (d).ptr, \
        (s).ptr, \
        aven_assert_lt_internal( \
            (s).len * sizeof(*(s).ptr), \
            (d).len * sizeof(*(d).ptr) + 1 \
        ) \
    )

static inline noreturn void aven_panic(const char *msg) {
    // TODO: use stderr instead of stdout
    // using stdout because fprintf(stderr, ...) requires work or stdio.h
    int printf(const char *fmt, ...);
    noreturn void _Exit(int status);

    printf("PANIC: %s", msg);
    _Exit(1);
}

#if defined(AVEN_IMPLEMENTATION) and !defined(AVEN_IMPLEMENTATION_SEPARATE_TU)
    #define AVEN_FN static inline
#else
    #define AVEN_FN
#endif

#ifdef _WIN32
    #define AVEN_WIN32_FN(t) __declspec(dllimport) t __stdcall
#endif

#endif // AVEN_H
