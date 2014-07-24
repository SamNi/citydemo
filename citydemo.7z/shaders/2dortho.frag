// Dumb, orthographic, intended for 2D GUIs.
// Textured screen space quads, intended for the FBO blit.
#version 440 core
#pragma debug(on)

in vec4 colorOut;
in vec2 texOut;
out vec4 FragColor;

uniform sampler2D texMap;

void main() {
	FragColor = colorOut*texture(texMap, texOut);
 }