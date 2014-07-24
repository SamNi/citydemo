// Standard general purpose shader, roughly emulates the classic
// fixed-function, gouraud shaded, texture mapped pipeline
#version 440 core
#pragma debug(on)

in vec2 texOut;
in vec3 normalOut;
in vec4 colorOut;
in float kDiffuseOut;
out vec4 FragColor;

uniform sampler2D texMap;
uniform float seed;

void main() {
	//float kDiffuse = dot(normalize(normalOut), vec3(0,0,-1));

	if (kDiffuseOut > 0)
		FragColor = kDiffuseOut*colorOut*texture(texMap, texOut);
	else
		FragColor = vec4(0,0,0,1.0f);
}