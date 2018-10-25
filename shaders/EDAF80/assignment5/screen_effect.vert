#version 410

layout (location = 0) in vec3 vertex;

out VS_OUT {
	vec3 vertex;
} vs_out;

void main()
{
	vec3 v = vec3(2.0*vertex.xy-1.0,0.0);
	vs_out.vertex = v;
	gl_Position = vec4(v,1.0);
}