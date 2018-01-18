#!/bin/sh

mkdir -p work && c++ -std=c++1y -pthread -Wall -Wextra -o work/eb *.cc
