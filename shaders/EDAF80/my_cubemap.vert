#version 410

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoord;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 binormal;

uniform mat4 vertex_model_to_world;
uniform mat4 normal_model_to_world;
uniform mat4 vertex_world_to_clip;

out VS_OUT {
	vec3 vertex;
	vec2 texcoord;
	mat3 tang_to_world;
} vs_out;


void main()
{
	vs_out.vertex = vec3(vertex_model_to_world * vec4(vertex, 1.0));
	vs_out.texcoord = texcoord.xy;
	vs_out.tang_to_world = mat3(normal_model_to_world) * mat3(tangent,binormal,normal);

	gl_Position = vertex_world_to_clip * vec4(vs_out.vertex, 1.0);
}



