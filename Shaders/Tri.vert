#version 450 core

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(set = 0, binding = 0) uniform Global_Camera {
	mat4 View;
	mat4 Proj;
	mat4 ViewProj;
} Camera;

layout(push_constant) uniform PushConst {
	mat4 Model;
} PC;

layout(location = 0) out vec3 outNormal;

void main() {
	outNormal = inNormal;
	gl_Position = Camera.ViewProj * PC.Model * vec4(inPosition, 1.0f);
}