#version 450

// Uniforms
layout(set=1,binding=0)uniform local_uniform_buffer{
    vec4 diffuse_color;
    float shininess;
}UBO;

// Samplers
const int diffuse_i=0;
const int specular_i=1;
layout(set=1,binding=1)uniform sampler2D samplers[2];

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
    vec3 view_position;
    vec3 frag_position;
}DTO;

layout(location=0)out vec4 out_color;

// Function prototypes
vec4 calculate_directional_lights(DirectionalLight directional_light,vec3 normal,vec3 view_direction);

// Main
void main(){
    vec3 view_direction=normalize(DTO.view_position-DTO.frag_position);
    out_color=calculate_directional_lights(directional_light,DTO.surface_normal,view_direction);
}

// Functions
vec4 calculate_directional_lights(DirectionalLight directional_light,vec3 normal,vec3 view_direction){
    // Diffuse color
    float diffuse_factor=max(dot(normal,-directional_light.direction),0.);
    vec4 diffuse_samp=texture(samplers[diffuse_i],DTO.texture_coordinate);
    
    vec4 diffuse=vec4(vec3(directional_light.color*diffuse_factor),diffuse_samp.a);
    diffuse*=diffuse_samp;
    
    // Ambient color
    vec4 ambient=vec4(vec3(DTO.ambient_color*UBO.diffuse_color),diffuse_samp.a);
    ambient*=diffuse_samp;
    
    // Specular highlight
    vec3 half_direction=normalize(view_direction-directional_light.direction);
    float specular_factor=pow(max(0.,dot(normal,half_direction)),UBO.shininess);
    
    vec4 specular=vec4(vec3(directional_light.color*specular_factor),1.);
    specular*=vec4(texture(samplers[specular_i],DTO.texture_coordinate).rgb,diffuse.a);
    
    return diffuse+ambient+specular;
}