#version 440 core
#pragma debug(on)

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 vertexCol;

out vec3 color;

uniform mat4x4 modelView;
uniform mat4x4 projection;

void main() {
	color = vertexCol;

	gl_Position = projection*modelView*position;
}