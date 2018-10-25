#version 410

uniform vec3 camera_position;

in VS_OUT {
	vec3 vertex;
	mat3 tbn;
	vec2 texcoord;
} fs_in;

out vec4 frag_color;

void main(){
    vec3 normal = normalize(fs_in.tbn[2]);

	vec3 toCamera = normalize(camera_position - fs_in.vertex);

	float facing = 1.0-max(dot(toCamera,normal),0.0);

	frag_color = vec4(pow(facing,1.0/2.2));
}
