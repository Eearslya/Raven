#version 450 core

layout(location = 0) in vec3 inNormal;

layout(location = 0) out vec4 outColor;

void main() {
	outColor = vec4((inNormal + vec3(1, 1, 1)) * 0.5, 1.0);
}