#version 410

uniform bool has_cubemap_texture;
uniform samplerCube cubemap_texture;

in VS_OUT {
	vec3 vertex;
} fs_in;

out vec4 frag_color;

void main(){
	vec3 retCol = vec3(0.0,0.0,1.0);
	if(has_cubemap_texture){
		vec3 sampleDir = normalize(fs_in.vertex);
		retCol = texture(cubemap_texture,sampleDir).xyz;
	}
	frag_color = vec4(pow(retCol,vec3(1.0/2.2)),1.0);
}
