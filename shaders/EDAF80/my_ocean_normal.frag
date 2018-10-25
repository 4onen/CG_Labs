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
	normalTexs[0] = texture(bump_texture,fs_in.bumpCoord[0]).xyz*2.0-1.0;
	normalTexs[1] = texture(bump_texture,fs_in.bumpCoord[1]).xyz*2.0-1.0;
	normalTexs[2] = texture(bump_texture,fs_in.bumpCoord[2]).xyz*2.0-1.0;

	//Blending using the method from AMD Ruby: Whiteout, at SIGGRAPH 2007:
	//  https://blog.selfshadow.com/publications/blending-in-detail/
	//Chose this method because expansion beyond two textures is trivial.
	vec3 blendedNormal = normalize(vec3(
		normalTexs[0].xy+normalTexs[1].xy+normalTexs[2].xy,
		normalTexs[0].z*normalTexs[1].z*normalTexs[2].z
	));

	//This LERP is awful, but this is a debug slider anyway.
	blendedNormal = mix(vec3(0.0,0.0,1.0),blendedNormal,waveBumpScale);

	mat3 tang_to_world = mat3(
		normalize(fs_in.tang_to_world[0]),
		normalize(fs_in.tang_to_world[1]),
		normalize(fs_in.tang_to_world[2])
	);

	vec3 normal = tang_to_world*blendedNormal;

	vec3 retCol = vec3(clamp(0.5+0.5*normal,0.0,1.0));
	
	frag_color = vec4(pow(retCol,vec3(1.0/2.2)),1.0);
}
