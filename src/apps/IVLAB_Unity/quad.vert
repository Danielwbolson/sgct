#version 410 core

in vec3 inPosition;
in vec2 inTexcoords;

out vec2 texCoordsFrag;

void main() {
	gl_Position = vec4(inPosition, 1.0);

	texCoordsFrag = inTexcoords;
}