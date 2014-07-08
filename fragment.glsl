#version 430

in vec3 color;
in vec2 texOut;
out vec4 FragColor;

uniform sampler2D Diffuse;

void main() {
    //FragColor = texture(Diffuse, texOut)*vec4(color, 1);
    FragColor = texture(Diffuse, texOut);
}