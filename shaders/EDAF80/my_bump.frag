#version 410

uniform vec3 light_position;
uniform vec3 camera_position;
uniform vec3 ambient;
uniform vec3 diffuse;
uniform vec3 specular;
uniform float shininess;

uniform bool has_diffuse_texture;
uniform sampler2D diffuse_texture;
uniform sampler2D bump_texture;

in VS_OUT {
	vec3 vertex;
	vec2 texcoord;
	mat3 tang_to_world;
} fs_in;

out vec4 frag_color;

void main(){
	vec3 diffuseCol;
	vec3 normal;
	if(has_diffuse_texture){
		diffuseCol = diffuse*texture(diffuse_texture,fs_in.texcoord.xy).xyz;
		mat3 tang_to_world = mat3(normalize(fs_in.tang_to_world[0])
								 ,normalize(fs_in.tang_to_world[1])
								 ,normalize(fs_in.tang_to_world[2])
								 );
		normal = normalize(tang_to_world*(2*texture(bump_texture,fs_in.texcoord.xy).xyz-1));
	}else{
		diffuseCol = diffuse;
		normal = normalize(fs_in.tang_to_world[2]);
	}
	vec3 toLight = normalize(light_position - fs_in.vertex);
	vec3 toCamera = normalize(camera_position - fs_in.vertex);
	vec3 diffuseLighting = diffuseCol * max(0.0,dot(normal, toLight));
	vec3 specularLighting = specular * clamp(pow(max(0.0,dot(reflect(-toLight,normal),toCamera)),shininess),0.0,1.0);
	frag_color = vec4(pow(ambient+diffuseLighting+specularLighting,vec3(1.0/2.2)),1.0);
}
