#!/bin/bash

set -e

git clone https://github.com/json-c/json-c.git
cd json-c

mkdir build && cd build
cmake -DBUILD_SHARED_LIBS=False -DDISABLE_WERROR=True -DCMAKE_INSTALL_PREFIX=/usr/local/i686-w64-mingw32 -DCMAKE_C_COMPILER=i686-w64-mingw32-gcc -DCMAKE_SYSTEM_NAME="Windows" ..
make V=1
sudo make install

cd .. && rm -rf build

mkdir build && cd build
cmake -DBUILD_SHARED_LIBS=False -DDISABLE_WERROR=True -DCMAKE_INSTALL_PREFIX=/usr/local/x86_64-w64-mingw32 -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc -DCMAKE_SYSTEM_NAME="Windows" ..
make V=1
sudo make install

cd .. && rm -rf build

cd ..
