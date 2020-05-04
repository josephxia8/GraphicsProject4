R"zzz(
#version 330 core
uniform vec4 light_position;
uniform vec3 camera_position;

uniform vec3 joint_trans[128];
uniform vec4 joint_rot[128];
uniform mat4 deform_inv[128];
uniform int shader_num;
uniform float time_since_start;

in int jid0;
in int jid1;
in float w0;
in vec3 vector_from_joint0;
in vec3 vector_from_joint1;
in vec4 normal;
in vec2 uv;
in vec4 vert;

out vec4 vs_light_direction;
out vec4 vs_normal;
out vec2 vs_uv;
out vec4 vs_camera_direction;

vec3 qtransform(vec4 q, vec3 v) {
	return v + 2.0 * cross(cross(v, q.xyz) - q.w*v, q.xyz);
}

void main() {
	// FIXME: Implement linear skinning here
	float w1 = 1 - w0;
	vec3 newv_from_j0 = qtransform(joint_rot[jid0], vector_from_joint0);
	vec3 pos1 = joint_trans[jid0] + newv_from_j0;
	gl_Position = vec4(pos1.x * w0, pos1.y * w0, pos1.z *w0, 1);
	
	if (jid1 >= 0){
		vec3 newv_from_j1 = qtransform(joint_rot[jid1], vector_from_joint1);
		vec3 pos2 = joint_trans[jid1] + newv_from_j1;
		gl_Position += vec4(pos2.x * w1, pos2.y * w1, pos2.z * w1, 1);
	}
	
	gl_Position.w = 1;
	
	vs_normal = normal;
	vs_light_direction = light_position - gl_Position;
	vs_camera_direction = vec4(camera_position, 1.0) - gl_Position;
	vs_uv = uv;

	float factor = sin(time_since_start);
	factor = factor + 1;
	factor = factor / 2;
	float factor_inv = 1 - factor;

	// sphere deformation shader
	if (shader_num % 2 == 1){
		vec3 center = vec3(0,13, 0);
		vec3 pos = vec3(vert.x, vert.y, vert.z);
		vec3 dir = pos - center;
		dir = normalize(dir);
		dir = dir * 10;
		vec3 newPos = (center + dir);
		gl_Position = (factor_inv * gl_Position) + (factor * vec4(newPos.x, newPos.y, newPos.z, 1));
	}

	// cube deformation shader
	/*if ((shader_num % 512)/256 == 1){
		vec3 center = vec3(0,13, 0);
		vec3 pos = vec3(vert.x, vert.y, vert.z);
		vec3 dir = pos - center;
		float f = max(dir.x, dir.y);
		f = max(f, dir.z);
		f = 1/f; 
		dir = f * dir;
		dir = dir * 4;
		vec3 newPos = (center + dir);
		gl_Position = (factor_inv * gl_Position) + (factor * vec4(newPos.x, newPos.y, newPos.z, 1));
	}*/

	// wiggly shader
	if ((shader_num % 32)/16 != 0){
		gl_Position.y = gl_Position.y + sin(gl_Position.x + time_since_start * 5);
	}

	//miku miku bounce (wiggly in the z direction)
	if ((shader_num % 64)/32 != 0){
		gl_Position.y = gl_Position.y + sin(gl_Position.z + time_since_start * 5);
	}

	//"Walk cycle" shader
	if ((shader_num % 128)/64 != 0){
		gl_Position.y = gl_Position.y + sin(gl_Position.x) * sin(time_since_start * 3);
	}

}
)zzz"
