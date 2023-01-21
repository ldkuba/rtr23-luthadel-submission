#version 450

// Uniforms
layout(set=1,binding=0)uniform local_uniform_buffer{
    vec4 diffuse_color;
    float shininess;
}UBO;

// Samplers
const int diffuse_i=0;
const int specular_i=1;
const int normal_i=2;
layout(set=1,binding=1)uniform sampler2D samplers[3];

// Light
struct DirectionalLight{
    vec3 direction;
    vec4 color;
};
struct PointLight{
    vec3 position;
    vec4 color;
    
    // Fallof factors:
    float constant;// Must be >= 1
    float linear;
    float quadratic;
};

// TODO: Temp
DirectionalLight directional_light={
    vec3(-.57735,-.57735,-.57735),
    vec4(.8,.8,.8,1.)
};

PointLight pl0={
    vec3(0,-5,-.5),
    vec4(0,1,0,1),
    1,.35,.44
};
PointLight pl1={
    vec3(-5,0,-.5),
    vec4(1,0,0,1),
    1,.35,.44
};

// TODO: Temp end

// Tangent bi-tangent normal
mat3 TBN;

// === I/O ===
layout(location=0)flat in uint in_mode;
// Data transfer object
layout(location=1)in struct data_transfer_object{
    vec4 ambient_color;
    vec3 surface_normal;
    vec4 surface_tangent;
    vec2 texture_coordinate;
    vec3 view_position;
    vec3 frag_position;
    vec4 color;
}DTO;

layout(location=0)out vec4 out_color;

// Function prototypes
vec4 calculate_directional_lights(DirectionalLight light,vec3 normal,vec3 view_direction);
vec4 calculate_point_lights(PointLight light,vec3 normal,vec3 frag_position,vec3 view_direction);

// Main
void main(){
    vec3 normal=DTO.surface_normal;
    vec3 tangent=DTO.surface_tangent.xyz;
    
    // Make sure tanget is really orthogonal to normal vector
    // For this we use Gram-Schmidt process
    // tangent -= (projection of tangent vector onto normal vector)
    tangent-=dot(tangent,normal)*normal;
    
    vec3 bitangent=cross(tangent,normal)*DTO.surface_tangent.w;
    TBN=mat3(tangent,bitangent,normal);
    
    // Texture normal sample (normalized to range 0 - 1)
    vec3 local_normal=2.*sqrt(texture(samplers[normal_i],DTO.texture_coordinate).rgb)-1.;
    normal=normalize(TBN*local_normal);
    
    if(in_mode==0||in_mode==1){
        vec3 view_direction=normalize(DTO.view_position-DTO.frag_position);
        out_color=calculate_directional_lights(directional_light,normal,view_direction);
        
        out_color+=calculate_point_lights(pl0,normal,DTO.frag_position,view_direction);
        out_color+=calculate_point_lights(pl1,normal,DTO.frag_position,view_direction);
    }else if(in_mode==2){
        out_color=vec4(abs(normal),1.);
    }
}

// Functions
vec4 calculate_directional_lights(DirectionalLight light,vec3 normal,vec3 view_direction){
    // Diffuse color
    float diffuse_factor=max(dot(normal,-light.direction),0.);
    vec4 diffuse_samp=texture(samplers[diffuse_i],DTO.texture_coordinate);
    
    vec4 diffuse=vec4(vec3(light.color*diffuse_factor),diffuse_samp.a);
    
    // Ambient color
    vec4 ambient=vec4(vec3(DTO.ambient_color*UBO.diffuse_color),diffuse_samp.a);
    
    // Specular highlight
    vec3 half_direction=normalize(view_direction-light.direction);
    float specular_factor=pow(max(0.,dot(normal,half_direction)),UBO.shininess);
    
    vec4 specular=vec4(vec3(light.color*specular_factor),1.);
    
    // Add texture info
    if(in_mode==0){
        diffuse*=diffuse_samp;
        ambient*=diffuse_samp;
        specular*=vec4(texture(samplers[specular_i],DTO.texture_coordinate).rgb,diffuse.a);
    }
    
    return ambient+diffuse+specular;
}

vec4 calculate_point_lights(PointLight light,vec3 normal,vec3 frag_position,vec3 view_direction){
    vec3 light_direction=normalize(light.position-frag_position);
    float diffuse_factor=max(0.,dot(normal,light_direction));
    
    vec3 reflect_dir=reflect(-light_direction,normal);
    float specular_factor=pow(max(0.,dot(view_direction,reflect_dir)),UBO.shininess);
    
    // Callculate falloff with distance (Attenuation)
    float distance=length(light.position-frag_position);
    float attenuation=1./(light.constant+light.linear*distance+light.quadratic*distance*distance);
    
    vec4 ambient=DTO.ambient_color;
    vec4 diffuse=light.color*diffuse_factor;
    vec4 specular=light.color*specular_factor;
    
    // Apply texture
    if(in_mode==0){
        vec4 diffuse_samp=texture(samplers[diffuse_i],DTO.texture_coordinate);
        diffuse*=diffuse_samp;
        ambient*=diffuse_samp;
        specular*=vec4(texture(samplers[specular_i],DTO.texture_coordinate).rgb,diffuse.a);
    }
    
    // Apply attenuation
    diffuse*=attenuation;
    ambient*=attenuation;
    specular*=attenuation;
    
    return ambient+diffuse+specular;
}