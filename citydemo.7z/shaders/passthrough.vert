#version 440 core
#pragma debug(on)

layout (location = 0) in vec2 Position;

void main() {
	gl_Position = vec4(Position, 0.0f, 1.0f);
}