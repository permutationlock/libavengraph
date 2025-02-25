// config.h can be used to define custom defaults for flags

// A nice strict set of debug flags for clang and gcc
#if !defined(_WIN32) && defined(__GNUC__)
    #define AVEN_BUILD_COMMON_DEFAULT_CCFLAGS \
        "-std=c11 -pedantic -fstrict-aliasing -O0 -g3 -Werror -Wall -Wextra " \
        "-Wshadow -Wconversion -Wdouble-promotion -Winit-self " \
        "-Wcast-align -Wstrict-prototypes -Wold-style-definition " \
        "-fsanitize-trap -fsanitize=unreachable -fsanitize=undefined"
    #define LIBAVENGL_DEFAULT_GLFW_CCFLAGS \
        "-std=c11 -fstrict-aliasing -O0 -g3 -Werror -Wall -Wextra " \
        "-Wstrict-prototypes -Wold-style-definition -Winit-self " \
        "-Wno-unused-parameter -Wno-sign-compare -Wno-overflow " \
        "-Wno-missing-field-initializers " \
        "-fsanitize-trap -fsanitize=unreachable -fsanitize=undefined"
    #define LIBAVENGL_DEFAULT_STB_CCFLAGS \
        "-std=c11 -pedantic -fstrict-aliasing -O0 -g3 -Werror -Wall -Wextra " \
        "-Wstrict-prototypes -Wold-style-definition -Winit-self " \
        "-Wno-unused-parameter -Wno-unused-function -Wno-sign-compare " \
        "-Wno-missing-field-initializers " \
        "-fsanitize-trap -fsanitize=unreachable -fsanitize=undefined"
#endif




/* // Thread sanitizer flags
 * #define AVEN_BUILD_COMMON_DEFAULT_LDFLAGS \
 *     "-ltsan -lm -lpthread"
 *     "-fsanitize=thread -DAVEN_THREAD_POOL_USE_PTHREADS "
 */
