# build
<pre><code>
git clone https://github.com/bsdelf/eb.git
cd eb
./bootstrap.sh
</code></pre>
# usage
`./eb help`

### for example, build eb itself
<pre><code>
% tree
├── ConsUnit.cc
├── ConsUnit.h
├── Parallel.cc
├── Parallel.h
├── Tools.cc
├── Tools.h
├── main.cc
└── scx
    ├── Dir.hpp
    └── FileInfo.hpp

% eb c++1y flag="-Wall -O2" thread workdir=work out=eb
* Build: 4 files
[  25% ] Parallel.cc => work/Parallel.o
[  50% ] ConsUnit.cc => work/ConsUnit.o
[  75% ] main.cc => work/main.o
[ 100% ] Tools.cc => work/Tools.o
- Link - work/eb

% ldd work/eb

work/eb:
	libc++.so.1 => /usr/lib/libc++.so.1 (0x800841000)
	libcxxrt.so.1 => /lib/libcxxrt.so.1 (0x800b02000)
	libm.so.5 => /lib/libm.so.5 (0x800d1e000)
	libgcc_s.so.1 => /usr/local/lib/gcc48/libgcc_s.so.1 (0x800f47000)
	libthr.so.3 => /lib/libthr.so.3 (0x80115d000)
	libc.so.7 => /lib/libc.so.7 (0x801382000)
</code></pre>

### hmm, a little complicated example...
<pre><code>
% tree
kern
├── asmfun.asm
├── binfo.h
├── color.c
├── color.h
├── draw.c
├── draw.h
├── font.c
├── font.h
├── gdt.asm
├── gdt.c
├── gdt.h
├── idt.asm
├── idt.c
├── idt.h
├── kernel.c
├── paging.c
├── paging.h
├── timer.c
└── timer.h
libk
├── bcd.c
├── intdef.h
├── limits.h
├── limits32.h
├── printf.c
├── qdivrem.c
├── quad.h
├── stand.h
└── strlen.c

% ./build
* Build: 15 files
[   6% ] libk/bcd.c => bcd.o
[  13% ] libk/printf.c => printf.o
[  20% ] libk/qdivrem.c => qdivrem.o
[  26% ] libk/strlen.c => strlen.o
[  33% ] kern/asmfun.asm => asmfun.asm.o
[  40% ] kern/color.c => color.o
[  46% ] kern/draw.c => draw.o
[  53% ] kern/font.c => font.o
[  60% ] kern/gdt.asm => gdt.asm.o
[  66% ] kern/gdt.c => gdt.o
[  73% ] kern/idt.asm => idt.asm.o
[  80% ] kern/idt.c => idt.o
[  86% ] kern/kernel.c => kernel.o
[  93% ] kern/paging.c => paging.o
[ 100% ] kern/timer.c => timer.o
- Link - kernel.bin

% touch kern/draw.h
% ./build.sh
* Build: 4 files
[  25% ] kern/draw.c => draw.o
[  50% ] kern/idt.c => idt.o
[  75% ] kern/kernel.c => kernel.o
[ 100% ] kern/timer.c => timer.o
- Link - kernel.bin

% cat build.sh
	eb cc=clang flag="-std=c99 -nostdlib -nostdinc -fno-builtin -fno-stack-protector -m32" \
	    as="yasm" asflag="-felf" \
		ld="ld" ldflag="-melf_i386_fbsd -Ttext 0xc0000000 -e start" \
		out=kernel.elf libk/* kernel/*
</code></pre>

### Note
Since OS X Yosemite doesn't support POSIX 2008 specification yet, there is no `st_mtim` field in `struct stat`, you'll have to use `st_mtimespec` instead.
