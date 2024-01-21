#version 450
#extension GL_EXT_scalar_block_layout : require
#extension GL_GOOGLE_include_directive : require

#include "include/space_transforms.glsl"
#include "include/noise.glsl"
#include "include/shadow_sampling.glsl"

#define MAX_DIRECTIONAL_CASCADES 4

// Global uniforms
layout(std430, set = 0, binding = 0)uniform global_frag_uniform_buffer {
    mat4 projection_inverse;
    mat4 view_inverse;
    vec4 camera_position; // TODO: extract from view_inverse
    mat4 light_space_directional[MAX_DIRECTIONAL_CASCADES];
    vec4 light_pos_directional;
    vec4 light_color_directional;
    float animation_time;
    uint num_directional_cascades;
}GlobalUBO;

// Samplers
layout(set = 0, binding = 1)uniform sampler2D depth_sampler;
layout(set = 0, binding = 2)uniform sampler2D shadow_sampler[4];

// IO
layout(location = 0)in vec2 in_texture_coords;
layout(location = 0)out vec4 out_color;

// Helper functions
vec4 raymarch_volume(vec3 start_pos, vec3 end_pos, int max_samples, float initial_step_size, float frag_depth);
float sample_density(vec3 position);

// float checkerboard(in vec2 uv) {
//         vec2 pos = floor(uv);
//       	return mod(pos.x + mod(pos.y, 2.0), 2.0);
// }

const int MAX_SAMPLES = 8;

void main() {    
    // Doesn't help at all :(
    // float d1 = textureOffset(depth_sampler, in_texture_coords, ivec2(0, 0)).x;
    // float d2 = textureOffset(depth_sampler, in_texture_coords, ivec2(0, 1)).x;
    // float d3 = textureOffset(depth_sampler, in_texture_coords, ivec2(1, 1)).x;
    // float d4 = textureOffset(depth_sampler, in_texture_coords, ivec2(1, 0)).x;
    
    // /*
    // * we select once the minimum once the maximum depth
    // * following a checkerboard pattern:
    // */
    // float depth = min(min(d1, d2), min(d3, d4));

    float depth = texture(depth_sampler, in_texture_coords).x;

    // if(depth == 1.0) {
    //     out_color = vec4(1.0, 1.0, 1.0, 1.0);
    //     return;
    // }

    vec3 world_pos = screen_to_world(
        in_texture_coords,
        depth,
        GlobalUBO.projection_inverse,
        GlobalUBO.view_inverse
    );
    out_color = raymarch_volume(GlobalUBO.camera_position.xyz, world_pos, MAX_SAMPLES, 5.0, depth);
}

const float PI = 3.1415926535897932384626433832795;
const float PI_RCP = 1.0 / PI;

float rayleigh(float cos_theta) {
    return (3.0f / (16.0f * PI)) * (1 + cos_theta * cos_theta);
}

float dither_pattern[16] = float[16](// “bayer” matrix
    0.0, 0.5, 0.125, 0.625,
    0.75, 0.22, 0.875, 0.375,
    0.1875, 0.6875, 0.0625, 0.5625,
    0.9375, 0.4375, 0.8125, 0.3125
);

vec4 raymarch_volume(vec3 start_pos, vec3 end_pos, int max_samples, float initial_step_size, float frag_depth) {    
    // Step vector
    vec3 step_vector = normalize(end_pos - start_pos);
    
    // Total distance
    float dist = distance(start_pos, end_pos);
    
    // Current position
    vec3 pos = start_pos;
    
    int dither_index = (int(gl_FragCoord.x)% 4) * 4 + (int(gl_FragCoord.y)%4);
    float dither_value = dither_pattern[dither_index];
    
    float remaining_distance = dist;
    float current_step_size = initial_step_size;
    
    // Step to first sample
    pos += step_vector * dither_value * current_step_size;
    remaining_distance -= dither_value * current_step_size;
    
    int num_fitted_samples = max(1, int(remaining_distance / current_step_size));
    while(num_fitted_samples * 2 < max_samples) {
        current_step_size *= 0.5;
        num_fitted_samples *= 2;
    }
    
    vec4 ret_color = vec4(0.0, 0.0, 0.0, 1.0);
    
    int sample_index = 0;
    while(remaining_distance > 0.0 && sample_index < max_samples) {
        float density = sample_density(pos);
        
        // Find appropriate cascade
        bool shadow_coords_found = false;
        vec4 shadow_coords;
        uint cascade_index = 0;
        for(int i = 0; i < GlobalUBO.num_directional_cascades; i++) {
            shadow_coords = shadow_bias_mat * GlobalUBO.light_space_directional[i] * vec4(pos, 1.0);
            shadow_coords /= shadow_coords.w;
            if(shadow_coords.x >= 0.0 && shadow_coords.x <= 1.0 && shadow_coords.y >= 0.0 && shadow_coords.y <= 1.0) {
                shadow_coords_found = true;
                break;
            }
            cascade_index++;
        }

        // Sample directional shadows
        float shadow = 0.0;
        if(shadow_coords_found) {
            shadow = filterPCF(shadow_coords, pos, shadow_sampler[cascade_index], false);
        }

        // Ambient light scattering
        if (shadow == 0.0) {
            shadow = 0.1;
        }
        
        // Not needed for directional light because it has constant intensity
        vec3 light_ray = pos - GlobalUBO.light_pos_directional.xyz;
        float light_distance = length(light_ray);
        
        // TODO: add Rayleigh scattering
        float cos_theta = dot(step_vector, normalize(light_ray));
        float scattering = rayleigh(cos_theta);
        
        // Calculate light change
        // vec4 delta = density * // Probability of collision (density)
        // shadow * // Shadow strength
        // scattering * GlobalUBO.light_color_directional * // scattering function + color
        // exp(-density * light_distance) * // This controlls how fast light influence decays (smoke color)
        // exp(-remaining_distance * density) * // I dont even know what this does anymore
        // current_step_size;
    
        float influence = 1.0;
        if(remaining_distance < current_step_size) {
            influence = remaining_distance / current_step_size;
        }

        ret_color += influence * density * shadow * scattering * GlobalUBO.light_color_directional * current_step_size;
        
        // Advance the ray, check if we can squeeze samples in between
        pos += step_vector * current_step_size;
        remaining_distance -= current_step_size;
        sample_index++;
    }

    if(frag_depth == 1.0) {
        ret_color += vec4(0.0, 0.0, 0.0, 0.0);
    }

    return ret_color;
}

float sample_density(vec3 pos) {
    vec3 smoke_move_dir = normalize(vec3(0.0, 2.0, 1.0));
    float smoke_move_speed = 0.001;
    vec3 noise_sample_position = pos + smoke_move_speed * smoke_move_dir * GlobalUBO.animation_time;
    float jitter_offset = 0.5 * snoise(noise_sample_position);
    float inv_worley = 1.0 - worley(
        pos * 0.2, 1.5 + jitter_offset, 
        false
    ).y;
    float density = exp(5.0 * inv_worley) * 0.2;
    return density;
}
