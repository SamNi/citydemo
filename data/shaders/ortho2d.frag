#version 440 core
#pragma debug(on)

in vec4 colorOut;
in vec2 texOut;

out vec4 FragColor;

uniform sampler2D texSampler;

void main() {
    FragColor = colorOut*texture(texSampler, texOut);
}