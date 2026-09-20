./zig_make.sh
rm -rf build_out build_work build_release
mkdir build_release
./build \
    --cflags "" \
    --ppflags "cc -E -std=c11 -target x86_64-linux-gnu -DBENCHMARK_THREADED" \
    --ccflags "cc -std=c11 -target x86_64-linux-gnu -g0 -O3 -ffast-math" \
    --glfw-ccflags "cc -std=c11 -target x86_64-linux-gnu -g0 -O3 -ffast-math" \
    --ldflags "cc -target x86_64-linux-gnu -g0 -O3"
mkdir build_release/x86_64-linux-gnu
mv build_out/visualization build_release/x86_64-linux-gnu/
mv build_bench/all build_release/x86_64-linux-gnu/
mv build_bench/pyramid build_release/x86_64-linux-gnu/
zip build_release/x86_64-linux-gnu_2_21.zip build_release/x86_64-linux-gnu_2_21/*
tar -czvf build_release/x86_64-linux-gnu_2_21.tar.gz build_release/x86_64-linux-gnu_2_21
./build clean
./build \
    --cflags "" \
    --ppflags "cc -E -std=c11 -target x86_64-linux-musl -DBENCHMARK_THREADED" \
    --ccflags "cc -std=c11 -target x86_64-linux-musl -g0 -O3 -ffast-math -static" \
    --glfw-ccflags "cc -std=c11 -target x86_64-linux-musl -g0 -O3 -ffast-math -static" \
    --ldflags "cc -target x86_64-linux-musl -g0 -O3 -static" \
    --syslibs "c"
mkdir build_release/x86_64-linux-musl
mv build_out/visualization build_release/x86_64-linux-musl/
mv build_bench/all build_release/x86_64-linux-musl/
mv build_bench/pyramid build_release/x86_64-linux-musl/
zip build_release/x86_64-linux-musl.zip build_release/x86_64-linux-musl/*
tar -czvf build_release/x86_64-linux-musl.tar.gz build_release/x86_64-linux-musl
./build clean
./build \
    --cflags "" \
    --ppflags "cc -E -target x86_64-windows-gnu -std=c11 -DBENCHMARK_THREADED" \
    --ccflags "cc -target x86_64-windows-gnu -std=c11 -g0 -O3 -ffast-math" \
    --glfw-ccflags "cc -target x86_64-windows-gnu -std=c11 -g0 -O3 -ffast-math" \
    --ldflags "cc -target x86_64-windows-gnu -g0 -O3" \
    --exext ".exe" --obext ".o" --soext ".dll" --arext ".a" --wrext ".o" \
    --ldwinflag "-Wl,--subsystem,windows" --winutf8 --winpthreads \
    --syslibs "kernel32 user32 gdi32 shell32"
rm build_out/visualization.pdb
mkdir build_release/x86_64-windows-gnu
mv build_out/visualization.exe build_release/x86_64-windows-gnu/
mv build_bench/all.exe build_release/x86_64-windows-gnu/
mv build_bench/pyramid.exe build_release/x86_64-windows-gnu/
zip build_release/x86_64-windows-gnu.zip build_release/x86_64-windows-gnu/*
tar -czvf build_release/x86_64-windows-gnu.tar.gz build_release/x86_64-windows-gnu
./build clean \
    --exext ".exe" --obext ".o" --soext ".dll" --arext ".a" --wrext ".o" \
    --windres "zig" --winutf8 --winpthreads
