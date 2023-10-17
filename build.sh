#!/bin/sh


./configure --target-list="x86_64-linux-user" --disable-system --enable-pie --enable-user \
    --enable-linux-user --disable-gtk --disable-sdl --disable-vnc --disable-strip


#make clean

make -j`nproc`

