#!/bin/sh
#
# ./build
# ./build clean
#

eb c++1y flags="-O2" ldflags="-flto" verbose strict thread workdir=work out=eb "$*"
