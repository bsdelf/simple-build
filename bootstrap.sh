#!/bin/sh

mkdir -p work && c++ -std=c++1z -pthread -Wall -Wextra -o work/eb *.cc
