#version 440 core
#pragma debug(on)

in vec4 color;
in vec2 texOut;
layout(location = 0, index = 0) out vec4 FragColor;

uniform sampler2D Diffuse;

void main() {
	FragColor = texture(Diffuse, texOut);
}