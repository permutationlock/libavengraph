// config.h can be used to defines custom defaults for flags

#if !defined(_WIN32) && defined(__GNUC__)
    #define AVEN_BUILD_COMMON_DEFAULT_CCFLAGS \
        "-DAVEN_UNREACHABLE_ASSERT " \
        "-std=c11 -pedantic -fstrict-aliasing -O- -g3 -Werror -Wall -Wextra " \
        "-Wshadow -Wconversion -Wdouble-promotion -Winit-self " \
        "-Wcast-align -Wstrict-prototypes -Wold-style-definition " \
        "-fsanitize-trap -fsanitize=unreachable -fsanitize=undefined"
    #define LIBAVENGL_DEFAULT_GLFW_CCFLAGS \
        "-std=c11 -pedantic -fstrict-aliasing -O0 -g3 -Werror -Wall -Wextra " \
        "-Wstrict-prototypes -Wold-style-definition -Winit-self " \
        "-Wno-unused-parameter -Wno-sign-compare -Wno-overflow " \
        "-Wno-missing-field-initializers " \
        "-fsanitize-trap -fsanitize=unreachable -fsanitize=undefined"
    #define LIBAVENGL_DEFAULT_STB_CCFLAGS \
        "-DAVEN_UNREACHABLE_ASSERT " \
        "-std=c11 -pedantic -fstrict-aliasing -O0 -g3 -Werror -Wall -Wextra " \
        "-Wstrict-prototypes -Wold-style-definition -Winit-self " \
        "-Wno-unused-parameter -Wno-unused-function -Wno-sign-compare " \
        "-Wno-missing-field-initializers " \
        "-fsanitize-trap -fsanitize=unreachable -fsanitize=undefined"
#endif
