#version 450
#extension GL_EXT_scalar_block_layout : require
// #extension GL_ARB_shading_language_include : require

// #include "noise.glsl"

// Consts

// Global uniforms
layout(std430, set = 0, binding = 0)uniform global_frag_uniform_buffer {
    mat4 projection_inverse;
    mat4 view_inverse;
    vec4 camera_position; // TODO: extract from view_inverse
    mat4 light_space_directional;
    vec4 light_pos_directional;
    vec4 light_color_directional;
}GlobalUBO;

// Samplers
const int color_i = 0;
const int depth_i = 1;
const int shadow_i = 2;
layout(set = 0, binding = 1)uniform sampler2D samplers[3];

// IO
layout(location = 0)in vec2 in_texture_coords;
layout(location = 0)out vec4 out_color;

// Calculate view position from depth
vec3 get_frag_world_position(vec2 coords);
vec4 raymarch_volume(vec4 frag_color, vec3 start_pos, vec3 end_pos, int num_samples);

//	Simplex 3D Noise 
//	by Ian McEwan, Ashima Arts
//
vec4 permute(vec4 x){return mod(((x*34.0)+1.0)*x, 289.0);}
vec4 taylorInvSqrt(vec4 r){return 1.79284291400159 - 0.85373472095314 * r;}

float snoise(vec3 v){ 
  const vec2  C = vec2(1.0/6.0, 1.0/3.0) ;
  const vec4  D = vec4(0.0, 0.5, 1.0, 2.0);

// First corner
  vec3 i  = floor(v + dot(v, C.yyy) );
  vec3 x0 =   v - i + dot(i, C.xxx) ;

// Other corners
  vec3 g = step(x0.yzx, x0.xyz);
  vec3 l = 1.0 - g;
  vec3 i1 = min( g.xyz, l.zxy );
  vec3 i2 = max( g.xyz, l.zxy );

  //  x0 = x0 - 0. + 0.0 * C 
  vec3 x1 = x0 - i1 + 1.0 * C.xxx;
  vec3 x2 = x0 - i2 + 2.0 * C.xxx;
  vec3 x3 = x0 - 1. + 3.0 * C.xxx;

// Permutations
  i = mod(i, 289.0 ); 
  vec4 p = permute( permute( permute( 
             i.z + vec4(0.0, i1.z, i2.z, 1.0 ))
           + i.y + vec4(0.0, i1.y, i2.y, 1.0 )) 
           + i.x + vec4(0.0, i1.x, i2.x, 1.0 ));

// Gradients
// ( N*N points uniformly over a square, mapped onto an octahedron.)
  float n_ = 1.0/7.0; // N=7
  vec3  ns = n_ * D.wyz - D.xzx;

  vec4 j = p - 49.0 * floor(p * ns.z *ns.z);  //  mod(p,N*N)

  vec4 x_ = floor(j * ns.z);
  vec4 y_ = floor(j - 7.0 * x_ );    // mod(j,N)

  vec4 x = x_ *ns.x + ns.yyyy;
  vec4 y = y_ *ns.x + ns.yyyy;
  vec4 h = 1.0 - abs(x) - abs(y);

  vec4 b0 = vec4( x.xy, y.xy );
  vec4 b1 = vec4( x.zw, y.zw );

  vec4 s0 = floor(b0)*2.0 + 1.0;
  vec4 s1 = floor(b1)*2.0 + 1.0;
  vec4 sh = -step(h, vec4(0.0));

  vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
  vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;

  vec3 p0 = vec3(a0.xy,h.x);
  vec3 p1 = vec3(a0.zw,h.y);
  vec3 p2 = vec3(a1.xy,h.z);
  vec3 p3 = vec3(a1.zw,h.w);

//Normalise gradients
  vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
  p0 *= norm.x;
  p1 *= norm.y;
  p2 *= norm.z;
  p3 *= norm.w;

// Mix final noise value
  vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
  m = m * m;
  return 42.0 * dot( m*m, vec4( dot(p0,x0), dot(p1,x1), 
                                dot(p2,x2), dot(p3,x3) ) );
}

vec3 snoiseVec3(vec3 x){
  float s  = snoise(vec3( x ));
  float s1 = snoise(vec3( x.y - 19.1 , x.z + 33.4 , x.x + 47.2 ));
  float s2 = snoise(vec3( x.z + 74.2 , x.x - 124.5 , x.y + 99.4 ));
  vec3 c = vec3( s , s1 , s2 );
  return c;
}

vec3 curlNoise(vec3 p){
  const float e = .1;
  vec3 dx = vec3( e   , 0.0 , 0.0 );
  vec3 dy = vec3( 0.0 , e   , 0.0 );
  vec3 dz = vec3( 0.0 , 0.0 , e   );

  vec3 p_x0 = snoiseVec3( p - dx );
  vec3 p_x1 = snoiseVec3( p + dx );
  vec3 p_y0 = snoiseVec3( p - dy );
  vec3 p_y1 = snoiseVec3( p + dy );
  vec3 p_z0 = snoiseVec3( p - dz );
  vec3 p_z1 = snoiseVec3( p + dz );

  float x = p_y1.z - p_y0.z - p_z1.y + p_z0.y;
  float y = p_z1.x - p_z0.x - p_x1.z + p_x0.z;
  float z = p_x1.y - p_x0.y - p_y1.x + p_y0.x;

  const float divisor = 1.0 / ( 2.0 * e );
  return vec3( x , y , z ) * divisor;
}

void main() {
    vec3 world_pos = get_frag_world_position(in_texture_coords);
    out_color = raymarch_volume(texture(samplers[color_i], in_texture_coords), world_pos, GlobalUBO.camera_position.xyz, 3);

    // float curl = length(curlNoise(world_pos / 5.0)) / 100.0;
    // out_color = vec4(curl, curl, curl, 1.0);
}

vec3 get_frag_world_position(vec2 coords) {
    float fragment_depth = texture(samplers[depth_i], coords).w;
    
    // Convert coords and fragment_depth to
    // normalized device coordinates (clip space)
    vec4 ndc = vec4(
        coords.x * 2.0 - 1.0,
        1.0 - coords.y * 2.0,
        fragment_depth,
        1.0
    );
    
    // Transform to view space using inverse camera projection
    vec4 view_pos = GlobalUBO.projection_inverse * ndc;
    // Since we used a projection transformation (even if it was in inverse)
    // we need to convert our homogen eous coordinates using the perspective divide.
    view_pos /= view_pos.w;
    
    // Transform to world space using inverse view projection
    vec4 world_pos = GlobalUBO.view_inverse * view_pos;
    return world_pos.xyz;
}

// Shadowmap sampling

const mat4 shadow_bias_mat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

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
		float dist = texture(samplers[shadow_i], shadow_coord.st + offset).r;
		if (shadow_coord.w > 0.0 && dist < shadow_coord.z - shadow_bias) 
		{
			shadow = 0.0;
		}
	}
	return shadow;
}

float filterPCF(vec4 sc, vec3 world_pos)
{
	ivec2 texDim = textureSize(samplers[shadow_i], 0);
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
            int index = int(16.0 * random(floor(world_pos * 1000.0), i)) % 16;
			shadowFactor += textureProj(sc, vec2(dx*x, dy*y) + poissonDisk[index] / poissonSpread);
			count++;
		}
	
	}
	return shadowFactor / count;
}

const float PI_RCP = 1.0 / 3.1415926535897932384626433832795;

vec4 raymarch_volume(vec4 frag_color, vec3 start_pos, vec3 end_pos, int num_samples) {
    vec3 dir = normalize(end_pos - start_pos);
    float dist = distance(start_pos, end_pos);
    float step = dist / float(num_samples);

    // Start position
    vec4 start_pos_lightspace = shadow_bias_mat * GlobalUBO.light_space_directional * vec4(start_pos, 1.0);
    start_pos_lightspace /= start_pos_lightspace.w;

    // Step vector
    vec3 step_vector = dir * step;
    vec4 step_vector_lightspace = shadow_bias_mat * GlobalUBO.light_space_directional * vec4(step_vector, 0.0);
    step_vector_lightspace /= step_vector_lightspace.w;

    // Current position
    vec3 pos = start_pos;
    vec4 pos_lightspace = start_pos_lightspace;

    float remaining_distance = dist;

    for (int i = 0; i < num_samples; i++) {
        float density = length(curlNoise(pos / 5.0)) / 10.0;
        float shadow = filterPCF(pos_lightspace, pos);

        float pos_depth = length(pos - GlobalUBO.light_pos_directional.xyz);
        float pos_depth_recip = 1.0 / pos_depth;

        // Calculate light change
        frag_color += density * (shadow * (GlobalUBO.light_color_directional * 0.25 * PI_RCP) * pos_depth_recip * pos_depth_recip) * 
                        exp(-pos_depth * density) * 
                        exp(-remaining_distance * density) * 
                        step;

        // Advance the ray
        pos += step_vector;
        pos_lightspace += vec4(step_vector_lightspace.xyz, 0.0);
        remaining_distance -= step;
    }

    return frag_color;
}