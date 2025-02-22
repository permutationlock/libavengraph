./build clean -cc "emcc" -ccflags "" \
    -ldflags "-s ASYNCIFY -s USE_GLFW=3" \
    -exext ".html .js .wasm" -no-glfw -syslibs ""
