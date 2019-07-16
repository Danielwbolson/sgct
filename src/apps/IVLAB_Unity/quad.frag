#version 410 core

// Same name as in vert shader
in vec2 texCoordsFrag;

// Our texture
uniform sampler2D tex;

out vec4 color;

void main(){
  color = texture(tex, texCoordsFrag);
}