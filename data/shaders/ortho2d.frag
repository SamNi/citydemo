#version 440 core
#pragma debug(on)

in vec4 colorOut;
in vec2 texOut;
in vec4 normalOut;

out vec4 FragColor;

uniform sampler2D texSampler;

vec4 grayscale(vec4 v) {
    float c = .2989*v.r + .5870*v.g + .1140*v.b;
    return vec4(c, c, c, 0.5f);
}

void main() {
    FragColor = colorOut*texture(texSampler, texOut);
}