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

//Returns:
// [0] - wave height
// [1] - tangent
// [2] - binormal
mat3 waveHeight(vec2 uv, float A, vec2 D, float f, float p, float k, float t){
	float s = A*sin(dot(uv,D)*f+t*p),
		  c = A*cos(dot(uv,D)*f+t*p),
		  Q = k/10.0,
		  QDxyWAs = Q*D.x*D.y*f*s;
	return mat3(vec3(Q*D.x*c
					,Q*D.y*c
					,s
					)
			   ,vec3(1.0-Q*D.x*D.x*f*s
				    ,-QDxyWAs
					,D.x*f*c
			   		)
			   ,vec3(-QDxyWAs
				    ,1.0-Q*D.y*D.y*f*s
					,D.y*f*c
			   		)
			   );
}

void main()
{
	vec2 uv = (-1.0+2.0*texcoord.xy)*waveSpaceScale;
	float t = nowTime;
	
	mat3 wave_height = mat3(0.0);

	if(num_waves>0){
		for(uint i=0;i<num_waves;++i){
			wave_height += waveHeight(uv,waveAmplitudes[i],waveDirections[i],waveFrequencies[i],wavePhases[i],waveSpikies[i],t);
		}
	}

	vec2 dist_to_texedgeXY = min(texcoord.xy,1.0-texcoord.xy);
	float dist_to_texedge = min(dist_to_texedgeXY.x,dist_to_texedgeXY.y);
	float texedgeFactor = smoothstep(0.0,0.05,dist_to_texedge);
	wave_height[0] *= texedgeFactor;

	vec3 tang = normalize(mix(vec3(1.0,0.0,0.0),wave_height[1],texedgeFactor));
	vec3 bino = normalize(mix(vec3(0.0,1.0,0.0),wave_height[2],texedgeFactor));
	vec3 norm = cross(tang,bino);

	vs_out.bumpCoord[0] = texcoord.xy*texScale.xy + nowTime*bumpSpeed;
	vs_out.bumpCoord[1] = texcoord.xy*texScale.xy*2 + nowTime*bumpSpeed*4;
	vs_out.bumpCoord[2] = texcoord.xy*texScale.xy*4 + nowTime*bumpSpeed*8;

	mat3 modelsurface_to_model = mat3(tangent,binormal,normal);

	//						world	<-	model		  ,	model<-modelsurface	, modelsurface <- wavesurface
	vs_out.tang_to_world = mat3(normal_model_to_world)*modelsurface_to_model*mat3(tang,bino,norm);

	vs_out.vertex = vec3(vertex_model_to_world * vec4(vertex+modelsurface_to_model*wave_height[0],1.0));

	gl_Position = vertex_world_to_clip * vec4(vs_out.vertex,1.0);
}