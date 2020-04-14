R"zzz(#version 330 core
uniform mat4 bone_transform; // transform the cylinder to the correct configuration
const float kPi = 3.1415926535897932384626433832795;
uniform mat4 projection;
uniform mat4 model;
uniform mat4 view;
in vec4 vertex_position;

// FIXME: Implement your vertex shader for cylinders
// Note: you need call sin/cos to transform the input mesh to a cylinder

void main() {
	float n = vertex_position[0] + .5;
	n = n * 2 * kPi;

	mat4 mvp = projection * view * model;
	gl_Position = mvp * bone_transform * vec4(sin(n) * .5, vertex_position[1], cos(n) * .5, vertex_position[3]);
}
)zzz"
