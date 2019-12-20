#!/bin/sh
#
# ./build
# ./build clean
#

eb lto c++17 release optimize verbose strict thread workdir=work target=eb "$*"
