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




/// ignore everything below. this is the graveyard

// Uncomment for R-only grayscale textures
//FragColor = texture(texMap, texOut).rrra;
    
// Swizzle r,g,b
//FragColor = texture(texMap, texOut).bgra;

// Negative
//FragColor = vec4(1.) - texture(texMap, texOut);

// Conventional way (alpha blended)
// emulating glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//FragColor = vec4(1,0,0,1);




vec4 f();
vec4 g();

float backgr = 0.5;
float pi = 3.141591653589793238462643f;
float two_pi = 2*pi;

float overlay(float a, float b) {
	float ret;
	if (a < 0.5)
		ret = 2*a*b;
	else
		ret = 1 - 2*(1-a)*(1-b);
	return ret;
}

vec4 overlay_pix(vec4 a, vec4 b) {
	vec4 ret = vec4(overlay(a.x,b.x), overlay(a.y,b.y), overlay(a.z,b.z), 1);
	return ret;
}

vec4 graywipe() {
	return vec4(vec3(texOut.x), 1);
}

float rand() {
	return fract(sin(seed + dot(gl_FragCoord.xy, vec2(12.9898,78.233))) * 43758.5453);
}

float unif(float a, float b) {
	return a + (b-a)*rand();
}

vec4 f() {
	float t = sin(0.05*length(1280-gl_FragCoord));
	if (t > 0)
		return vec4(t,0,0,1);
	else
		return vec4(0,0,-t,1);
}

vec4 g() {
	float t = cos(0.05*length(gl_FragCoord));
	if (t > 0)
		return vec4(backgr,t,backgr,1);
	else
		return vec4(backgr,backgr,-t,1);
}
