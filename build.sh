#!/bin/sh


./configure --as-shared-lib --target-list="x86_64-linux-user" --disable-bsd-user --disable-docs --disable-tests --disable-tools --disable-fdt --disable-system --disable-docs

#make clean

make -j`nproc`

