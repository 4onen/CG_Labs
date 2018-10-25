#version 410

uniform vec3 camera_position;

uniform samplerCube cubemap_texture;

in VS_OUT {
	vec3 vertex;
	mat3 tbn;
	vec2 texcoord;
} fs_in;

out vec4 frag_color;

void main(){
	vec3 to_camera = normalize(camera_position - fs_in.vertex);
	vec3 n = normalize(fs_in.tbn[0]);

	vec3 reflect_dir = reflect(to_camera,n);
	
	vec3 retCol = texture(cubemap_texture,reflect_dir).xyz;
	//vec3 retCol = vec3(max(0.0,dot(n,vec3(1.0,0.0,0.0))));

	frag_color = vec4(pow(retCol,vec3(1.0/2.2)),1.0);
}
