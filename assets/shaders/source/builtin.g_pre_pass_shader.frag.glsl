#version 450
#extension GL_EXT_scalar_block_layout : require

// Instance uniforms
layout(std430, set = 1, binding = 0)uniform instance_uniform_buffer {
    float smoothness;
}UBO;

layout(location = 0)in vec4 in_ss_position;
layout(location = 1)in vec3 in_normal;

layout(location = 0)out vec4 out_color;

// Main
void main() {
    out_color = vec4(in_normal, UBO.smoothness);
}