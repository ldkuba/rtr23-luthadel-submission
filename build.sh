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

# build shaders
glslc ../../src/shaders/simple_vertex_shader.vert -o shaders/simple_vertex_shader.vert.spv
glslc ../../src/shaders/simple_fragment_shader.frag -o shaders/simple_fragment_shader.frag.spv

cmake -S ../.. -B . -GNinja \
    -DCMAKE_BUILD_TYPE=$MODE
ninja && ./VulkanEngine