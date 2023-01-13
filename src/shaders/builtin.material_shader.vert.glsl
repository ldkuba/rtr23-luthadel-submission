#version 450

layout(set=0,binding=0)uniform global_uniform_buffer{
    mat4 projection;
    mat4 view;
    vec4 ambient_color;
}UBO;

layout(push_constant)uniform push_constants{
    
    // Only guaranteed a total of 128 bytes.
    mat4 model;// 64 bytes
}PC;

layout(location=0)in vec3 in_position;
layout(location=1)in vec3 in_normal;
layout(location=2)in vec2 in_texture_coordinate;

// Data transfer object
layout(location=0)out struct data_transfer_object{
    vec4 ambient_color;
    vec3 surface_normal;
    vec2 texture_coordinate;
}DTO;

void main(){
    gl_Position=UBO.projection*UBO.view*PC.model*vec4(in_position,1.);
    DTO.ambient_color=UBO.ambient_color;
    DTO.surface_normal=mat3(PC.model)*in_normal;
    DTO.texture_coordinate=in_texture_coordinate;
}