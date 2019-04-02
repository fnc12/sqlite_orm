#!/bin/sh
# Copyright (c) 2012-2019 Sébastien Rombauts (sebastien.rombauts@gmail.com)
#
# Distributed under the MIT License (MIT) (See accompanying file LICENSE.txt
# or copy at http://opensource.org/licenses/MIT)

# exit on firts error
set -e

echo "g++ version: "
g++ --version

mkdir -p compile
cd compile

TRAVIS_JOB_NAME="clang default"

if [ "$TRAVIS_JOB_NAME" = "clang default" ]; then cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=../third_party/vcpkg/scripts/buildsystems/vcpkg.cmake -DSQLITE_ORM_OMITS_CODECVT=ON .. ; else\
    cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=../third_party/vcpkg/scripts/buildsystems/vcpkg.cmake .. ; fi



#cd third_party
#git clone https://github.com/Microsoft/vcpkg.git vcpkg
#cd vcpkg
#chmod +x bootstrap-vcpkg.sh
#./bootstrap-vcpkg.sh
#chmod +x vcpkg
#./vcpkg install gtest
#cd ../..

#mkdir -p build
#cd build
#
## Generate a Makefile for GCC (or Clang, depanding on CC/CXX envvar)
#cmake -DCMAKE_BUILD_TYPE=Debug -DSQLITECPP_USE_GCOV=OFF -DSQLITECPP_BUILD_EXAMPLES=ON -DSQLITECPP_BUILD_TESTS=ON ..
#
## Build (ie 'make')
#cmake --build .
#
## Build and run unit-tests (ie 'make test')
#ctest --output-on-failure