# The Aven Graph Library

An "adjacency slice" graph library along with implementations
of a few [path coloring algorithms for plane graphs][2].

## Code organization

The header-only graph library is contained in the `include` directory.
The majority of the library depends only on a few files from
the simple `libaven` header-only library, which is vendored in the
`dep` directory.

The few `*/geometry.h` files may be used construct vector geometry
for graph visualization and depend on the headers from
`libavengl`. The `libavengl` library draws 2D vector
graphics using the OpenGL ES 2.0 API.
The provided visualization also uses GLFW for window creation.
Both GLFW and `libavengl` are vendored in `dep`.

## Building examples

All examples should build with no system dependencies other
than a C compiler on any Windows or Linux system.

To build the build system:

```Shell
make
```
or
```Shell
cc -o build build.c
```
where `cc` is your favorite C compiler that supports C99 or later.

To build algorithm visualization and tikz figure generators:
```
./build
```
The resulting executables will be in the `build_out` directory.

For a release build using `clang` or `gcc` run:
```
  ./build -ccflags "-O3" -glfw-ccflags "-O3 -DNDEBUG"
```

To build and run the benchmarks you will need ~11.5GB of
unused available system RAM and a C compiler that supports C11
atomics:
```
./build bench -ccflags "-std=c11 -O3 -march=native"
```
The benchmark will take several minutes to an hour to complete
and will report incremental results.

To clean build artifacts:
```
./build clean
```

## Online visualization

The visualization compiled with Emscripten is [available online][1].

## License

The copyright Aven Bross 2024 and the license provided in `LICENSE.MIT`
apply to each text file that does
not contain its own separate license and copyright.

[1]: https://musing.permutationlock.com/static/triangulate/visualization.html
[2]: https://github.com/permutationlock/implpathcol_paper
