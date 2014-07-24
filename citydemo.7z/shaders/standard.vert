// Standard general purpose shader, roughly emulates the classic
// fixed-function, gouraud shaded, texture mapped pipeline
#version 440 core
#pragma debug(on)

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec3 normal;

out vec4 colorOut;
out vec2 texOut;
out vec3 normalOut;
out vec3 lightOut;
out float kDiffuseOut;

uniform mat4x4 modelView;
uniform mat4x4 projection;
uniform vec3 lightPos;

void main() {
    texOut = texCoord;
	colorOut = color;
	normalOut = normal;

	kDiffuseOut = dot(normalize(normal), normalize(vec3(0,0,-1.0f)));

    gl_Position = projection*modelView*vec4(position,1.0f);
}