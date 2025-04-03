./zig_make.sh
mkdir build_release
./build \
    -ccflags "cc -target x86_64-linux-gnu.2.21 -std=c11 -g0 -O3 -ffast-math -DNDEBUG" \
    -ldflags "cc -g0 -O3 -target x86_64-linux-gnu.2.21"
mv build_out build_release/x86_64-linux-gnu_2_21
zip build_release/x86_64-linux-gnu_2_21.zip build_release/x86_64-linux-gnu_2_21/*
tar -czvf build_release/x86_64-linux-gnu_2_21.tar.gz build_release/x86_64-linux-gnu_2_21
./build \
    -ccflags "cc -target x86_64-linux-musl -std=c11 -g0 -O3 -ffast-math -DNDEBUG" \
    -ldflags "cc -g0 -O3 --sysroot $MUSL_SYSROOT -L/lib -Wl,--dynamic-linker,/lib/ld-musl-x86_64.so.1" \
    -syslibs "c"
mv build_out build_release/x86_64-linux-musl
zip build_release/x86_64-linux-musl.zip build_release/x86_64-linux-musl/*
tar -czvf build_release/x86_64-linux-musl.tar.gz build_release/x86_64-linux-musl
./build clean
./build \
    -ccflags "cc -target x86_64-windows-gnu -std=c11 -g0 -O3 -ffast-math -DNDEBUG" \
    -ldflags "cc -g0 -O3 -target x86_64-windows-gnu" \
    -windres "zig" -windresflags "rc" -windresoutflag "/fo" \
    -exext ".exe" -obext ".o" -soext ".dll" -arext ".a" -wrext ".o" \
    -ldwinflag "-Wl,--subsystem,windows" -winutf8 -winpthreads \
    -syslibs "kernel32 user32 gdi32 shell32"
rm build_out/visualization.pdb
mv build_out  build_release/x86_64-windows-gnu
zip build_release/x86_64-windows-gnu.zip build_release/x86_64-windows-gnu/*
tar -czvf build_release/x86_64-windows-gnu.tar.gz build_release/x86_64-windows-gnu
./build \
    -ccflags "cc -target x86_64-windows-gnu -std=c11 -g0 -O3 -ffast-math -DNDEBUG" \
    -ldflags "cc -g0 -O3 -target x86_64-windows-gnu" \
    -windres "zig" -windresflags "rc" -windresoutflag "/fo" \
    -exext ".exe" -obext ".o" -soext ".dll" -arext ".a" -wrext ".o" \
    -ldwinflag "-Wl,--subsystem,windows" -winutf8 -winpthreads clean
./emcc_make.sh
mv build_out build_release/wasm32-unknown-emscripten
zip build_release/wasm32-unknown-emscripten.zip build_release/wasm32-unknown-emscripten/*
tar -czvf build_release/wasm32-unknown-emscripten.tar.gz build_release/wasm32-unknown-emscripten
./emcc_clean.sh


