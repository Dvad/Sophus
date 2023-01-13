#!/bin/bash

set -x # echo on
set -e # exit on error

cd super_project
mkdir -p build
cd build
CC=clang CXX=clang++ cmake -DROW_ACCESS=$ROW_ACCESS -DSUPER_PROJ_FARM_NG_PROTOS=$BUILD_PROTOS -DCOVERAGE=On --debug-find ..
make -j2

gcovr
