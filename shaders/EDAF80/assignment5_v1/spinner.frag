#version 410

uniform float nowTime;
uniform vec3 color;

in VS_OUT {
	vec2 uv;
} fs_in;

out vec4 frag_color;

void main(){
    vec2 po = vec2(sign(uv.y)*acos(uv.x/length(uv)),length(uv));

    vec3 retCol = vec3(po.x,po.y,uv.y);
	
	frag_color = vec4(pow(retCol,vec3(1.0/2.2)),1.0);
}
