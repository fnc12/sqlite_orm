#!/bin/sh

# exit on firts error
set -e

echo "Prepare third party libraries"
cd third_party
git clone https://github.com/Microsoft/vcpkg.git vcpkg
cd vcpkg
chmod +x bootstrap-vcpkg.sh

if [[ "$CXX" == *"clang"* && "$TRAVIS_OS_NAME" == "osx" ]]; then ./bootstrap-vcpkg.sh --allowAppleClang ; else ./bootstrap-vcpkg.sh ; fi

chmod +x vcpkg
./vcpkg install gtest
cd ../..

echo "Prepare for compile"
mkdir -p compile
cd compile

cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=../third_party/vcpkg/scripts/buildsystems/vcpkg.cmake ..

echo "Compiling..."
cmake --build . --config Debug

echo "Run testing..."
ctest -C Debug --output-on-failure -V

