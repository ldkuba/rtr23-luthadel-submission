#version 450
#extension GL_EXT_scalar_block_layout : require

// Consts

// Global uniforms
layout(std430, set = 0, binding = 0)uniform global_frag_uniform_buffer {
    mat4 projection_inverse;
}GlobalUBO;

// Samplers
const int color_i = 0;
const int depth_normals_i = 0;
layout(set = 0, binding = 1)uniform sampler2D samplers[2];

// IO
layout(location = 0)in vec2 in_texture_coords;
layout(location = 0)out vec4 out_color;

// Calculate view position from depth
vec3 calculate_view_position(vec2 coords);

void main() {
    vec3 view_pos = calculate_view_position(in_texture_coords);
    out_color = texture(samplers[color_i], in_texture_coords);
}

vec3 calculate_view_position(vec2 coords) {
    float fragment_depth = texture(samplers[depth_normals_i], coords).w;
    
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
    
    // Since we used a projection transformation (even if it was in inverse)
    // we need to convert our homogen eous coordinates using the perspective divide.
    vs_pos.xyz = vs_pos.xyz / vs_pos.w;
    
    return vs_pos.xyz;
}
