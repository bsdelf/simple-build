#!/bin/sh
#
# ./build
# ./build clean
#

eb c++1y flag="-Wall -O2" thread workdir=work out=eb $*
