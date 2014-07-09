#version 440 core
#pragma debug(on)

layout(location = 0) in vec4 position;

uniform mat4x4 modelView;
uniform mat4x4 projection;

void main() {
	gl_Position = projection*modelView*position;
}