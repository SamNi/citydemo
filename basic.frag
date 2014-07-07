#version 430

in vec3 color;
out vec4 FragColor;

void main() {
	vec3 white = vec3(1,1,1);
    FragColor = vec4(white - color, 1);
}