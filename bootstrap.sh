#!/bin/sh

mkdir -p work && c++ -std=c++14 -pthread -Wall -Wextra -o work/eb *.cc
