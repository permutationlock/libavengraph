# required environment:
#     WEB_CC: path to emscripten cc
#     WEB_AR: path to emscripten ar
#     APP_NAME: the name of your app
#     CFLAGS: common flags to be passed to C compiler
#     LDFLAGS: common flags to be passed to the linker

# clean old build artifiacts
./clean.sh

mkdir -p build_web/public
cd ..
./build --cc "$WEB_CC" \
    --ld "$WEB_CC" \
    --ar "$WEB_AR" \
    --ccflags "$CFLAGS" \
    --glfw-ccflags "$CFLAGS" \
    --stb-ccflags "$CFLAGS" \
    --ldflags "$LDFLAGS \
        -s ASYNCIFY \
        -s USE_GLFW=3 \
        -s MAX_WEBGL_VERSION=2 \
        -s MIN_WEBGL_VERSION=2 \
        -s EXPORTED_RUNTIME_METHODS=cwrap \
        -s EXPORTED_FUNCTIONS=_main,_aven_gl_window_impl_emscripten_resize \
        --shell-file web/shell.html" \
    --arflags "-rcs" \
    --exext ".html .js .wasm" \
    --obext ".o" \
    --arext ".a" \
    --winpthreads false \
    --winutf8 false \
    --glfw-external \
    --syslibs ""
cp build_out/visualization.html web/build_web/public/
cp build_out/visualization.js web/build_web/public/
cp build_out/visualization.wasm web/build_web/public/
cd web
