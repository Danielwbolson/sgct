#version 330 core

vec3 inPosition;
vec2 inTex;

void main() {
	gl_Position.xyz = inPosition;
	gl_Position.w = 1.0;
}