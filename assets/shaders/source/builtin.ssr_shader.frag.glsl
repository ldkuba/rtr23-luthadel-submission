#version 450
#extension GL_EXT_scalar_block_layout : require
#extension GL_GOOGLE_include_directive : require

#include "include/space_transforms.glsl"

// Const
const uint MAX_STEPS = 50;
const uint BINARY_SEARCH_STEPS = 5;
const float STEP_SIZE = 0.06;
const float MAX_DISTANCE = 100;
const float THICKNESS = 0.1;

// Global uniforms
layout(std430, set = 0, binding = 0)uniform global_frag_uniform_buffer {
    mat4 projection;
    mat4 projection_inverse;
    mat4 view;
    mat4 view_inverse;
    vec3 view_origin;
}GlobalUBO;

// Samplers
const int color_i = 0;
const int g_pre_pass_i = 1;
layout(set = 0, binding = 1)uniform sampler2D samplers[2];

// IO
layout(location = 0)in vec2 in_texture_coords;
layout(location = 0)out vec4 out_color;

// Calculate view position from depth
vec3 stw(vec2 coords);
vec2 wts(vec3 coords);
float vignette(vec2 uv);
vec3 hash33(vec3 point);

void main() {
    // Gather needed data
    vec3 camera_pos = GlobalUBO.view_origin;
    vec3 world_position = stw(in_texture_coords);
    vec3 world_normal = normalize(texture(samplers[g_pre_pass_i], in_texture_coords).rgb);
    vec3 look_dir = normalize(world_position - camera_pos);
    vec3 ray_dir = normalize(reflect(look_dir, world_normal));
    
    // Setup parameters
    float smoothness = 0.9; // TODO:
    float visibility = 1.0;
    
    // Jitter ray direction based on smoothness
    ray_dir += (1.0 - smoothness) * (hash33(world_position * 10) - vec3(0.5, 0.5, 0.5));
    
    // Reset
    float distance_traveled = 0.0;
    float previous_distance = 0.0;
    float depth = THICKNESS;
    vec2 res_uv = in_texture_coords;
    float reflected = 0.0;
    
    // Start ray marching
    for(int j = 0; j < MAX_STEPS; j ++ ) {
        previous_distance = distance_traveled;
        distance_traveled += STEP_SIZE;
        
        // Get position & projected position
        vec3 ray_pos = world_position + ray_dir * distance_traveled;
        vec3 proj_pos = stw(wts(ray_pos));
        
        // Compare distances
        float ray_dist = distance(ray_pos, camera_pos);
        float proj_dist = distance(proj_pos, camera_pos);
        
        float depth = ray_dist - proj_dist;
        if (depth > 0 && depth < THICKNESS) {
            for(int k = 0; k < BINARY_SEARCH_STEPS; k ++ ) {
                // Get midpoint position & projected position
                float midpoint_distance = (distance_traveled + previous_distance) * 0.5;
                ray_pos = world_position + ray_dir * midpoint_distance;
                proj_pos = stw(wts(ray_pos));
                
                // Get new distances
                float ray_dist = distance(ray_pos, camera_pos);
                float proj_dist = distance(proj_pos, camera_pos);
                
                if (proj_dist <= ray_dist) {
                    distance_traveled = midpoint_distance;
                    res_uv = wts(ray_pos);
                    reflected = 1.0;
                } else previous_distance = midpoint_distance;
            }
            break;
        }
    }
    
    // Compute visibility
    visibility *= vignette(in_texture_coords);
    visibility *= clamp(dot(ray_dir, look_dir), 0, 1);
    visibility *= 1.0 - clamp(0.25 * length(stw(res_uv) - world_position), 0, 1); // Fade with distance
    visibility *= smoothness;
    visibility *= (res_uv.x < 0 || res_uv.x > 1 ? 0 : 1) * (res_uv.y < 0 || res_uv.y > 1 ? 0 : 1);
    visibility = clamp(visibility, 0, 1);
    
    // Compute final color
    vec3 color = texture(samplers[color_i], in_texture_coords).rgb;
    vec3 reflect_color = texture(samplers[color_i], res_uv).rgb * reflected;
    vec3 final_color = mix(color, reflect_color, visibility);
    if (reflected == 0)final_color = color;
    
    out_color = vec4(final_color, 1);
}

vec3 stw(vec2 coords) {
    return screen_to_world(
        coords,
        texture(samplers[g_pre_pass_i], coords).w,
        GlobalUBO.projection_inverse,
        GlobalUBO.view_inverse
    );
}
vec2 wts(vec3 coords) {
    return world_to_screen(
        coords,
        GlobalUBO.view,
        GlobalUBO.projection
    );
}

float vignette(vec2 uv) {
    // Get texel size
    vec2 texel_size = 1.0 / textureSize(samplers[color_i], 0);
    
    // Compute vignette
    vec2 k = abs(uv - 0.5) * 1;
    k.x *= texel_size.y * texel_size.x;
    return pow(clamp(1.0 - dot(k, k), 0, 1), 1);
}

vec3 hash33(vec3 point) {
    point = fract(point * vec3(0.1031, 0.1030, 0.0973));
    point += dot(point, point.yxz + 33.33);
    return fract((point.xxy + point.yxx) * point.zyx);
}