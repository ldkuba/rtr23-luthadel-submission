// Shadowmap sampling
const mat4 shadow_bias_mat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

const float shadow_bias = 0.005;

vec2 poissonDisk_16[16] = vec2[]( 
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
vec2 poissonDisk_4[4] = vec2[](
  vec2( -0.94201624, -0.39906216 ),
  vec2( 0.94558609, -0.76890725 ),
  vec2( -0.094184101, -0.92938870 ),
  vec2( 0.34495938, 0.29387760 )
);

const float poissonSpread = 5000.0;

// Returns a random number based on a vec3 and an int.
float random(vec3 seed, int i){
	vec4 seed4 = vec4(seed,i);
	float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
	return fract(sin(dot_product) * 43758.5453);
}

float textureProj(vec4 shadow_coord, vec2 offset, sampler2D shadow_texture)
{
    shadow_coord.y = 1.0 - shadow_coord.y;

	float shadow = 1.0;
	if ( shadow_coord.z > -1.0 && shadow_coord.z < 1.0 ) 
	{
		float dist = texture(shadow_texture, shadow_coord.st + offset).r;
		if (shadow_coord.w > 0.0 && dist < shadow_coord.z - shadow_bias) 
		{
			shadow = 0.0;
		}
	}
	return shadow;
}

float filterPCF(vec4 sc, vec3 world_pos, sampler2D shadow_texture, bool large)
{
	ivec2 texDim = textureSize(shadow_texture, 0);
	float scale = 1.5;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 1;
	
	for (int x = -range; x <= range; x++)
	{
		for (int y = -range; y <= range; y++)
		{
            int i = (y+range)*range*2 + (x+range);
			if(large) {
            	int index = int(16.0 * random(floor(world_pos * 1000.0), i)) % 16;
				shadowFactor += textureProj(sc, vec2(dx*x, dy*y) + poissonDisk_16[index] / poissonSpread, shadow_texture);
			} else {
				int index = int(4.0 * random(floor(world_pos * 1000.0), i)) % 4;
				shadowFactor += textureProj(sc, vec2(dx*x, dy*y) + poissonDisk_4[index] / poissonSpread, shadow_texture);
			}
			count++;
		}
	
	}
	return shadowFactor / count;
}