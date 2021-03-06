#version 440 core
#pragma debug(on)

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec4 normal;

out vec4 colorOut;
out vec2 texOut;
out vec4 normalOut;

void main() {
    texOut = texCoord;
    colorOut = color;
    normalOut = normal;
    
    gl_Position = vec4(position,1.0f);
}