#version 410

layout (location = 0) in vec3 vertex;

uniform mat4 vertex_world_to_clip;

out VS_OUT {
	vec3 vertex;
} vs_out;

void main()
{
	vec3 v = vec3(2.0*vertex.xy-1.0,0.999999);
	vs_out.vertex = vec3(inverse(vertex_world_to_clip)*vec4(v,1.0f));
	gl_Position = vec4(v,1.0);
}