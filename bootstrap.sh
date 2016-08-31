#!/bin/sh

mkdir -p work && c++ -std=c++1y -pthread -Wall -o work/eb *.cc
