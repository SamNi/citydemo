// Dumb, orthographic, intended for 2D GUIs.
// Textured screen space quads, intended for the FBO blit.
#version 440 core
#pragma debug(on)

layout(location = 0) in vec2 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 texCoord;

out vec4 colorOut;
out vec2 texOut;

void main() {
	colorOut = color;
	texOut = texCoord;
    gl_Position = vec4(position, 0.0f, 1.0f);
}