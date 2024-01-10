#version 450
#extension GL_EXT_scalar_block_layout : require

layout(location = 0)in vec2 in_position;
layout(location = 1)in vec2 in_texture_coordinate;
layout(location = 0)out vec2 out_texture_coordinate;

void main() {
    // [0, 2]x[0, 2] -> [-1, 1]x[-1, 1]
    gl_Position = vec4(in_position.x - 1.0, in_position.y - 1.0, 0.0, 1.0);
    out_texture_coordinate = in_texture_coordinate;
}