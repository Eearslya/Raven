#version 450 core

layout(location = 0) in vec3 vPosition;

layout(location = 0) out vec4 outColor;

void main() {
	outColor = vec4((vPosition + vec3(1,1,1)) / 2, 1.0);
}