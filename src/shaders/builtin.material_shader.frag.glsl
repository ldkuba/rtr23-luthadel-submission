#version 450

// Uniforms
layout(set=1,binding=0)uniform local_uniform_buffer{
    vec4 diffuse_color;
}UBO;
layout(set=1,binding=1)uniform sampler2D sampler_diffuse;

// Light
struct DirectionalLight{
    vec3 direction;
    vec4 color;
};

// TODO: Temp
DirectionalLight directional_light={
    vec3(-.57735,-.57735,-.57735),
    vec4(.8,.8,.8,1.)
};

// I/O
layout(location=0)in struct data_transfer_object{
    vec4 ambient_color;
    vec3 surface_normal;
    vec2 texture_coordinate;
}DTO;

layout(location=0)out vec4 out_color;

// Function prototypes
vec4 calculate_directional_lights(DirectionalLight directional_light,vec3 normal);

// Main
void main(){
    out_color=calculate_directional_lights(directional_light,DTO.surface_normal);
}

// Functions
vec4 calculate_directional_lights(DirectionalLight directional_light,vec3 normal){
    float diffuse_factor=max(dot(normal,-directional_light.direction),0.);
    
    vec4 diffuse_samp=texture(sampler_diffuse,DTO.texture_coordinate);
    vec4 ambient=vec4(vec3(DTO.ambient_color*UBO.diffuse_color),diffuse_samp.a);
    vec4 diffuse=vec4(vec3(directional_light.color*diffuse_factor),diffuse_samp.a);
    
    diffuse*=diffuse_samp;
    ambient*=diffuse_samp;
    
    return diffuse+ambient;
}