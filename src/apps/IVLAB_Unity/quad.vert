#version 410 core

in vec3 inPosition;
in vec2 inTexcoords;

void main() {
	gl_Position.xyz = inPosition;
	gl_Position.w = 1.0;
}