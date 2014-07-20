#version 440 core
#pragma debug(on)

in vec2 texOut;
out vec4 FragColor;

uniform sampler2D Diffuse;
uniform float seed;

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

void main() {
    // Uncomment for R-only grayscale textures
    //FragColor = texture(Diffuse, texOut).rrra;
    
	// Swizzle r,g,b
	//FragColor = texture(Diffuse, texOut).bgra;

	// Negative
	//FragColor = vec4(1.) - texture(Diffuse, texOut);

	// Conventional way
    FragColor = texture(Diffuse, texOut);
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
