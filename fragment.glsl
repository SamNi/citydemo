#version 430
#pragma debug(on)

in vec3 color;
out vec4 FragColor;

uniform sampler2D Diffuse;

void main() {
    //FragColor = texture(Diffuse, texOut)*vec4(color, 1);
    //FragColor = texture(Diffuse, texOut);
    FragColor = vec4(1,1,1,1);
}