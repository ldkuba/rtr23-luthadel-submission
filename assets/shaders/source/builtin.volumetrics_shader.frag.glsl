#version 450
#extension GL_EXT_scalar_block_layout : require
#extension GL_GOOGLE_include_directive : require

#include "include/space_transforms.glsl"
#include "include/noise.glsl"
#include "include/shadow_sampling.glsl"

// Global uniforms
layout(std430, set = 0, binding = 0)uniform global_frag_uniform_buffer {
    mat4 projection_inverse;
    mat4 view_inverse;
    vec4 camera_position; // TODO: extract from view_inverse
    mat4 light_space_directional;
    vec4 light_pos_directional;
    vec4 light_color_directional;
    float animation_time;
}GlobalUBO;

// Samplers
const int depth_i = 0;
const int shadow_i = 1;
layout(set = 0, binding = 1)uniform sampler2D samplers[2];

// IO
layout(location = 0)in vec2 in_texture_coords;
layout(location = 0)out vec4 out_color;

// Helper functions
vec4 raymarch_volume(vec3 start_pos, vec3 end_pos, int max_samples, float initial_step_size);
float sample_density(vec3 position);

// float checkerboard(in vec2 uv) {
    //     vec2 pos = floor(uv);
    //   	return mod(pos.x + mod(pos.y, 2.0), 2.0);
// }

const int MAX_SAMPLES = 8;

void main() {
    float ndc_depth = texture(samplers[depth_i], in_texture_coords).w;
    
    vec3 world_pos = screen_to_world(
        in_texture_coords,
        ndc_depth,
        GlobalUBO.projection_inverse,
        GlobalUBO.view_inverse
    );
    out_color = raymarch_volume(GlobalUBO.camera_position.xyz, world_pos, MAX_SAMPLES, 5.0);
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

vec4 raymarch_volume(vec3 start_pos, vec3 end_pos, int max_samples, float initial_step_size) {
    // Start position
    vec4 start_pos_lightspace = shadow_bias_mat * GlobalUBO.light_space_directional * vec4(start_pos, 1.0);
    start_pos_lightspace /= start_pos_lightspace.w;
    
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
        
        vec4 pos_lightspace = shadow_bias_mat * GlobalUBO.light_space_directional * vec4(pos, 1.0);
        float shadow = filterPCF(pos_lightspace /= pos_lightspace.w, pos, samplers[shadow_i], false);
        
        // Ambient light scattering
        if (shadow == 0.0) {
            shadow = 0.15;
        }
        
        // Not needed for directional light because it has constant intensity
        vec3 light_ray = pos - GlobalUBO.light_pos_directional.xyz;
        float light_distance = length(light_ray);
        
        // TODO: add Rayleigh scattering
        float cos_theta = dot(step_vector, normalize(light_ray));
        float scattering = rayleigh(cos_theta);
        
        // Calculate light change
        ret_color += (density * 180.0) * // Probability of collision (density)
        shadow * // Shadow strength
        scattering * GlobalUBO.light_color_directional * // scattering function
        exp(-density * light_distance) * // This controlls how fast light influence decays (smoke color)
        exp(-remaining_distance * density) * // I dont even know what this does anymore
        current_step_size;
        
        // Advance the ray, check if we can squeeze samples in between
        pos += step_vector * current_step_size;
        remaining_distance -= current_step_size;
        sample_index ++ ;
    }
    
    return ret_color;
}

float sample_density(vec3 pos) {
    vec3 smoke_move_dir = normalize(vec3(0.0, 2.0, 1.0));
    float smoke_move_speed = 0.002;
    vec3 noise_sample_position = pos + smoke_move_speed * smoke_move_dir * GlobalUBO.animation_time;
    float jitter_offset = 0.5 * snoise(noise_sample_position);
    float inv_worley = 1.0 - worley(
        pos * 1.0, 1.7 + jitter_offset,
        false
    ).y;
    float density = exp(1.2 * inv_worley) / 30.0;
    return density;
}