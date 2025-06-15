#!/bin/sh

cd libs
cd Crow
mkdir build
cd build
cmake .. -DCROW_BUILD_EXAMPLES=OFF -DCROW_BUILD_TESTS=OFF
make install
cd ../../jwt-cpp
cmake .
cmake --build .
cmake --install .
cd ../libbcrypt
mkdir build
cd build
cmake ..
make
make install
ldconfig
