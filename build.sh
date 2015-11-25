#!/bin/sh
#
# ./build
# ./build clean
#

eb c++1y flag="-O2" strict thread workdir=work out=eb $*
