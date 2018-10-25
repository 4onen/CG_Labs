#version 410
#define PI 3.141592653589793
#define TAU 6.18318

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoord;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 binormal;

uniform bool has_diffuse_texture;

uniform mat4[100] vertex_model_to_world;
uniform mat4[100] normal_model_to_world;
uniform mat4 vertex_world_to_clip;

uniform float planet_radius;

uniform float nowTime;

const uint MAX_WAVES = 4;
uniform uint num_waves;
uniform float waveAmplitudes[MAX_WAVES];
uniform vec2 waveDirections[MAX_WAVES];
uniform float waveFrequencies[MAX_WAVES];
uniform float wavePhases[MAX_WAVES];
uniform float waveSpikies[MAX_WAVES];

out VS_OUT {
	vec3 vertex;
	mat3 tbn;
	vec2 texcoord;
	vec2 bumpCoords[2];
} vs_out;

mat3 rotate_by_dir(float angle, vec2 direction){
	//cross(vec3(direction.x,0.0,direction.y),vec3(0.0,1.0,0.0))
	vec3 ax = vec3(-direction.y,0.0,direction.x);
	float c = cos(-angle),
		  s = sin(-angle);

	return mat3(vec3(ax.x*ax.x+(1-ax.x*ax.x)*c, ax.z*s,ax.x*ax.z*(1-c)),
				vec3(                  -ax.z*s,      c,         ax.x*s),
				vec3(          ax.x*ax.z*(1-c),-ax.x*s,ax.z*ax.z+(1-ax.z*ax.z)*c)
			   );
}

//Returns:
// x - wave height
// y - wave derivative in x
// z - wave derivative in y
vec3 waveHeight(vec2 uv, float A, vec2 D, float f, float p, float k, float t){
	float trigContents = dot(D,uv)*f+t*p;
	float heightBeforePow = sin(trigContents)*0.5+0.5;
	float height = A*pow(heightBeforePow,k);
	float derivativeBase = k*A*pow(heightBeforePow,k-1.0)*f*cos(trigContents);
	return vec3(height,D*vec2(derivativeBase));
}

void main()
{
	const vec2 texScale = vec2(8.0,4.0);
	const vec2 bumpSpeed = vec2(-0.05f, 0.01f);
	vs_out.texcoord = texcoord.xy*texScale + nowTime*bumpSpeed;
	vs_out.bumpCoords[0] = texcoord.xy*texScale*2 + nowTime*bumpSpeed*4;
	vs_out.bumpCoords[1] = texcoord.xy*texScale*4 + nowTime*bumpSpeed*8;

	vec3 v = vec3(vertex_model_to_world[gl_InstanceID]*vec4(vertex,1.0));
	vec2 uv = v.xz;

	vec3 wave_height = vec3(0.0);
	for(uint i=0;i<num_waves;++i){
		wave_height += 
			waveHeight( uv
					  , waveAmplitudes[i]/4.0
					  , waveDirections[i]
					  , waveFrequencies[i]*2.0
					  , wavePhases[i]
					  , waveSpikies[i]
					  , nowTime);
	}

	v.y += wave_height.x;

	vec3 tang = normalize(vec3(1.0,0.0,wave_height.y));
	vec3 bino = normalize(vec3(0.0,1.0,wave_height.z));
	vec3 norm = cross(tang,bino);


	vs_out.tbn = mat3(normal_model_to_world[gl_InstanceID]) * 
				 mat3(tang,bino,norm) * 
				 mat3(tangent,binormal,normal);

	vec2 dir = vec2(0.0);
	float y = 0.0;
	if(!(v.x==0.0 && v.z==0.0)){
		dir = normalize(v.xz);
		y = length(v.xz);
	}
	float x = v.y;

	float h = planet_radius*exp(x/planet_radius);
	float phi = min(PI,y / planet_radius);

	if(!(dir.x==0.0 && dir.y==0.0)){
		vs_out.tbn = rotate_by_dir(phi,dir) * vs_out.tbn;
	}


	vec2 proj2d = h*vec2(cos(phi),sin(phi));
	vec3 proj3d;
	proj3d.yxz = vec3(proj2d.x,proj2d.y*dir);
	vs_out.vertex = proj3d;

	
	gl_Position = vertex_world_to_clip * vec4(proj3d,1.0);
}