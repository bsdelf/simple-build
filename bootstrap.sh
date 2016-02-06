#!/bin/sh

mkdir -p work && c++ -std=c++1y -lpthread -Wall -o work/eb *.cc 
