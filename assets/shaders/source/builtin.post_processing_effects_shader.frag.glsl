#version 450
#extension GL_EXT_scalar_block_layout : require

// Globals
// Global uniforms
layout(std430, set = 0, binding = 0)uniform global_frag_uniform_buffer {
    // Tone mapping
    float exposure;
    // DOF
    float max_blur;
    float aperture;
    float focus;
    float aspect;
}GlobalUBO;

// Samplers
const int target_i = 0;
const int g_pre_pass_i = 1;
layout(set = 0, binding = 1)uniform sampler2D samplers[2];

// IO
layout(location = 0)in vec2 in_texture_coords;
layout(location = 0)out vec4 out_color;

vec4 compute_depth_of_field(
    float max_blur,
    float aperture,
    float focus,
    float aspect
);

void main() {
    vec4 color = texture(samplers[target_i], in_texture_coords);
    
    // Calculate depth of field effect
    vec4 dof_color = compute_depth_of_field(
        GlobalUBO.max_blur,
        GlobalUBO.aperture,
        GlobalUBO.focus,
        GlobalUBO.aspect
    );
    
    // Reinhard tone mapping
    vec3 mapped_color = dof_color.rgb / (dof_color.rgb + vec3(1.0));
    mapped_color = pow(mapped_color, vec3(1.0 / GlobalUBO.exposure));
    
    out_color = vec4(mapped_color, color.a);
}

// Code taken from https://github.com/neilmendoza/ofxPostProcessing/blob/master/src/DofPass.cpp
vec4 compute_depth_of_field(
    float max_blur,
    float aperture,
    float focus,
    float aspect
) {
    vec2 vUv = in_texture_coords.st;
    
    vec2 aspect_correct = vec2(1.0, aspect);
    
    float zNear = 0.1;
    float zFar = 1000.0;
    
    float depth = texture(samplers[g_pre_pass_i], vUv).w;
    // depth = (2.0 * zNear * zFar) / (zFar + zNear - depth * (zFar - zNear));
    
    float factor = depth - focus;
    
    vec2 dof_blur = vec2(clamp(factor * aperture, - max_blur, max_blur));
    
    vec2 dof_blur9 = dof_blur * 0.9;
    vec2 dof_blur7 = dof_blur * 0.7;
    vec2 dof_blur4 = dof_blur * 0.4;
    
    vec4 col = vec4(0.0);
    
    col += texture(samplers[target_i], vUv.xy);
    col += texture(samplers[target_i], vUv.xy + (vec2(0.0, 0.4) * aspect_correct) * dof_blur);
    col += texture(samplers[target_i], vUv.xy + (vec2(0.15, 0.37) * aspect_correct) * dof_blur);
    col += texture(samplers[target_i], vUv.xy + (vec2(0.29, 0.29) * aspect_correct) * dof_blur);
    col += texture(samplers[target_i], vUv.xy + (vec2(-0.37, 0.15) * aspect_correct) * dof_blur);
    col += texture(samplers[target_i], vUv.xy + (vec2(0.40, 0.0) * aspect_correct) * dof_blur);
    col += texture(samplers[target_i], vUv.xy + (vec2(0.37, - 0.15) * aspect_correct) * dof_blur);
    col += texture(samplers[target_i], vUv.xy + (vec2(0.29, - 0.29) * aspect_correct) * dof_blur);
    col += texture(samplers[target_i], vUv.xy + (vec2(-0.15, - 0.37) * aspect_correct) * dof_blur);
    col += texture(samplers[target_i], vUv.xy + (vec2(0.0, - 0.4) * aspect_correct) * dof_blur);
    col += texture(samplers[target_i], vUv.xy + (vec2(-0.15, 0.37) * aspect_correct) * dof_blur);
    col += texture(samplers[target_i], vUv.xy + (vec2(-0.29, 0.29) * aspect_correct) * dof_blur);
    col += texture(samplers[target_i], vUv.xy + (vec2(0.37, 0.15) * aspect_correct) * dof_blur);
    col += texture(samplers[target_i], vUv.xy + (vec2(-0.4, 0.0) * aspect_correct) * dof_blur);
    col += texture(samplers[target_i], vUv.xy + (vec2(-0.37, - 0.15) * aspect_correct) * dof_blur);
    col += texture(samplers[target_i], vUv.xy + (vec2(-0.29, - 0.29) * aspect_correct) * dof_blur);
    col += texture(samplers[target_i], vUv.xy + (vec2(0.15, - 0.37) * aspect_correct) * dof_blur);
    
    col += texture(samplers[target_i], vUv.xy + (vec2(0.15, 0.37) * aspect_correct) * dof_blur9);
    col += texture(samplers[target_i], vUv.xy + (vec2(-0.37, 0.15) * aspect_correct) * dof_blur9);
    col += texture(samplers[target_i], vUv.xy + (vec2(0.37, - 0.15) * aspect_correct) * dof_blur9);
    col += texture(samplers[target_i], vUv.xy + (vec2(-0.15, - 0.37) * aspect_correct) * dof_blur9);
    col += texture(samplers[target_i], vUv.xy + (vec2(-0.15, 0.37) * aspect_correct) * dof_blur9);
    col += texture(samplers[target_i], vUv.xy + (vec2(0.37, 0.15) * aspect_correct) * dof_blur9);
    col += texture(samplers[target_i], vUv.xy + (vec2(-0.37, - 0.15) * aspect_correct) * dof_blur9);
    col += texture(samplers[target_i], vUv.xy + (vec2(0.15, - 0.37) * aspect_correct) * dof_blur9);
    
    col += texture(samplers[target_i], vUv.xy + (vec2(0.29, 0.29) * aspect_correct) * dof_blur7);
    col += texture(samplers[target_i], vUv.xy + (vec2(0.40, 0.0) * aspect_correct) * dof_blur7);
    col += texture(samplers[target_i], vUv.xy + (vec2(0.29, - 0.29) * aspect_correct) * dof_blur7);
    col += texture(samplers[target_i], vUv.xy + (vec2(0.0, - 0.4) * aspect_correct) * dof_blur7);
    col += texture(samplers[target_i], vUv.xy + (vec2(-0.29, 0.29) * aspect_correct) * dof_blur7);
    col += texture(samplers[target_i], vUv.xy + (vec2(-0.4, 0.0) * aspect_correct) * dof_blur7);
    col += texture(samplers[target_i], vUv.xy + (vec2(-0.29, - 0.29) * aspect_correct) * dof_blur7);
    col += texture(samplers[target_i], vUv.xy + (vec2(0.0, 0.4) * aspect_correct) * dof_blur7);
    
    col += texture(samplers[target_i], vUv.xy + (vec2(0.29, 0.29) * aspect_correct) * dof_blur4);
    col += texture(samplers[target_i], vUv.xy + (vec2(0.4, 0.0) * aspect_correct) * dof_blur4);
    col += texture(samplers[target_i], vUv.xy + (vec2(0.29, - 0.29) * aspect_correct) * dof_blur4);
    col += texture(samplers[target_i], vUv.xy + (vec2(0.0, - 0.4) * aspect_correct) * dof_blur4);
    col += texture(samplers[target_i], vUv.xy + (vec2(-0.29, 0.29) * aspect_correct) * dof_blur4);
    col += texture(samplers[target_i], vUv.xy + (vec2(-0.4, 0.0) * aspect_correct) * dof_blur4);
    col += texture(samplers[target_i], vUv.xy + (vec2(-0.29, - 0.29) * aspect_correct) * dof_blur4);
    col += texture(samplers[target_i], vUv.xy + (vec2(0.0, 0.4) * aspect_correct) * dof_blur4);
    
    return vec4(col.rgb / 41.0, 1.0);
}