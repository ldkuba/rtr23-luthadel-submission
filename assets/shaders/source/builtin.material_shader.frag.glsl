#version 450
#extension GL_EXT_scalar_block_layout : require

#define MAX_POINT_LIGHTS 10

// Light
struct DirectionalLight {
    vec4 direction;
    vec4 color;
};
struct PointLight {
    vec4 position;
    vec4 color;
    
    // Fallof factors:
    float constant; // Must be >= 1
    float linear;
    float quadratic;
    float padding;
};
struct NumPointLights {
    uint num; // For some reason always padded to 16
};

struct PhongProperties {
    vec4 diffuse_colour;
    vec3 padding;
    float shininess;
};

// Tangent bi-tangent normal
mat3 TBN;

// === I/O ===
// Global uniforms
layout(std430, set = 0, binding = 1)uniform global_frag_uniform_buffer {
    DirectionalLight directional_light;
    NumPointLights num_point_lights;
    PointLight point_lights[MAX_POINT_LIGHTS];
}GlobalUBO;

// Global InstSamplers
const int ssao_i = 0;
layout(set = 0, binding = 2)uniform sampler2D GlobalSamplers[1];

// Instance uniforms
layout(std430, set = 1, binding = 1)uniform local_uniform_buffer {
    vec4 diffuse_color;
    float shininess;
}UBO;

// Instance InstSamplers
const int diffuse_i = 0;
const int specular_i = 1;
const int normal_i = 2;
layout(set = 1, binding = 2)uniform sampler2D InstSamplers[3];

// From vertex shader
layout(location = 0)flat in uint in_mode;

// Data transfer object
layout(location = 1)in struct data_transfer_object {
    vec4 ambient_color;
    vec3 surface_normal;
    vec3 surface_tangent;
    vec2 texture_coordinate;
    vec3 view_position;
    vec3 frag_position;
    vec4 clip_position;
    vec4 color;
}InDTO;

layout(location = 0)out vec4 out_color;

// Function prototypes
vec4 sample_ssao();
vec4 calculate_directional_lights(DirectionalLight light, vec3 normal, vec3 view_direction);
vec4 calculate_point_lights(PointLight light, vec3 normal, vec3 frag_position, vec3 view_direction);

// Main
void main() {
    vec3 normal = InDTO.surface_normal;
    vec3 tangent = InDTO.surface_tangent;
    vec3 bitangent = cross(tangent, normal);
    
    // Make sure tanget is really orthogonal to normal vector
    // For this we use Gram-Schmidt process
    // tangent -= (projection of tangent vector onto normal vector)
    tangent -= dot(tangent, normal) * normal;
    
    TBN = mat3(tangent, bitangent, normal);
    
    // Texture normal sample (normalized to range 0 - 1)
    // vec3 local_normal=2.*sqrt(texture(InstSamplers[normal_i],InDTO.texture_coordinate).rgb)-1.;
    vec3 local_normal = 2.0 * texture(InstSamplers[normal_i], InDTO.texture_coordinate).rgb - 1.0;
    normal = normalize(TBN * local_normal);
    
    if (in_mode == 0||in_mode == 1 || in_mode == 4) {
        vec3 view_direction = normalize(InDTO.view_position - InDTO.frag_position);
        out_color = calculate_directional_lights(GlobalUBO.directional_light, normal, view_direction);
        
        for(int i = 0; i < min(GlobalUBO.num_point_lights.num, MAX_POINT_LIGHTS); i ++ ) {
            out_color += calculate_point_lights(GlobalUBO.point_lights[i], normal, InDTO.frag_position, view_direction);
        }
    } else if (in_mode == 2) {
        out_color = vec4(max(normal, 0), 1.0);
    } else if (in_mode == 3) {
        out_color = sample_ssao();
    }
}

// Functions
vec4 sample_ssao() {
    if (in_mode == 4)return vec4(1);
    vec2 ndc_position = InDTO.clip_position.xy / InDTO.clip_position.w;
    vec2 screen_position = ndc_position * 0.5 + 0.5;
    screen_position.y = 1.0 - screen_position.y;
    float vf = texture(GlobalSamplers[ssao_i], screen_position).r;
    return vec4(vec3(vf), 1);
}

vec4 calculate_directional_lights(DirectionalLight light, vec3 normal, vec3 view_direction) {
    // Diffuse color
    float diffuse_factor = max(dot(normal, - light.direction.xyz), 0.0);
    vec4 diffuse_samp = texture(InstSamplers[diffuse_i], InDTO.texture_coordinate);
    
    vec4 diffuse = vec4(
        vec3(light.color * diffuse_factor), diffuse_samp.a
    );
    
    // Ambient color
    vec4 visibility_factor = sample_ssao();
    vec4 ambient = visibility_factor * vec4(
        vec3(InDTO.ambient_color * UBO.diffuse_color), diffuse_samp.a
    );
    
    // Specular highlight
    vec3 half_direction = normalize(view_direction - light.direction.xyz);
    float specular_factor = pow(max(0.0, dot(normal, half_direction)), UBO.shininess);
    
    vec4 specular = vec4(
        vec3(light.color * specular_factor), diffuse_samp.a
    );
    
    // Add texture info
    if (in_mode == 0 || in_mode == 4) {
        diffuse *= diffuse_samp;
        ambient *= diffuse_samp;
        specular *= vec4(texture(InstSamplers[specular_i], InDTO.texture_coordinate).rgb, diffuse.a);
    }
    
    return ambient + diffuse + specular;
}

vec4 calculate_point_lights(PointLight light, vec3 normal, vec3 frag_position, vec3 view_direction) {
    vec3 light_direction = normalize(light.position.xyz - frag_position);
    float diffuse_factor = max(0.0, dot(normal, light_direction));
    
    vec3 reflect_dir = reflect(-light_direction, normal);
    float specular_factor = pow(max(0.0, dot(view_direction, reflect_dir)), UBO.shininess);
    
    // Account for SSAO
    vec4 visibility_factor = sample_ssao();
    
    // Callculate falloff with distance (Attenuation)
    float l_distance = length(light.position.xyz - frag_position);
    float attenuation = 1.0 / (light.constant + light.linear * l_distance + light.quadratic * l_distance * l_distance);
    
    vec4 ambient = InDTO.ambient_color * visibility_factor;
    vec4 diffuse = light.color * diffuse_factor;
    vec4 specular = light.color * specular_factor;
    
    // Apply texture
    if (in_mode == 0 || in_mode == 4) {
        vec4 diffuse_samp = texture(InstSamplers[diffuse_i], InDTO.texture_coordinate);
        diffuse *= diffuse_samp;
        ambient *= diffuse_samp;
        specular *= vec4(texture(InstSamplers[specular_i], InDTO.texture_coordinate).rgb, diffuse.a);
    }
    
    // Apply attenuation
    diffuse *= attenuation;
    ambient *= attenuation;
    specular *= attenuation;
    
    return ambient + diffuse + specular;
}