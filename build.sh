#!/bin/sh

# Ask the user if they want to enable fibers
read -p "Do you want to enable fibers? (y/n): " enable_fibers

# Set the fibers option based on user input
if [ "$enable_fibers" = "y" ]; then
    fibers_option="--fibers"
else
    fibers_option=""
fi

read -p "Do you want to build as a lib? (y/n): " as_lib
if [ "$as_lib" = "y" ]; then
    as_lib="--as-shared-lib"
else
    as_lib=""
fi
./configure $fibers_option $as_lib  --target-list="x86_64-linux-user" --disable-bsd-user --disable-tests --disable-tools --disable-fdt --disable-system --disable-docs

make -j`nproc`
