#version 440 core
layout(location = 0) in vec2 vertexPos;
layout(location = 1) in vec3 vertexCol;

out vec3 color;

uniform mat4x4 transMat;

void main() {
	color = vertexCol;
	gl_Position = transMat*vec4(vertexPos, 0.0, 1.0);
    //gl_Position = vec4(vertexPos, 0, 1);
}