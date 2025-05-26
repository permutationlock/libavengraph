make
./build --cc "emcc" --ccflags "-O3 -ffast-math -g0" \
    --ldflags "-s ASYNCIFY -s USE_GLFW=3 -s EXPORTED_RUNTIME_METHODS=cwrap \
        -s EXPORTED_FUNCTIONS=_main,_on_resize --shell-file web/shell.html \
        -O3 -g0" \
    --exext ".html .js .wasm" --no-glfw --syslibs ""
