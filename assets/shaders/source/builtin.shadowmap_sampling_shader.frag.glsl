#version 450
#extension GL_EXT_scalar_block_layout : require

layout(set = 0, binding = 1)uniform sampler2D ShadowMapDirectional;

// Data transfer object
layout(location = 0)in struct data_transfer_object {
    vec3 frag_position;
    vec4 shadow_coord_directional;
}InDTO;

layout(location = 0)out float out_shadow;

float textureProj(vec4 shadow_coord, vec2 offset);
float filterPCF(vec4 sc);

void main() {
    out_shadow = filterPCF(InDTO.shadow_coord_directional / InDTO.shadow_coord_directional.w);//textureProj(InDTO.shadow_coord_directional / InDTO.shadow_coord_directional.w, vec2(0.0));
    //out_shadow = 1.0;
}

const float shadow_bias = 0.005;

vec2 poissonDisk[16] = vec2[]( 
   vec2( -0.94201624, -0.39906216 ), 
   vec2( 0.94558609, -0.76890725 ), 
   vec2( -0.094184101, -0.92938870 ), 
   vec2( 0.34495938, 0.29387760 ), 
   vec2( -0.91588581, 0.45771432 ), 
   vec2( -0.81544232, -0.87912464 ), 
   vec2( -0.38277543, 0.27676845 ), 
   vec2( 0.97484398, 0.75648379 ), 
   vec2( 0.44323325, -0.97511554 ), 
   vec2( 0.53742981, -0.47373420 ), 
   vec2( -0.26496911, -0.41893023 ), 
   vec2( 0.79197514, 0.19090188 ), 
   vec2( -0.24188840, 0.99706507 ), 
   vec2( -0.81409955, 0.91437590 ), 
   vec2( 0.19984126, 0.78641367 ), 
   vec2( 0.14383161, -0.14100790 ) 
);
const float poissonSpread = 5000.0;

// Returns a random number based on a vec3 and an int.
float random(vec3 seed, int i){
	vec4 seed4 = vec4(seed,i);
	float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
	return fract(sin(dot_product) * 43758.5453);
}

float textureProj(vec4 shadow_coord, vec2 offset)
{
    shadow_coord.y = 1.0 - shadow_coord.y;

	float shadow = 1.0;
	if ( shadow_coord.z > -1.0 && shadow_coord.z < 1.0 ) 
	{
		float dist = texture(ShadowMapDirectional, shadow_coord.st + offset).r;
		if (shadow_coord.w > 0.0 && dist < shadow_coord.z - shadow_bias) 
		{
			shadow = 0.0;
		}
	}
	return shadow;
}

float filterPCF(vec4 sc)
{
	ivec2 texDim = textureSize(ShadowMapDirectional, 0);
	float scale = 1.5;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 2;
	
	for (int x = -range; x <= range; x++)
	{
		for (int y = -range; y <= range; y++)
		{
            int i = (y+range)*range*2 + (x+range);
            int index = int(16.0 * random(floor(InDTO.frag_position.xyz * 1000.0), i)) % 16;
			shadowFactor += textureProj(sc, vec2(dx*x, dy*y) + poissonDisk[index] / poissonSpread);
			count++;
		}
	
	}
	return shadowFactor / count;
}