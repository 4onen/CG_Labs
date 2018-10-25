#version 410

#define PI 3.1415926535

uniform float t;

in VS_OUT {
	vec2 texcoord;
} fs_in;

in vec4 gl_FragCoord;

out vec4 frag_color;

float rand(vec2 p){
    return fract(sin(dot(p,vec2(12.9898,4.1414)))*43758.5453);
}

float noise(vec2 p){
    const vec2 d = vec2(1.,0.);
	vec2 i = floor(p);
    vec2 f = mix(d.yy,d.xx,fract(p)*fract(p));
	return mix(mix(rand(i),rand(i+d.xy),f.x),mix(rand(i+d.yx),rand(i+d.xx),f.x),f.y);
}

void main(){
    vec2 u = 2.*fs_in.texcoord-1.;
    u.x += PI*sign(u.y);
    float t = smoothstep(0.05,0.1,abs(u.y))*smoothstep(-0.6,-0.3,-abs(u.y));

    u *= 0.6*vec2(100.,40.);

    float f = noise(u);
    f = smoothstep(0.7,1.0,f)*smoothstep(0.3,1.0,t);

    if(f<0.7) discard;

    vec3 col = vec3(0.4,0.5,0.5);
    col *= 1.+smoothstep(-0.9,-0.4,-f);

    frag_color = vec4(sqrt(col),1.0);
}