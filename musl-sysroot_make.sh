curl -o musl.tar.gz https://musl.libc.org/releases/musl-1.2.5.tar.gz
tar -xvf musl.tar.gz
rm musl.tar.gz
mkdir musl-sysroot
cd musl-1.2.5
CC="zig cc --target=x86_64-linux-musl" CFLAGS="-O2 -g0" \
    ./configure --prefix=../musl-sysroot/ --target=x86_64-linux-musl
make
make install
cd ..
rm -rf musl-1.2.5

