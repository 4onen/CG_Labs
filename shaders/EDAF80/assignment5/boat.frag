#version 410

uniform vec3 diffuse_color;

in VS_OUT {
	vec3 vertex;
	mat3 tbn;
	vec2 texcoord;
} fs_in;

out vec4 frag_color;

float rand(vec2 n) { 
	return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453);
}

float noise(vec2 p){
    vec2 f = fract(p);
    vec2 x = floor(p);
    const vec2 d = vec2(0.0,1.0);

    return mix(mix(rand(x),rand(x+d.yx),f.x),mix(rand(x+d.xy),rand(x+d.yy),f.x),f.y);
}

void main(){
    vec2 uv = fs_in.texcoord;
    uv *= 10.0;

    vec3 retCol = noise(uv)*diffuse_color*vec3(0.5,0.4,0.3);

	frag_color = vec4(pow(retCol,vec3(1.0/2.2)),1.0);
}
