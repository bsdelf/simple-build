# Simple Build

## Description

`sb` is a build tool which is suitable for small C/C++ projects.

For given source files, `sb` can detect all dependencies and figure out which source files should be (re)compiled and linked together. Therefore, developers can be fully focused on project implementation details without the need to amend Makefiles every now and then.

## Features

- Build automation
- Utilize all CPU cores

## Requirements

- Compiler: Clang or GCC
- System: FreeBSD / Linux / macOS

## Install

To build `sb`, you need a C++ compiler with c++17 support. Instructions:

```
git clone https://github.com/bsdelf/sb.git
cd sb && ./bootstrap.sh
cp sb /usr/local/bin
```

## Usage

Suppose we have a project with following structure:

```
.
├── clib.c
├── clib.h
├── main.cpp
├── utils.cpp
└── utils.h
```

### Scenario 1

To build this project, just type `sb`. The output will be:

```
Build 3 file(s)
[  25% ] ./utils.cpp => ./utils.cpp.o
[  50% ] ./main.cpp => ./main.cpp.o
[  75% ] ./clib.c => ./clib.c.o
[ 100% ] ./a.out
```

After then, an executable binary `a.out` will be available. To customize name, for example "demo", use following command:

```
sb target=demo
```

### Scenario 2

To use c++17 features for C++ source files and c89 features for C source files, use following command:

```
sb c++17 c89
```

### Scenario 3

To build "clib" into a shared library and link the rests against with "libclib",
use following two commands:

```
sb clib.c shared target=libclib.dylib
```

```
sb main.cpp utils.cpp ldflags="-L./ -lclib"
```

Note: ".dylib" is a shared library extension on macOS, for Linux or FreeBSD, ".so" should be used.

## Help

```
Usage:

    sb [options...] [files...] [directories...]

Options:

    clean       clean files
    jobs        set number of jobs
    target      set target name
    workdir     set working directory
    verbose     set verbose level

    as          set assembler
    asflags     add assembler flags
    cc          set c compiler
    cflags      add c compiler flags
    cxx         set c++ compiler
    cxxflags    add c++ compiler flags
    ld          set linker
    ldflags     add linker flags
    prefix      add search directories

    wol         without link
    thread      use pthreads
    optimize    set optimize level
    debug       enable -g
    release     enable -DNDEBUG
    strict      enable -Wall -Wextra -Werror
    shared      enable -fPIC -shared
    lto         enable -flto
    c89         enable -std=c89
    c99         enable -std=c99
    c11         enable -std=c11
    c18         enable -std=c18
    c++11       enable -std=c++11
    c++14       enable -std=c++14
    c++17       enable -std=c++17
    c++20       enable -std=c++20

    help        show help

```
