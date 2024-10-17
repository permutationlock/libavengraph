#! /bin/bash
make
./build -cc "emcc" -ccflags "-O3 -DNEDBUG" \
    -ldflags "-s ASYNCIFY -s USE_GLFW=3" \
    -exext ".html .js .wasm" -no-glfw -syslibs ""
