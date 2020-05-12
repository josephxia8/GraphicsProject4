R"zzz(
#version 330 core
uniform vec4 light_position;
uniform vec3 camera_position;
uniform vec4 tri_rot;
in vec4 vertex_position;
in vec4 normal;
in vec2 uv;
in vec3 color;
in vec4 face_pos;
in vec4 face_rot;
out vec4 vs_light_direction;
out vec4 vs_normal;
out vec2 vs_uv;
out vec4 vs_camera_direction;
out vec3 vs_color;

vec3 qtransform(vec4 q, vec3 v) {
	return v + 2.0 * cross(cross(v, q.xyz) - q.w*v, q.xyz);
}

void main() {
	vec3 new_pos = qtransform(tri_rot, vertex_position.xyz);
	new_pos = qtransform(face_rot, new_pos);
	vec4 new_pos_four = vec4(new_pos.x, new_pos.y, new_pos.z, 1);
	gl_Position = new_pos_four + face_pos;
	gl_Position.w = vertex_position.w;
	vs_light_direction = light_position - gl_Position;
	vs_camera_direction = vec4(camera_position, 1.0) - gl_Position;
	vs_normal = normal;
	vs_uv = uv;
	vs_color = color;
}
)zzz"
