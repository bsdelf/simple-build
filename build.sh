#!/bin/sh
#
# ./build
# ./build clean
#

eb c++1y flags="-O2" verbose strict thread workdir=work out=eb $*
