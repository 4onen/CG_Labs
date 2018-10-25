#version 410

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoord;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 binormal;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;

uniform float nowTime;

const uint MAX_WAVES = 6;
uniform uint num_waves;
uniform float waveAmplitudes[MAX_WAVES];
uniform vec2 waveDirections[MAX_WAVES];
uniform float waveFrequencies[MAX_WAVES];
uniform float wavePhases[MAX_WAVES];
uniform float waveSpikies[MAX_WAVES];
uniform float waveSpaceScale;

uniform vec2 texScale;
uniform vec2 bumpSpeed;

out VS_OUT {
	vec3 vertex;
	mat3 tang_to_world;
	vec2 bumpCoord[3];
} vs_out;

float myRand2(vec2 p){
	return fract(sin(dot(p.xy,vec2(12.9898,78.233)))*43758.5453123);
}

vec3 myNoise2(vec2 p){
	vec2 f = floor(p);
	vec2 x = fract(p);
	vec2 d = vec2(1.0,0.0);

	float noiseVal = mix( mix(myRand2(f),myRand2(f+d),x.x)
						, mix(myRand2(f+d.yx),myRand2(f+d.xx),x.x)
						, x.y
						);
	float dx = mix(myRand2(f+d)-myRand2(f),myRand2(f+d.xx)-myRand2(f+d.yx),x.y);
	float dy = mix(myRand2(f+d.yx)-myRand2(f+d.yy),myRand2(f+d.xx)-myRand2(f+d.yx),x.x);
	
	return vec3(noiseVal,dx,dy);
}

vec3 waveHeight(vec2 uv, float A, vec2 D, float f, float p, float k, float t){
	vec2 pnt = f*uv+D*p*t;
	vec3 n = myNoise2(uv+D*myNoise2(uv+D*myNoise2(pnt).x).x);
	n.x = pow(n.x,k);
	n.yz = pow(n.yz,vec2(k-1.0));
	return A*n;
}

void main()
{
	vec2 uv = (-1.0+2.0*texcoord.xy)*waveSpaceScale;
	float t = mod(nowTime,100.0);
	
	vec3 wave_height = vec3(0.0);

	if(num_waves>0){
		for(uint i=0;i<num_waves;++i){
			wave_height += waveHeight(uv,waveAmplitudes[i],waveDirections[i],waveFrequencies[i],wavePhases[i],waveSpikies[i],t);
		}
	}

	vec2 dist_to_texedgeXY = min(texcoord.xy,1.0-texcoord.xy);
	float dist_to_texedge = min(dist_to_texedgeXY.x,dist_to_texedgeXY.y);
	float texedgeFactor = smoothstep(0.0,0.1,dist_to_texedge);

	wave_height *= texedgeFactor;

	vec3 tang = normalize(vec3(1.0,0.0,wave_height.y));
	vec3 bino = normalize(vec3(0.0,1.0,wave_height.z));
	vec3 norm = cross(tang,bino);

	vs_out.vertex = vec3(vertex_model_to_world * vec4(vertex+normal*wave_height.x,1.0));


	vs_out.bumpCoord[0] = texcoord.xy*texScale + nowTime*bumpSpeed;
	vs_out.bumpCoord[1] = texcoord.xy*texScale*2 + nowTime*bumpSpeed*4;
	vs_out.bumpCoord[2] = texcoord.xy*texScale*4 + nowTime*bumpSpeed*8;

	//						world	<-	model	  ,	model	<-	modelsurface	, modelsurface <- wavesurface
	vs_out.tang_to_world = mat3(normal_model_to_world)*mat3(tangent,binormal,normal)*mat3(tang,bino,norm);

	gl_Position = vertex_world_to_clip * vec4(vs_out.vertex,1.0);
}