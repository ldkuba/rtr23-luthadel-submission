#version 450

layout(set=1,binding=1)uniform local_uniform_buffer{
    vec4 diffuse_color;
}lub;
layout(set=1,binding=2)uniform sampler2D texture_sampler;

layout(location=0)in vec2 frag_texture_coordinate;

layout(location=0)out vec4 outColor;

void main(){
    outColor=vec4(texture(texture_sampler,frag_texture_coordinate).rgb,1.);
}