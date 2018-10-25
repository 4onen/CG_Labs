#version 410

uniform bool has_diffuse_texture;
uniform sampler2D bump_texture;

uniform mat4 vertex_model_to_world;

in VS_OUT {
	vec3 vertex;
	vec2 texcoord;
	mat3 tang_to_world;
} fs_in;

out vec4 frag_color;

void main(){
	vec3 normal;
	if(has_diffuse_texture){
		mat3 tang_to_world = mat3(normalize(fs_in.tang_to_world[0])
								 ,normalize(fs_in.tang_to_world[1])
								 ,normalize(fs_in.tang_to_world[2])
								 );
		normal = tang_to_world*(2*texture(bump_texture,fs_in.texcoord.xy).xyz-1);
	}else{
		normal = normalize(fs_in.tang_to_world[2]);
	}
	frag_color = vec4(pow(max(normal/2.0+0.5,0.0),vec3(1.0/2.2)),1.0);
}
