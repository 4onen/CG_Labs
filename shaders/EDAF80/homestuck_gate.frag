#version 410

#define PI 3.1415926535

uniform float t;

in VS_OUT {
	vec2 texcoord;
} fs_in;

out vec4 frag_color;

void main(){
    //Convert texture coordinates to [-1,1] by [-1,1] coords
    vec2 u = 2.*fs_in.texcoord-1.;

    //Gate color
    const vec3 col = vec3(0.,0.8,0.9);

    //Convert [-1,1] by [-1,1] coords to polar coordinates
    vec2 ro = vec2(length(u),-acos(dot(u,vec2(0.,1.))/length(u)));
    //Scale down a touch, to avoid rasterizer clipping
    ro.x /= 0.95;

    //Frequency multiplier for center-touches
    const float a = 1.3;
    //Epicycle ratio
    float k = 11.+mod(t/10.,PI);

    //Center-touch managment function
    float ak = 1.0+sin(2*PI*a*k);

    //Function return value
    float ret = 0.;
    for(int i=0;i<12;i++){
        //Increment the angle by 2pi
        ro.y += PI*float(2*i);
        //Update return value with new epicycle drawing pass
        ret = max(ret,smoothstep(-0.05,0.,-abs(
            ro.x - (ak+abs(sin(10.*ro.y/k)))/(ak+1.0)
        )));
    }
    if(ret<0.6) discard;
    frag_color = vec4(0.,0.8,0.9,1.0);
}
