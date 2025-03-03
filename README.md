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
  ./build -ccflags "-O3 -ffast-math" -glfw-ccflags "-O3 -DNDEBUG"
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

To build and run the algorithm benchmarks you will need ~3GB of
available system RAM. To benchmark the threaded algorithms
your C compiler must support C11 atomics. An example full benchmark
run command for `gcc` or `clang` on Linux would be:
```
./build bench -ccflags "-std=c11 -O3 -march=native -DBENCHMARK_THREADED"
```
The benchmarks may take up to a few hours to complete.
 
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
and see `crpoc_make.sh`, `emcc_make.sh`, `zig_make.sh`, and `zig_make.bat`
for examples.

[1]: https://musing.permutationlock.com/static/triangulate/visualization.html
[2]: https://github.com/permutationlock/implpathcol_paper
