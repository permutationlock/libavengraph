zig cc -DAVEN_BUILD_COMMON_DEFAULT_CC="\"zig\"" \
    -DAVEN_BUILD_COMMON_DEFAULT_AR="\"zig\"" \
    -DAVEN_BUILD_COMMON_DEFAULT_WINDRES="\"zig\"" \
    -DAVEN_BUILD_COMMON_DEFAULT_CCFLAGS="\"cc -std=c11\"" \
    -DAVEN_BUILD_COMMON_DEFAULT_LDFLAGS="\"cc\"" \
    -DAVEN_BUILD_COMMON_DEFAULT_ARFLAGS="\"ar -rcs\"" \
    -Wall -Wextra -o build build.c
