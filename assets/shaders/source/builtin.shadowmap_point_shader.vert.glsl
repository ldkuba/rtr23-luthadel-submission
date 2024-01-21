#version 450
#extension GL_EXT_scalar_block_layout : require

layout(std430, set = 0, binding = 0)uniform global_uniform_buffer {
    mat4 light_space;
}UBO;

layout(push_constant)uniform push_constants {
    // Only guaranteed a total of 128 bytes.
    mat4 model; // 64 bytes
}PC;

layout(location = 0)in vec3 in_position;
layout(location = 1)in vec3 in_normal;
layout(location = 2)in vec3 in_tangent;
layout(location = 3)in vec4 in_color;
layout(location = 4)in vec2 in_texture_coordinate;

void main() {
    // In -1 to 1 space.
    gl_Position = (UBO.light_space * PC.model) * vec4(in_position, 1.0);
}