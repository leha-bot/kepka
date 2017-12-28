#!/bin/bash

set -x

cmake -G Ninja -DCMAKE_BUILD_TYPE=$BUILD_TYPE -DBUILD_TESTING=ON .. || exit 1
cmake --build . || exit 1
ASAN_OPTIONS=alloc_dealloc_mismatch=0 ctest . || exit 1
