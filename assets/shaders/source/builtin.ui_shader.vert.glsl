#version 450

layout(set = 0, binding = 0)uniform global_uniform_buffer {
    mat4 projection;
    mat4 view;
}ubo;

layout(push_constant)uniform push_constants {
    
    // Only guaranteed a total of 128 bytes.
    mat4 model; // 64 bytes
}pc;

layout(location = 0)in vec3 in_position;
layout(location = 1)in vec2 in_texture_coordinate;

layout(location = 0)out vec2 frag_texture_coordinate;

void main() {
    gl_Position = ubo.projection * ubo.view * pc.model * vec4(in_position, 1.0);
    frag_texture_coordinate = in_texture_coordinate;
}