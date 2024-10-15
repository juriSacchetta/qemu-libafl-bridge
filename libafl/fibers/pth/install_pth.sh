#!/bin/bash
mkdir -p ./build
cd build

../configure --enable-optimize --enable-syscall-soft --with-mctx-mth=mcsc --prefix=$(pwd)/../install
make
make install