R"zzz(
#version 330 core
in vec4 face_normal;
in vec4 vertex_normal;
in vec4 light_direction;
in vec4 camera_direction;
in vec2 uv_coords;
uniform vec4 diffuse;
uniform vec4 ambient;
uniform vec4 specular;
uniform float shininess;
uniform float alpha;
uniform sampler2D textureSampler;
uniform int shader_num;
out vec4 fragment_color;

float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}
void main() {
	vec3 texcolor = texture(textureSampler, uv_coords).xyz;
	float intensity = dot(vertex_normal, camera_direction);

	vec4 highlightColor = vec4(0.96, 0.86, 1, alpha);
	vec4 baseColor = vec4(0.84, 0.36, 1, alpha);
	vec4 shadowColor = vec4(0.36, 0.11, 0.44, alpha);

	float is = 0.5f;
	float ih = 0.8f;
	
	if (length(texcolor) == 0.0) {
		//vec3 color = vec3(0.0, 1.0, 0.0);
		//vec3 color = vec3(diffuse);
		vec3 color = vec3(diffuse);
		//vec2 randuv = vec2(rand(light_direction.xy), rand(light_direction.zw));
		//vec3 color = vec3(diffuse) + texture(textureSampler, randuv).xyz;
		//vec3 color = texture(textureSampler, randuv).xyz;
		//vec3 color = vec3(diffuse) + vec3(randuv.x, randuv.y, 1.0);
		float dot_nl = dot(normalize(light_direction), normalize(vertex_normal));
		dot_nl = clamp(dot_nl, 0.0, 1.0);
		vec4 spec = specular * pow(max(0.0, dot(reflect(-light_direction, vertex_normal), camera_direction)), shininess);
		color = clamp(dot_nl * color + vec3(ambient) + vec3(spec), 0.0, 1.0);
		fragment_color = vec4(color, alpha);
	} else {
		fragment_color = vec4(texcolor.rgb, alpha);
	}
	//fragment_color = vec4(0.2, 0.2, 0.2, 0.5);


	vec3 temp_color = normalize(vec3(fragment_color.x, fragment_color.y, fragment_color.z));
	temp_color = temp_color * 0.66;
	
	if (intensity < is)
	{
		//fragment_color = shadowColor;
		temp_color = 0.33 * normalize(temp_color);
	}
	else
	if (intensity > ih)
	{
		//fragment_color = highlightColor;
		temp_color = normalize(temp_color);
	}
	float color_mag = fragment_color.x + fragment_color.y + fragment_color.z;
	//fragment_color = vec4(color_mag/3, color_mag/3, color_mag/3, alpha);

	//fragment_color = vec4(min(temp_color.x, fragment_color.x), min(temp_color.y, fragment_color.y), min(temp_color.z, fragment_color.z), fragment_color.w);

	/*if (dot(camera_direction, vertex_normal) < 0.2)
	{
		fragment_color = vec4(1, 0, 1, alpha);
	}*/
}
)zzz"
