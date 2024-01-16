#version 450
#extension GL_EXT_scalar_block_layout : require
#extension GL_GOOGLE_include_directive : require

#include "include/space_transforms.glsl"
#include "include/shadow_sampling.glsl"

// Globals
layout(std430, set = 0, binding = 0)uniform global_frag_uniform_buffer {
	mat4 projection_inverse;
	mat4 view_inverse;
	mat4 light_space_directional;
}GlobalUBO;

// Samplers
const int depth_i = 0;
const int dir_shadows_i = 1;
layout(set = 0, binding = 1)uniform sampler2D samplers[2];

// IO
layout(location = 0)in vec2 in_texture_coords;
layout(location = 0)out float out_shadow;

// Helper functions
// vec3 get_frag_world_position(vec2 coords);
vec4 get_shadow_coords(vec3 world_pos);

void main() {
	vec3 world_position = screen_to_world(
		in_texture_coords,
		texture(samplers[depth_i], in_texture_coords).r,
		GlobalUBO.projection_inverse,
		GlobalUBO.view_inverse
	);
	vec4 shadow_coord = get_shadow_coords(world_position);
	out_shadow = filterPCF(shadow_coord, world_position, samplers[dir_shadows_i], true);
}

vec4 get_shadow_coords(vec3 world_pos) {
	vec4 shadow_coords = shadow_bias_mat * GlobalUBO.light_space_directional * vec4(world_pos, 1.0);
	return shadow_coords / shadow_coords.w;
}