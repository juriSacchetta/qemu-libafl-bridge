#!/bin/sh

# Function to run configure if the argument is not "skip"
configure() {
    if [ "$1" != "skip" ]; then
        ../configure --prefix=$PREFIX \
            --enable-debug 
    fi
}

# Create a separate build directory
mkdir -p .build
cd .build

# Set the installation prefix
PREFIX=$(pwd)

# Check if the optional argument is "skip"
if [ "$1" = "skip" ]; then
    echo "Skipping configure step."
else
    rm -rf *
    configure
fi

make

make install
