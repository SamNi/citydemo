#version 440 core
#pragma debug(on)

in vec2 texOut;
in vec3 normalOut;
in vec4 colorOut;
out vec4 FragColor;

uniform sampler2D texMap;
uniform float seed;

void main() {
	float kDiffuse = dot(normalize(normalOut), normalize(vec3(0.0f, 0.0f, -1.0f)));

	if (kDiffuse > 0)
		FragColor = kDiffuse*colorOut*texture(texMap, texOut);
	else
		FragColor = vec4(0,0,0,1.0f);
}