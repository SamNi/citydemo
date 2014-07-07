#version 430

in vec3 color;
out vec4 FragColor;

void main() {
    //FragColor = vec4(vec3(1,1,1) - color, 1);
    FragColor = vec4(color, 1);
}