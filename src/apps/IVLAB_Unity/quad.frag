#version 410 core

// Same name as in vert shader
in vec2 fragUV;

// Our texture
uniform sampler2D tex;

out vec4 color;

void main(){
  color = texture(tex, fragUV); /*vec4(1, 0, 0, 1);*/
}