#version 450
#extension GL_EXT_scalar_block_layout : require
#extension GL_GOOGLE_include_directive : require

#include "include/space_transforms.glsl"
#include "include/shadow_sampling.glsl"

#define MAX_DIRECTIONAL_CASCADES 4

// Globals
layout(std430, set = 0, binding = 0)uniform global_frag_uniform_buffer {
	mat4 projection_inverse;
	mat4 view_inverse;
	mat4 light_spaces_directional[MAX_DIRECTIONAL_CASCADES];
	uint num_directional_cascades;
}GlobalUBO;

// Samplers
layout(set = 0, binding = 1)uniform sampler2D depth_sampler;
layout(set = 0, binding = 2)uniform sampler2D dir_shadows_samplers[MAX_DIRECTIONAL_CASCADES];

// IO
layout(location = 0)in vec2 in_texture_coords;
layout(location = 0)out float out_shadow;

// Helper functions
vec4 get_shadow_coords(vec3 world_pos, uint cascade_index);

void main() {
	vec3 world_position = screen_to_world(
		in_texture_coords,
		texture(depth_sampler, in_texture_coords).r,
		GlobalUBO.projection_inverse,
		GlobalUBO.view_inverse
	);

	bool shadow_coords_found = false;
	vec4 shadow_coords;
	uint cascade_index = 0;
	for(int i = 0; i < GlobalUBO.num_directional_cascades; i++) {
		shadow_coords = get_shadow_coords(world_position, cascade_index);
		if(shadow_coords.x >= 0.0 && shadow_coords.x <= 1.0 && shadow_coords.y >= 0.0 && shadow_coords.y <= 1.0) {
			shadow_coords_found = true;
			break;
		}
		cascade_index++;
	}

	if(shadow_coords_found) {
		out_shadow = filterPCF(shadow_coords, world_position, dir_shadows_samplers[cascade_index], true);
	} else {
		out_shadow = 0.0;
	}
}

vec4 get_shadow_coords(vec3 world_pos, uint cascade_index) {
	vec4 shadow_coords = shadow_bias_mat * GlobalUBO.light_spaces_directional[cascade_index] * vec4(world_pos, 1.0);
	return shadow_coords / shadow_coords.w;
}