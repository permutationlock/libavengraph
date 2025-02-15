#if defined(_WIN32)
    #if defined(_MSC_VER) && defined(__clang__)
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wdeprecated-declarations"
    #endif
    #define _GLFW_WIN32
#elif defined(__linux__)
    #define _GLFW_WAYLAND
    #define _GLFW_X11
#elif defined(__FreeBSD__) || \
    defined(__OpenBSD__) || \
    defined(__NetBSD__) || \
    defined(__DragonFly__)

    #define _GLFW_X11
#endif

#define _GNU_SOURCE

#include "src/init.c"
#include "src/platform.c"
#include "src/context.c"
#include "src/monitor.c"
#include "src/window.c"
#include "src/input.c"
#include "src/vulkan.c"

#include "src/null_init.c"
#include "src/null_window.c"
#include "src/null_joystick.c"
#include "src/null_monitor.c"

#if defined(_WIN32)
    #include "src/win32_init.c"
    #include "src/win32_module.c"
    #include "src/win32_monitor.c"
    #include "src/win32_window.c"
    #include "src/win32_joystick.c"
    #include "src/win32_time.c"
    #include "src/win32_thread.c"
    #include "src/wgl_context.c"

    #include "src/egl_context.c"
    #include "src/osmesa_context.c"
    #if defined(_MSC_VER) && defined(__clang__)
        #pragma clang diagnostic pop
    #endif
#elif defined(__linux__) 
    #include "src/posix_poll.c"
    #include "src/posix_module.c"
    #include "src/posix_thread.c"
    #include "src/posix_time.c"
    #include "src/linux_joystick.c"
    #include "src/xkb_unicode.c"

    #include "src/egl_context.c"
    #include "src/osmesa_context.c"

    #include "src/wl_init.c"
    #include "src/wl_monitor.c"
    #include "src/wl_window.c"

    #include "src/x11_init.c"
    #include "src/x11_monitor.c"
    #include "src/x11_window.c"
    #include "src/glx_context.c"
#elif defined(__FreeBSD__) || \
    defined(__OpenBSD__) || \
    defined( __NetBSD__) || \
    defined(__DragonFly__)

    #include "src/posix_module.c"
    #include "src/posix_thread.c"
    #include "src/posix_time.c"
    #include "src/posix_poll.c"
    #include "src/null_joystick.c"
    #include "src/xkb_unicode.c"

    #include "src/x11_init.c"
    #include "src/x11_monitor.c"
    #include "src/x11_window.c"
    #include "src/glx_context.c"

    #include "src/egl_context.c"
    #include "src/osmesa_context.c"
#endif

