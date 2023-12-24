#version 450
#extension GL_EXT_scalar_block_layout : require

// Consts
precision mediump float;
const int MAX_KERNEL_SIZE = 20;
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
const int depth_i = 0;
const int noise_i = 1;
layout(set = 0, binding = 1)uniform sampler2D samplers[2];

// IO
layout(location = 0)in vec2 in_texture_coords;
layout(location = 0)out vec4 out_color;

// Calculate view position from depth
vec3 calculate_view_position(vec2 coords);

void main() {
    float x = texture(samplers[0], in_texture_coords).r;
    out_color = vec4(texture(samplers[1], in_texture_coords).r, 0, 1, 1);
    return;
    
    vec3 view_pos = calculate_view_position(in_texture_coords);
    
    // The dFdy and dFdX are glsl functions used to calculate two vectors in view space that lie
    // on the plane of the surface being drawn. We pass the view space position to these functions.
    // The cross product of these two vectors give us the normal in view space.
    vec3 view_normal = cross(dFdy(view_pos.xyz), dFdx(view_pos.xyz));
    
    // The normal is initilly away from the screen based on the order in which we calculate the cross products.
    // Here, we need to invert it to point towards the screen by multiplying by -1.
    // Then we normalize this vector to get a unit normal vector.
    view_normal = normalize(view_normal * -1.0);
    // we calculate a random offset using the noise texture sample.
    //This will be applied as rotation to all samples for our current fragments.
    vec3 random_vector = texture(samplers[noise_i], in_texture_coords * GlobalUBO.noise_scale).xyz;
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
        vec4 offset = vec4(sample_pos, 1.0);
        offset = GlobalUBO.projection * offset;
        offset.xy /= offset.w;
        offset.xy = offset.xy * HALF_2 + HALF_2;
        
        // this is the geometry's depth i.e. the view_space_geometry_depth
        // this value is negative in my coordinate system
        float geometry_depth = calculate_view_position(offset.xy).z;
        
        float range_check = smoothstep(0.0, 1.0, GlobalUBO.sample_radius / abs(view_pos.z - geometry_depth));
        
        // samplePos.z is the sample's depth i.e. the view_space_sampling_position depth
        // this value is negative in my coordinate system
        // for occlusion to be true the geometry's depth should be greater or equal (equal or less negative and consequently closer to the camera) than the sample's depth
        occlusion_factor += float(geometry_depth >= sample_pos.z + 0.0001) * range_check;
    }
    
    // we will devide the accmulated occlusion by the number of samples to get the average occlusion value.
    float average_occlusion_factor = occlusion_factor * INV_MAX_KERNEL_SIZE_F;
    
    float visibility_factor = 1.0 - average_occlusion_factor;
    
    // We can raise the visibility factor to a power to make the transition
    // more sharp. Experiment with the value of this power to see what works best for you.
    // Even after raising visibility to a power > 1, the range still remains between [0.0, 1.0].
    visibility_factor = pow(visibility_factor, 2.0);
    
    // out_color = visibility_factor;
}

vec3 calculate_view_position(vec2 coords) {
    float fragment_depth = texture(samplers[depth_i], coords).r;
    
    // Convert coords and fragment_depth to
    // normalized device coordinates (clip space)
    vec4 ndc = vec4(
        coords.x * 2.0 - 1.0,
        coords.y * 2.0 - 1.0,
        fragment_depth * 2.0 - 1.0,
        1.0
    );
    
    // Transform to view space using inverse camera projection matrix.
    vec4 vs_pos = GlobalUBO.projection_inverse * ndc;
    
    // since we used a projection transformation (even if it was in inverse)
    // we need to convert our homogeneous coordinates using the perspective divide.
    vs_pos.xyz = vs_pos.xyz / vs_pos.w;
    
    return vs_pos.xyz;
}
