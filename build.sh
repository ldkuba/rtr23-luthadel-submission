#!/bin/bash

mkdir build
cd ./build
mkdir shaders

# build shaders
glslc ../src/shaders/simple_vertex_shader.vert -o shaders/simple_vertex_shader.vert.spv
glslc ../src/shaders/simple_fragment_shader.frag -o shaders/simple_fragment_shader.frag.spv

cmake -S .. -B .
ninja -v
