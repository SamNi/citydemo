#version 440 core
#pragma debug(on)

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 vertexCol;
layout(location = 2) in vec2 texCoord;

out vec3 color;
out vec2 texOut;

uniform mat4x4 modelView;
uniform mat4x4 projection;

void main() {
	color = vertexCol;
    texOut = texCoord;

	gl_Position = projection*modelView*position;
    //gl_Position = vec4(vertexPos, 0, 1);
}