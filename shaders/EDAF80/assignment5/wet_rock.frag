#version 410

uniform vec3 camera_position;

uniform samplerCube cubemap_texture;

in VS_OUT {
	vec3 vertex;
	mat3 tbn;
	vec2 texcoord;
    float height;
} fs_in;

out vec4 frag_color;

float rand(vec2 n) { 
	return fract(sin(dot(n, vec2(12.9898, 4.1414))) * 43758.5453);
}

float noise(vec2 p){
    vec2 f = fract(p);
    vec2 x = floor(p);
    const vec2 d = vec2(0.0,1.0);

    return mix(mix(rand(x),rand(x+d.yx),f.x),mix(rand(x+d.xy),rand(x+d.yy),f.x),f.y);
}

void main(){
    vec2 uv = fs_in.texcoord;
    uv *= 32.0;
	vec3 n = normalize(fs_in.tbn[0]+0.8*vec3(0.0,noise(uv),noise(uv.yx)).yxz);


	vec3 to_camera = normalize(camera_position - fs_in.vertex);
	vec3 reflect_dir = reflect(to_camera,n);

    float wetness = max(0.0,0.8-smoothstep(0.0,2.0,fs_in.height));
	float underwater = step(0.0,-fs_in.height);
	
	vec3 rockCol = texture(cubemap_texture,reflect_dir,100.0*wetness).xyz+vec3(0.05);
	vec3 underwaterRockCol = vec3(0.1,0.1,0.2);
	vec3 retCol = mix(rockCol,underwaterRockCol,underwater);

	frag_color = vec4(pow(retCol,vec3(1.0/2.2)),1.0);
}
