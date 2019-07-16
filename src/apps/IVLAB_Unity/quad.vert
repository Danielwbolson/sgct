#version 410 core

in vec3 inPosition;
in vec2 inUV;

out vec2 fragUV;

void main() {
	gl_Position = vec4(inPosition, 1.0);

	fragUV = inUV;
}