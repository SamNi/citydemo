#version 440 core
layout(location = 0) in vec2 vertexPos;
layout(location = 1) in vec3 vertexCol;
layout(location = 2) in vec2 texCoord;

out vec3 color;
out vec2 texOut;

uniform mat4x4 transMat;

void main() {
	color = vertexCol;
    texOut = texCoord;

	gl_Position = transMat*vec4(vertexPos, 0.0, 1.0);
    //gl_Position = vec4(vertexPos, 0, 1);
}