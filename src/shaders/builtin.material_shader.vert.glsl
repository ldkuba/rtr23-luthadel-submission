#version 450
#extension GL_ARB_separate_shader_objects:enable

layout(set=0,binding=0)uniform global_uniform_buffer{
    mat4 view;
    mat4 project;
}gub;
layout(set=1,binding=0)uniform local_uniform_buffer{
    mat4 model;
    vec4 diffuse_color;
}lub;

layout(location=0)in vec3 in_position;
layout(location=1)in vec3 in_color;
layout(location=2)in vec2 in_texture_coordinate;

layout(location=0)out vec3 frag_color;
layout(location=1)out vec2 frag_texture_coordinate;

void main(){
    gl_Position=gub.project*gub.view*lub.model*vec4(in_position,1.);
    frag_color=in_color;
    frag_texture_coordinate=in_texture_coordinate;
}