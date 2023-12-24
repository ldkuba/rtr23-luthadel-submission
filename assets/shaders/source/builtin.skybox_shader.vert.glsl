#version 450

layout(set = 0, binding = 0)uniform global_uniform_buffer {
    mat4 projection;
    mat4 view;
}UBO;

layout(location = 0)in vec3 in_position;
layout(location = 1)in vec3 in_normal;
layout(location = 2)in vec3 in_tangent;
layout(location = 3)in vec4 in_color;
layout(location = 4)in vec2 in_texture_coordinate;

layout(location = 0)out vec3 out_texture_coordinate;

void main() {
    out_texture_coordinate = in_position;
    gl_Position = UBO.projection * UBO.view * vec4(in_position, 1.0);
}