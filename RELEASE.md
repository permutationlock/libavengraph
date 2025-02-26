# Generating release binaries

The script `release_make.sh` is used to generate all release binaries.

## Prerequisites
To run the script you will need to be on a POSIX system with
`make`, `zip`, `tar`, [`zig`][2], and [`emcc`][3] in the `PATH`.

You will also need a [musl libc][1] sysroot. Such a sysroot may easily
be created by downloading the musl source code and building and
installing it at a local prefix. The script `musl-sysroot_make.sh` will
download a musl release using `curl` and create a `musl-sysroot` sub directory
in the current working directory.

## Running the script

If you have a `musl-sysroot` folder created by `musl-sysroot_make.sh`, then you
may run
```Shell
MUSL_SYSROOT=musl-sysroot ./release_make.sh
```
The release binaries and zip files will be in the `build_release`
directory.

## Cleaning

```Shell
./release_clean.sh
```
