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

    if (gui_quad_instanced) {
        normalOut = vec3(0.0f, 0.0f, 1.0f);
        gl_Position = the_mvp[gl_InstanceID]*vec4(position,1.0f);
    }
    else {
    	normalOut = (modelView*vec4(normal, 0.0f)).xyz;
        gl_Position = projection*modelView*vec4(position,1.0f);
    }
}