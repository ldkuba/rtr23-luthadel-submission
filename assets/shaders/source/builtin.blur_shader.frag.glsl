#version 450
#extension GL_EXT_scalar_block_layout : require

// Consts
const float INV_TOTAL_SAMPLES_FACTOR = 1.0 / 16.0;

// Samplers
const int target_i = 0;
layout(set = 0, binding = 0)uniform sampler2D samplers[1];

// IO
layout(location = 0)in vec2 in_texture_coords;
layout(location = 0)out vec4 out_color;

// Calculate view position from depth
vec3 calculate_view_position(vec2 coords);

void main() {
    vec2 texel_size = 1.0 / vec2(textureSize(samplers[target_i], 0));
    vec3 blurred_target = vec3(0.0);
    for(int t = -2; t < 2; ++ t) {
        for(int s = -2; s < 2; ++ s) {
            vec2 offset = vec2(float(s), float(t)) * texel_size;
            
            blurred_target += texture(samplers[target_i], in_texture_coords + offset).rgb;
        }
    }
    
    out_color = vec4(blurred_target * INV_TOTAL_SAMPLES_FACTOR, 1);
}