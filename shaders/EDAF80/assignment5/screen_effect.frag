#version 410

uniform float screen_tint_opacity;
uniform vec3 screen_tint_color;

in VS_OUT {
	vec3 vertex;
} fs_in;

out vec4 frag_color;

void main(){
	frag_color = vec4(screen_tint_color,screen_tint_opacity);
}
