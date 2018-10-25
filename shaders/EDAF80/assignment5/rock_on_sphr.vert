#version 410
#define PI 3.141592653589793
#define TAU 6.18318

layout (location = 0) in vec3 vertex;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 texcoord;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 binormal;

uniform float planet_radius;

uniform mat4[200] vertex_model_to_world;
uniform mat4 vertex_world_to_clip;

out VS_OUT {
	vec3 vertex;
	mat3 tbn;
	vec2 texcoord;
	float height;
} vs_out;

mat3 rotate_by_dir(float angle, vec2 direction){
	//cross(vec3(direction.x,0.0,direction.y),vec3(0.0,1.0,0.0))
	vec3 ax = vec3(-direction.y,0.0,direction.x);
	float c = cos(-angle),
		  s = sin(-angle);

	return mat3(vec3(ax.x*ax.x+(1-ax.x*ax.x)*c, ax.z*s,ax.x*ax.z*(1-c)),
				vec3(                  -ax.z*s,      c,         ax.x*s),
				vec3(          ax.x*ax.z*(1-c),-ax.x*s,ax.z*ax.z+(1-ax.z*ax.z)*c)
			   );
}

void main()
{
	vs_out.texcoord = texcoord.xy;
	mat3 normal_model_to_world = transpose(inverse(mat3(vertex_model_to_world[gl_InstanceID])));
	vs_out.tbn = normal_model_to_world * mat3(tangent,binormal,normal);

	vec3 v = vec3(vertex_model_to_world[gl_InstanceID]*vec4(vertex,1.0));

	vec2 dir = vec2(0.0);
	float y = 0.0;
	if(!(v.x==0.0 && v.z==0.0)){
		dir = normalize(v.xz);
		y = length(v.xz);
	}
	float x = v.y*v.y*v.y/3.0;

	float h = planet_radius*exp(x/planet_radius);
	float phi = min(PI,y / planet_radius);

	if(!(dir.x==0.0 && dir.y==0.0)){
		vs_out.tbn = rotate_by_dir(phi,dir) * vs_out.tbn;
	}


	vec2 proj2d = h*vec2(cos(phi),sin(phi));
	vec3 proj3d;
	proj3d.yxz = vec3(proj2d.x,proj2d.y*dir);
	vs_out.vertex = proj3d;
	vs_out.height = h-planet_radius;

	
	gl_Position = vertex_world_to_clip * vec4(proj3d,1.0);
}