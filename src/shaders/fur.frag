R"zzz(
#version 330 core
in vec4 face_normal;
in vec4 vertex_normal;
in vec4 light_direction;
in vec4 world_position;
in vec3 color;
out vec4 fragment_color;
void main() {
	vec4 pos = world_position;
	fragment_color = vec4(color.x, color.y, color.z, 1.0);
}
)zzz"
