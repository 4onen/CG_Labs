#version 410

layout (location = 0) in vec3 vertex;

uniform mat4 vertex_world_to_clip;
uniform float aspect;

out VS_OUT {
	vec2 uv;
} vs_out;

void main()
{
	vec3 v = vec3(2.0*vertex.xy-1.0,0.01);
	if(aspect>0.0){
    	vs_out.uv = vec2(v.x*aspect,v.y);
	}else{
		vs_out.uv = v
	}
	gl_Position = vec4(v,1.0);
}