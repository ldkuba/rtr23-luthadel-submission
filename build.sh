#!/bin/bash

MODE=$1

if [ $MODE != 'release' ] && [ $MODE != 'debug' ]; then
    if [ $MODE = 'r' ]; then
        MODE='release'
    elif [ $MODE = 'd' ]; then
        MODE='debug'
    else
        echo 'there was an attempt'
        exit 1
    fi
fi

mkdir build
cd ./build
mkdir $MODE
cd ./$MODE
mkdir shaders

cmake -S ../.. -B . -GNinja \
    -DCMAKE_BUILD_TYPE=$MODE
ninja && ./VulkanEngine