#version 450
#extension GL_EXT_scalar_block_layout : require

layout(std430, set = 0, binding = 0)uniform global_uniform_buffer {
    mat4 projection;
    mat4 view;
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

layout(location = 0)out vec4 out_ss_position;
layout(location = 1)out vec3 out_normal;

void main() {
    out_ss_position = UBO.projection * UBO.view * PC.model * vec4(in_position, 1.0);
    out_normal = normalize(mat3(PC.model) * in_normal);
    
    gl_Position = out_ss_position;
}