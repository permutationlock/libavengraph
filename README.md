# The Aven Graph Library

An "adjacency slice" graph library along with implementations
of [path coloring algorithms for plane graphs](https://github.com/implpathcol_paper).

## Building

To build the build system:

```Shell
make
```
or
```Shell
cc -o build build.c
```
where `cc` is your favorite C compiler that supports C99 or later.

To build examples:
```
./build
```
The resulting example animation executables will be in the `build_out` directory.

To build and run benchmarks you will need a C compiler that supports C11
atomics and the GNU `asm` inline assembly extension:
```
./build bench -ccflags "-O3"
```

To clean build artifacts:
```
./build clean
```

## Online examples

The animated examples compiled with Emscripten are available online:

 - [Poh 3-color Animation](https://musing.permutationlock.com/static/triangulate/poh.html)
 - [Hartman 3-choose Animation](https://musing.permutationlock.com/static/triangulate/hartman.html)
