#version 430

in vec3 color;
in vec2 texOut;
out vec4 FragColor;

uniform sampler2D Diffuse;

void main() {
    int x = 1;
    //FragColor = vec4(vec3(1,1,1) - color, 1);
    //FragColor = vec4(color, 1);

    //vec2 t = texOut - vec2(0.5,0.5);
    //float x = sign(t.x*t.y);
    //FragColor = vec4(x,x,x,1);
    FragColor = texture(Diffuse, texOut);
}