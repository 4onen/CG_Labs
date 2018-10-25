#version 410

in VS_OUT {
	vec3 vertex;
	mat3 tbn;
} fs_in;

out vec4 frag_color;

void main(){
	vec3 n = normalize(fs_in.tbn[2]);

	frag_color = vec4(n*0.5+0.5,1.0);
}
