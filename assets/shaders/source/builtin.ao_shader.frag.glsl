#version 450
#extension GL_EXT_scalar_block_layout : require
#extension GL_GOOGLE_include_directive : require

#include "include/space_transforms.glsl"

// Const
const int MAX_KERNEL_SIZE = 64;
const float INV_MAX_KERNEL_SIZE_F = 1.0 / float(MAX_KERNEL_SIZE);
const vec2 HALF_2 = vec2(0.5);

// Global uniforms
layout(std430, set = 0, binding = 0)uniform global_frag_uniform_buffer {
    mat4 projection;
    mat4 projection_inverse;
    vec2 noise_scale;
    float sample_radius;
    vec3 kernel[MAX_KERNEL_SIZE];
}GlobalUBO;

// Samplers
const int depth_normals_i = 0;
const int noise_i = 1;
layout(set = 0, binding = 1)uniform sampler2D samplers[2];

// IO
layout(location = 0)in vec2 in_texture_coords;
layout(location = 0)out vec4 out_color;

vec3 stv(vec2 coords);
vec2 vtc(vec3 coords);

void main() {
    vec3 view_pos = stv(in_texture_coords);
    
    vec3 view_normal = texture(samplers[depth_normals_i], in_texture_coords).xyz;
    view_normal = normalize(view_normal);
    // vec3 view_normal = cross(dFdy(view_pos.xyz), dFdx(view_pos.xyz));
    // view_normal = normalize(view_normal * -1.0);
    
    // we calculate a random offset using the noise texture sample.
    //This will be applied as rotation to all samples for our current fragments.
    vec3 random_vector = texture(samplers[noise_i], in_texture_coords * GlobalUBO.noise_scale).xyz;
    random_vector = vec3(random_vector.xy * 2.0 - 1.0, random_vector.z);
    
    // here we apply the Gramm-Schmidt process to calculate the TBN matrix
    // with a random offset applied.
    vec3 tangent = normalize(random_vector - view_normal * dot(random_vector, view_normal));
    vec3 bitangent = cross(view_normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, view_normal);
    float occlusion_factor = 0.0;
    for(int i = 0; i < MAX_KERNEL_SIZE; i ++ ) {
        vec3 sample_pos = TBN * GlobalUBO.kernel[i].xyz;
        
        // here we calculate the sampling point position in view space.
        sample_pos = view_pos + sample_pos * GlobalUBO.sample_radius;
        
        // now using the sampling point offset
        vec2 offset = vtc(sample_pos);
        
        // this is the geometry's depth i.e. the view_space_geometry_depth
        // this value is negative in my coordinate system
        float geometry_depth = stv(offset.xy).z;
        
        float range_check = smoothstep(0.0, 1.0, GlobalUBO.sample_radius / abs(view_pos.z - geometry_depth));
        
        // samplePos.z is the sample's depth i.e. the view_space_sampling_position depth
        // this value is negative in my coordinate system
        // for occlusion to be true the geometry's depth should be greater or equal (equal or less negative and consequently closer to the camera) than the sample's depth
        occlusion_factor += float(geometry_depth >= sample_pos.z + 0.0001) * range_check;
    }
    
    // We will divide the accumulated occlusion by the number of samples to get the average occlusion value.
    float average_occlusion_factor = occlusion_factor * INV_MAX_KERNEL_SIZE_F;
    
    float visibility_factor = 1.0 - average_occlusion_factor;
    
    // We can raise the visibility factor to a power to make the transition
    // more sharp. Experiment with the value of this power to see what works best for you.
    // Even after raising visibility to a power > 1, the range still remains between [0.0, 1.0].
    visibility_factor = pow(visibility_factor, 2.0);
    
    // out_color = visibility_factor;
    out_color = vec4(visibility_factor, 0, 0, 1);
}

vec3 stv(vec2 coords) {
    float depth = texture(samplers[depth_normals_i], coords).w;
    return screen_to_view(
        coords,
        depth,
        GlobalUBO.projection_inverse
    );
}
vec2 vtc(vec3 coords) {
    return view_to_screen(
        coords,
        GlobalUBO.projection
    );
}