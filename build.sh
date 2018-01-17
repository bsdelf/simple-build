#!/bin/sh
#
# ./build
# ./build clean
#

eb c++14 flags="-O2" strict thread workdir=work out=eb $*
