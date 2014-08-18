#version 440 core
#pragma debug(on)

layout(location = 0) in vec3 position;
layout(location = 1) in vec4 color;
layout(location = 2) in vec2 texCoord;
layout(location = 3) in vec3 normal;

out vec4 colorOut;
out vec2 texOut;
out vec3 normalOut;

layout (binding = 0, std140) uniform per_instance_mvp {
    mat4x4 the_mvp[1024];
};

uniform bool gui_quad_instanced;
uniform mat4x4 modelView;
uniform mat4x4 projection;

void main() {
    texOut = texCoord;
    colorOut = color;
	normalOut = normal;
    if (gui_quad_instanced)
        gl_Position = the_mvp[gl_InstanceID]*vec4(position,1.0f);
    else
        gl_Position = projection*modelView*vec4(position,1.0f);
}