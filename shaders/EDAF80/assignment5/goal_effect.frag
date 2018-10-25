#version 410

#define PI 3.14159265358979323846264338
#define TAU 6.2831853071795864769

uniform vec3 camera_position;
uniform float nowTime;

in VS_OUT {
	vec3 vertex;
	mat3 tbn;
	vec2 texcoord;
    vec3 model_vertex;
} fs_in;

out vec4 frag_color;

float col(in vec2 p, in float r){
    return length(p)-r;
}

float scene(in vec3 p){
    float centerCol = col(p.xz,0.1);
    vec2 po = vec2(length(p.xz),mod(atan(p.z,p.x),TAU*0.125));
    float borderCol = col(po-vec2(0.6,0.1),0.1);
    return min(min(centerCol,borderCol),p.y);
}

vec4 march(in vec3 o, in vec3 r){
    float t = 0.0;
    vec3 p = o;
    for(int i = 0; i<100; ++i){
        vec3 p = o+r*t;
        float d = scene(p);

        if(d<0.01) break;
        if(length(p) > 2.5) break;

        t += d;
    }
    return vec4(p,t);
}

void main(){
    const vec3 goalCol = vec3(0.1f,0.4f,1.0f);

    vec3 from_camera = normalize(fs_in.vertex - camera_position);
	vec4 march_result = march(fs_in.model_vertex,from_camera);

	vec3 retCol = goalCol*(1.0-march_result.w);
	
	frag_color = vec4(pow(retCol,vec3(1.0/2.2)),1.0);
}
