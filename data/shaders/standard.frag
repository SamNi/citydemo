#version 440 core
#pragma debug(on)

in vec2 texOut;
in vec3 normalOut;
in vec4 colorOut;

out vec4 FragColor;

uniform sampler2D texMap;
uniform float seed;

void main() {
    vec4 vertColor = colorOut;
    vec4 texel = texture(texMap, texOut);

    //float N_dot_L = dot( normalize(normalOut), normalize(vec3(0.0f, 0.0f, 1.0f)));
    //float kIntensity = max(0.0f, N_dot_L);
    FragColor = texel*vertColor;
}