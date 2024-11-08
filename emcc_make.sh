#! /bin/bash
make
./build -cc "emcc" -ccflags "-O3 -DNEDBUG -DAVEN_UNREACHABLE_ASSERT" \
    -stb-ccflags "-O3 -DNDEBUG -DAVEN_UNREACHABLE_ASSERT" \
    -ldflags "-s ASYNCIFY -s USE_GLFW=3 -s EXPORTED_RUNTIME_METHODS=cwrap \
    -s EXPORTED_FUNCTIONS=_main,_on_resize --shell-file web/shell.html \
    -O3" -exext ".html .js .wasm" -no-glfw -syslibs ""
