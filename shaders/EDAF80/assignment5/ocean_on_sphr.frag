#version 410

uniform vec3 camera_position;

uniform samplerCube cubemap_texture;
uniform sampler2D bump_texture;

uniform vec3 water_deep_color;
uniform vec3 water_shallow_color;

in VS_OUT {
	vec3 vertex;
	mat3 tbn;
	vec2 texcoord;
	vec2 bumpCoords[2];
} fs_in;

out vec4 frag_color;

void main(){
	mat3 tang_to_world = mat3(
		normalize(fs_in.tbn[0]),
		normalize(fs_in.tbn[1]),
		normalize(fs_in.tbn[2])
	);

	vec3 texNormals[3];
	texNormals[0] = texture(bump_texture,fs_in.texcoord).xyz-0.5;
	texNormals[1] = texture(bump_texture,fs_in.bumpCoords[0]).xyz-0.5;
	texNormals[2] = texture(bump_texture,fs_in.bumpCoords[1]).xyz-0.5;

	vec3 blendedNormal = normalize(2.0*(texNormals[0]+texNormals[1]+texNormals[2]));

	vec3 normal = tang_to_world*blendedNormal;

	vec3 toCamera = normalize(camera_position - fs_in.vertex);

	float facing = 1.0-max(dot(toCamera,normal),0.0);
	vec3 water_color = mix(water_deep_color,water_shallow_color,facing);

	float fastFresnel = clamp(0.02037 + (1.0-0.02037)*pow(facing,5.0),0.0,1.0);
	vec3 reflection = texture(cubemap_texture,reflect(-toCamera,normal)).rgb;

	vec3 refractVec = refract(-toCamera,normal,1.0/1.33);
	vec3 refraction = texture(cubemap_texture,refractVec).rgb;

	vec3 retCol = water_color + mix(refraction,reflection,fastFresnel);
	
	frag_color = vec4(pow(retCol,vec3(1.0/2.2)),facing);
}
