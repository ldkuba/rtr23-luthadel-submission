#version 450

const int diffuse_i=0;
layout(set=1,binding=0)uniform samplerCube samplers[1];

layout(location=0)in vec3 in_texture_coordinate;
layout(location=0)out vec4 out_color;

void main(){
    out_color=texture(samplers[diffuse_i],in_texture_coordinate);
}