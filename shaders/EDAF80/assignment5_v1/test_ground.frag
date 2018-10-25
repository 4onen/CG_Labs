
#version 410

in VS_OUT {
	vec3 vertex;
	vec3 normal;
	vec3 diffuse_color;
} fs_in;

out vec4 frag_color;

void main(){
	vec3 normal = normalize(fs_in.normal);
    frag_color = vec4(fs_in.diffuse_color,1.0);//vec4(normal,1.0);//vec4(fs_in.diffuse_color*clamp(dot(vec3(1.0),normal),0.0,1.0),1.0);
}