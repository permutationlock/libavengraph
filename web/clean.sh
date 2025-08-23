rm -rf build_web

cd ..
./build clean --exext ".html .js .wasm" \
    --obext ".o" \
    --arext ".a" \
    --glfw-external \
    --syslibs ""
cd web
