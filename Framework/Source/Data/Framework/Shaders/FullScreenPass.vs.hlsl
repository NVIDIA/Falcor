#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 texCIn;

layout(location = 0) out vec2 texC;

void main()
{
    gl_Position = vec4(pos, 0.0, 1.0);
    texC = texCIn;
}