#version 410 core

in vec3 inPosition;
in vec2 inUV;
uniform mat4 pvm;

out vec2 fragUV;


void main() {
	gl_Position = pvm * vec4(inPosition, 1.0);

	fragUV = inUV;
}