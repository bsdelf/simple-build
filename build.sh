#!/bin/sh
#
# ./build
# ./build clean
#

eb c++1z flags="-O2" ldflags="-flto" verbose strict thread workdir=work out=eb "$*"
