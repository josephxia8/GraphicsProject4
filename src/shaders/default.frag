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
uniform float time_since_start;
out vec4 fragment_color;

float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}
void main() {
	vec3 texcolor = texture(textureSampler, uv_coords).xyz;
	float intensity = dot(vertex_normal, camera_direction);

	float is = 0.5f;
	float ih = 0.95f;
	
	if (length(texcolor) == 0.0) {
		vec3 color = vec3(diffuse);
		float dot_nl = dot(normalize(light_direction), normalize(vertex_normal));
		dot_nl = clamp(dot_nl, 0.0, 1.0);
		vec4 spec = specular * pow(max(0.0, dot(reflect(-light_direction, vertex_normal), camera_direction)), shininess);
		color = clamp(dot_nl * color + vec3(ambient) + vec3(spec), 0.0, 1.0);
		fragment_color = vec4(color, alpha);
	} else {
		fragment_color = vec4(texcolor.rgb, alpha);
	}

	// metal shader (failed toon shader)
	if ((shader_num % 16)/8 == 1){
		vec3 temp_color = vec3(fragment_color);
		//temp_color = temp_color * 0.1;
		
		if (intensity < is)
		{
			//fragment_color = shadowColor;
			temp_color = 0.35 * temp_color;
		} else if (intensity > ih) {
			//fragment_color = highlightColor;
			temp_color = temp_color;
		} else {
			temp_color = 0.7 * temp_color;
		}
		fragment_color = vec4(temp_color.x, temp_color.y, temp_color.z, fragment_color.w);
	}

	// toon shader 
	if ((shader_num % 256)/128 == 1){
		vec3 temp_color = vec3(diffuse);
		//temp_color = temp_color * 0.1;
		
		if (intensity < is)
		{
			//fragment_color = shadowColor;
			temp_color = 0.35 * temp_color;
		} else if (intensity > ih) {
			//fragment_color = highlightColor;
			temp_color = 0.85 * temp_color;
		} else {
			temp_color = 0.7 * temp_color;
		}
		fragment_color = vec4(temp_color.x, temp_color.y, temp_color.z, fragment_color.w);
	}

	// iradescent shader
	if ((shader_num % 1024)/512 != 0){
		float t_lambertian = dot(camera_direction, vertex_normal);
		vec4 a = vec4(0, 1, 0.6, alpha);
		vec4 b = vec4(0.96, 0.86, 0.3, alpha);
		vec4 c = vec4(0.84, 0.96, 0.3, alpha);
		vec4 d = vec4(0.3, 0.86, 0.96, alpha);
		fragment_color = a + b * cos(2 * 3.141592645 * (c * t_lambertian + d) + time_since_start * 3);
		fragment_color.w = alpha;
	}

	// flat shader
	if ((shader_num % 2048)/1024 != 0 || ((shader_num % 8192)/4096 != 0)){
		fragment_color = vec4(diffuse.x, diffuse.y, diffuse.z, fragment_color.w);
	}

	// black and white shader
	float color_mag = fragment_color.x + fragment_color.y + fragment_color.z;
	if ((shader_num % 4)/2 != 0){
		fragment_color = vec4(color_mag/3, color_mag/3, color_mag/3, alpha);
	}

	// outline shader
	if ((shader_num % 8)/4 != 0){
		if (dot(camera_direction, vertex_normal) < 0.2)
		{
			fragment_color = vec4(1, 1, 1, alpha);
		}
	}
}
)zzz"
