#version 450 core

layout(location = 0) out vec3 vPosition;

void main() {
	const vec3 positions[3] = vec3[3](
		vec3(-1.0f, -1.0f, 0.0f),
		vec3(3.0f,  -1.0f, 0.0f),
		vec3(-1.0f,  3.0f, 0.0f)
	);

	vPosition = positions[gl_VertexIndex];
	gl_Position = vec4(positions[gl_VertexIndex], 1.0f);
}