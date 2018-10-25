#version 410

uniform vec3 light_position;
uniform vec3 camera_position;
uniform vec3 ambient;
uniform vec3 diffuse;
uniform vec3 specular;
uniform float shininess;

in VS_OUT {
	vec3 vertex;
	vec3 normal;
} fs_in;

out vec4 frag_color;

void main()
{
	vec3 normal = normalize(fs_in.normal);
	vec3 toLight = normalize(light_position - fs_in.vertex);
	vec3 toCamera = normalize(camera_position - fs_in.vertex);
	vec3 diffuseLighting = diffuse * clamp(dot(normal, toLight),0.0,1.0);
	vec3 specularLighting = specular * pow(clamp(dot(reflect(-toLight,normal),toCamera),0.0,1.0),shininess);
	frag_color = vec4(pow(ambient+diffuseLighting+specularLighting,vec3(1.0)),1.0);
}
