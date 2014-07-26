/*

in vec4 colorOut;
in vec2 texOut;
in vec3 normalOut;

out vec4 FragColor;

uniform sampler2D gSampler;

void main() {
	float kDiffuse = dot(normalize(normalOut), vec3(0.0f, 0.0f, -1.0f));
	//FragColor = kDiffuse*colorOut*texture(gSampler, texOut);
	//FragColor = kDiffuse*colorOut;
	//FragColor = vec4(texOut.x, texOut.y, 0.0f, 1);
	//FragColor = texture2D(gSampler, texOut);
	//FragColor = texOut.xxyy;
	//FragColor = kDiffuse*vec4(1,0,0,1);
    FragColor = texture2D(gSampler, texOut);
}*/
#version 440 core
#pragma debug(on)

in vec4 colorOut;
in vec2 texOut;
in vec3 normalOut;

out vec4 FragColor;

uniform sampler2D texSampler;

void main() {
    FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f) - texture(texSampler, texOut);
}
