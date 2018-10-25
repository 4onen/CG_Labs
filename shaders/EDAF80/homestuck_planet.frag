#version 410

#define PI 3.1415926535

uniform mat4 colorSpectra;
uniform float blockify;
uniform float smoothing;

in VS_OUT {
	vec2 texcoord;
} fs_in;

out vec4 frag_color;

float rand(vec2 p){
    return fract(sin(dot(p,vec2(12.9898,4.1414)))*43758.5453);
}

float blockNoise(vec2 p){
    const vec2 d = vec2(1.,0.);
    vec2 i = floor(p);
    vec2 f = fract(p);
    return mix(mix(rand(i),rand(i+d.xy),f.x),mix(rand(i+d.yx),rand(i+d.xx),f.x),f.y);
}

float noise(vec2 p){
    const vec2 d = vec2(1.,0.);
	vec2 i = floor(p);
    vec2 f = smoothstep(d.yy,d.xx,fract(p));
	return mix(mix(rand(i),rand(i+d.xy),f.x),mix(rand(i+d.yx),rand(i+d.xx),f.x),f.y);
}

void main(){
    vec2 u = fs_in.texcoord;

    float polarFix = (0.5-u.y);
    polarFix *= polarFix;
    polarFix *= polarFix;
    
    u *= 40.;
    u.x /= 1.0+polarFix;

    float factor = mix(noise(u+noise(u)),blockNoise(u),blockify);

    vec3 col = colorSpectra[0].xyz;
    col = mix(
        mix(col,colorSpectra[1].xyz,step(colorSpectra[1][3],factor)),
        mix(col,colorSpectra[1].xyz,smoothstep(colorSpectra[0][3],colorSpectra[1][3],factor)),
        smoothing);
    col = mix(
        mix(col,colorSpectra[2].xyz,step(colorSpectra[2][3],factor)),
        mix(col,colorSpectra[2].xyz,smoothstep(colorSpectra[1][3],colorSpectra[2][3],factor)),
        smoothing);
    col = mix(
        mix(col,colorSpectra[3].xyz,step(colorSpectra[3][3],factor)),
        mix(col,colorSpectra[3].xyz,smoothstep(colorSpectra[2][3],colorSpectra[3][3],factor)),
        smoothing);
    
	frag_color = vec4(pow(col,vec3(1.0/2.2)),1.0);
}
