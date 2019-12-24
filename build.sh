#!/bin/sh
#
# ./build
# ./build clean
#

sb lto c++17 release optimize verbose strict thread workdir=work target=sb $@
