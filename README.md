# Getting Started

This repository contains a simple utility to resolve a given IP address and
port using libfabric.

## Building

```sh
./autogen.sh
./configure --with-libfabric=<path> --prefix=<path> --enable-debug
make
make install -j8
```

`--prefix` allows setting the install location, and `--enable-debug` will
enable debug output.

## Running

See the usage (`-h`):

```sh
bturrubi at ivy15 :: ./src/fi_resolve -h
A simple libfabric application to resolve an address

Usage:  ./src/fi_resolve [options] <address>

        -f, <fabric name>       fabric to bind to
        -p, <port>              port to resolve (default: 1337)
        -P, <provider name>     libfabric provider to use
        -h,                     print this help and exit
```

## Why?

I was trying to debug a possible issue with `fi_av_insert` and needed a quick
tool to play around with it.
