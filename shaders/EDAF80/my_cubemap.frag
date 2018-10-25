#version 410

uniform vec3 camera_position;

uniform vec3 diffuse;

uniform samplerCube cubemap_texture;
uniform sampler2D bump_texture;
uniform bool has_diffuse_texture;

in VS_OUT {
	vec3 vertex;
	vec2 texcoord;
	mat3 tang_to_world;
} fs_in;

out vec4 frag_color;

void main(){
	vec3 normal;
	if(has_diffuse_texture){
		vec3 bump = texture(bump_texture,fs_in.texcoord.xy).xyz;
		mat3 tang_to_world = mat3( normalize(fs_in.tang_to_world[0])
								 , normalize(fs_in.tang_to_world[1])
								 , normalize(fs_in.tang_to_world[2])
								 );
		normal = normalize(tang_to_world*(2*bump-1));
	}else{
		normal = normalize(fs_in.tang_to_world[2]);
	}

	vec3 toCamera = normalize(camera_position - fs_in.vertex);
	vec3 retCol = diffuse*texture(cubemap_texture,reflect(-toCamera,normal)).rgb;
	frag_color = vec4(pow(retCol,vec3(1.0/2.2)),1.0);
}
