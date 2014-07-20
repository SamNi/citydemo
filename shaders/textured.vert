#version 440 core
#pragma debug(on)

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 texCoord;

out vec2 texOut;

uniform mat4x4 modelView;
uniform mat4x4 projection;

void main() {
    texOut = texCoord;

    gl_Position = projection*modelView*vec4(position,1.0);
}