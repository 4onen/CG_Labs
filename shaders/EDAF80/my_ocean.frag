#version 410

uniform vec3 camera_position;
uniform samplerCube cubemap_texture;
uniform sampler2D bump_texture;

uniform vec3 water_deep_color;
uniform vec3 water_shallow_color;

uniform float waveBumpScale;
uniform float reflectionScale;
uniform float refractionScale;

in VS_OUT {
	vec3 vertex;
	mat3 tang_to_world;
	vec2 bumpCoord[3];
} fs_in;

out vec4 frag_color;

void main(){
	vec3 normalTexs[3];
	normalTexs[0] = texture(bump_texture,fs_in.bumpCoord[0]).xyz-0.5;
	normalTexs[1] = texture(bump_texture,fs_in.bumpCoord[1]).xyz-0.5;
	normalTexs[2] = texture(bump_texture,fs_in.bumpCoord[2]).xyz-0.5;

	//Blending using the method from AMD Ruby: Whiteout, at SIGGRAPH 2007:
	//  https://blog.selfshadow.com/publications/blending-in-detail/
	//Chose this method because expansion beyond two textures is trivial.
	// vec3 blendedNormal = normalize(vec3(
	// 	normalTexs[0].xy+normalTexs[1].xy+normalTexs[2].xy,
	// 	normalTexs[0].z*normalTexs[1].z*normalTexs[2].z
	// ));

	vec3 blendedNormal = normalize(
		2.0*(normalTexs[0]+normalTexs[1]+normalTexs[2])
	);

	//This LERP is awful, but this is a debug slider anyway.
	blendedNormal = mix(vec3(0.0,0.0,1.0),blendedNormal,waveBumpScale);

	mat3 tang_to_world = mat3(
		normalize(fs_in.tang_to_world[0]),
		normalize(fs_in.tang_to_world[1]),
		normalize(fs_in.tang_to_world[2])
	);//*/

	vec3 normal = tang_to_world*blendedNormal;

	vec3 toCamera = normalize(camera_position - fs_in.vertex);

	float facing = 1.0-max(dot(toCamera,normal),0.0);
	vec3 water_color = mix(water_deep_color,water_shallow_color,facing);

	float fastFresnel = clamp(0.02037 + (1.0-0.02037)*pow(1.0-dot(toCamera,normal),5.0),0.0,1.0);
	vec3 reflection = reflectionScale*texture(cubemap_texture,reflect(-toCamera,normal)).rgb;

	vec3 refractVec = refract(-toCamera,normal,1.0/1.33);
	vec3 refraction = refractionScale*texture(cubemap_texture,refractVec).rgb*(1-fastFresnel);

	vec3 retCol =
				water_color
				+ reflection * fastFresnel
				+ water_deep_color * refraction * (1.0-fastFresnel);
	
	frag_color = vec4(pow(retCol,vec3(1.0/2.2)),1.0);
}
