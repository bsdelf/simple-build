#!/bin/sh
clang++ -o eb eb.cc -O2 -stdlib=libc++ -std=c++11 -I./scx -lpthread
