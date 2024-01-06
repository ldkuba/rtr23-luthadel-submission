#version 450
#extension GL_EXT_scalar_block_layout : require

layout(std430, set = 0, binding = 0)uniform global_uniform_buffer {
    mat4 projection;
    mat4 view;
    mat4 light_space_directional;
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

// Data transfer object
layout(location = 0)out struct data_transfer_object {
    vec3 frag_position;
    vec4 shadow_coord_directional;
}OutDTO;

const mat4 shadow_bias = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

void main() {

    OutDTO.frag_position = vec3(PC.model * vec4(in_position, 1.0));

    gl_Position = UBO.projection * UBO.view * PC.model * vec4(in_position, 1.0);

    OutDTO.shadow_coord_directional = (shadow_bias * UBO.light_space_directional * PC.model) * vec4(in_position, 1.0);
}