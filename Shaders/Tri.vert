#version 450 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(push_constant) uniform PushConst {
	mat4 ViewProjection;
	mat4 Model;
} PC;

layout(location = 0) out vec3 outNormal;

void main() {
	outNormal = inNormal;
	gl_Position = PC.ViewProjection * PC.Model * vec4(inPosition, 1.0f);
}