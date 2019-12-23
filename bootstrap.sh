#!/bin/sh

mkdir -p work && c++ -std=c++17 -O2 -pthread -Wall -Wextra -o work/sb *.cc
