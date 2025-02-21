# The Aven Graph Library

An "adjacency slice" graph library along with implementations
of a few [path coloring algorithms for plane graphs][2].

## Code organization

The header-only graph library is contained in the `include` directory.
The majority of the library depends only on a couple files from
the simple `libaven` header-only library, which is vendored in the
`deps` directory.

The `*/geometry.h` files define functions to construct vector geometry
for graph visualization and depend on the headers from
`libavengl`. The `libavengl` library draws 2D vector
graphics using the OpenGL ES 2.0 API.
The provided visualization uses GLFW for window creation.
The GLFW and `libavengl` libraries and the headers
for X11, xkbcommon, and Wayland are vendored in `deps`.

## Building the project

The visualization should build with no system dependencies other
than a C compiler on most modern Windows or Linux systems.

To build the build system:
```Shell
make
```
or
```Shell
cc -o build build.c
```
where `cc` is your favorite C compiler that supports C99 or later [^1].

To build the algorithm visualization run:
```
./build
```
The resulting executables will be in the `build_out` directory.

For a release build using `clang` or `gcc` run:
```
  ./build -ccflags "-O3" -glfw-ccflags "-O3 -DNDEBUG"
```

### Watch mode and hot reloading

To build and run the visualization in "watch mode" you may run:
```
  ./build watch
```
The build system will watch for changes to files in `visualization/`. If a
file in `visualization/game/` is modified, then the changes will be compiled
into a dynamic library and hot loaded into the running application. If
a file in the root of `visualization/` is changed, then the application
will close, compile, and run.

### Tikz drawings

A couple rudimentary programs were written to find and draw
algorithm examples as a series of Tikz drawings. To build these
exectuables, run:
```
./build tikz
```

### Benchmarks

To build and run the algorithm benchmarks you will need ~13GB of
unused available system RAM and a C compiler that supports C11
atomics. An example run command with `gcc` or `clang` on Linux would be:
```
./build bench -ccflags "-std=c11 -O3 -march=native"
```
The benchmarks may take up to a few hours to complete.

Here are the latest results from my Linux system with
`x86_64` Intel N100 processor, compiled using Clang 19.1.7:

|     Algorithm |  n=1.0e3  |  n=1.0e4  |  n=1.0e5  |  n=1.0e6  |   n=1.0e7  |
| ------------- | --------- | --------- | --------- | --------- | ---------- |
|          bfs  |   2.01e4  |   2.65e5  |   3.71e6  |   1.01e8  |    1.27e9  |
|      augment  |   4.46e4  |   6.72e5  |   1.79e7  |   4.17e8  |    4.77e9  |
| p3color\_bfs  |   2.45e5  |   3.49e6  |   5.52e7  |   1.27e9  |   1.98e10  |
|      p3color  |   5.27e4  |   6.08e5  |   7.25e6  |   1.32e8  |    1.51e9  |
|     p3choose  |   6.05e4  |   7.18e5  |   1.03e7  |   1.92e8  |    2.20e9  |

Threaded results for p3color :

| Threads |  n=1.0e3  |  n=1.0e4  |  n=1.0e5  |  n=1.0e6  |  n=1.0e7  |
| ------- | --------- | --------- | --------- | --------- | --------- |
|      1  |   5.27e4  |   6.08e5  |   7.25e6  |   1.32e8  |   1.51e9  |
|      2  |   5.25e4  |   4.37e5  |   4.26e6  |   7.51e7  |   8.61e8  |
|      3  |   4.87e4  |   3.28e5  |   3.30e6  |   5.65e7  |   6.49e8  |
|      4  |   4.90e4  |   2.76e5  |   2.79e6  |   4.72e7  |   5.44e8  |

Threaded results for p3choose :

| Threads |  n=1.0e3  |  n=1.0e4  |  n=1.0e5  |  n=1.0e6  |  n=1.0e7  |
| ------- | --------- | --------- | --------- | --------- | --------- |
|      1  |   6.05e4  |   7.18e5  |   1.03e7  |   1.92e8  |   2.20e9  |
|      2  |   7.55e4  |   6.76e5  |   7.98e6  |   1.30e7  |   1.47e8  |
|      3  |   7.44e4  |   6.08e5  |   6.70e6  |   1.01e7  |   1.13e8  |
|      4  |   8.22e4  |   6.10e5  |   6.36e6  |   8.77e7  |   9.73e8  |

Pyramid graph p3color scaling comparison:

|         Algorithm |  n=1.0e3  |  n=1.0e4  |  n=1.0e5  |  n=1.0e6  |   n=1.0e7  |
| ----------------- | --------- | --------- | --------- | --------- | ---------- |
|     p3color\_bfs  |   3.08e5  |   9.96e6  |   4.12e8  |  1.28e10  |   6.67e11  |
|          p3color  |   2.35e4  |   2.57e5  |   3.04e6  |   4.52e7  |    1.14e9  |
| p3color\_bfs  (f) |   3.11e5  |   9.79e6  |   4.46e8  |  3.52e10  |   1.18e12  |
|      p3color  (f) |   2.19e4  |   2.24e5  |   2.26e6  |   2.35e7  |    2.35e8  |
 
### Cleaning up

To clean all build artifacts:
```
./build clean
```

## Online visualization

The visualization compiled with Emscripten is [available online][1].

## License

The copyright Aven Bross 2024 and the license provided in `LICENSE.MIT`
apply to each text file that does
not contain its own separate license and copyright.

[^1]: Most common C compilers will be configured by default: `gcc`, `clang`,
and `tcc` on Linux, and `gcc.exe`, `clang.exe`, and `cl.exe` on Windows
should be supported out-of-the-box.
Otherwise you will need to define the various flags yourself.
Run `./build -h` for a full rundown of what needs to be configured,
and see `emcc_make.sh`, `zig_make.sh`, and `zig_make.bat` for examples.

[1]: https://musing.permutationlock.com/static/triangulate/visualization.html
[2]: https://github.com/permutationlock/implpathcol_paper
