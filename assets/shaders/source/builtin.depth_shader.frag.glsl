#version 450
#extension GL_EXT_scalar_block_layout : require

layout(location = 0)in vec4 in_ss_position;
layout(location = 1)in vec3 in_normal;

layout(location = 0)out vec4 out_color;

// Main
void main() {
    float depth = in_ss_position.z / in_ss_position.w;
    out_color = vec4(in_normal, depth);
}