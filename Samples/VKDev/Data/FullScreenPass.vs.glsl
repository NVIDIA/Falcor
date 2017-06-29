#version 400

layout (location = 0) in vec4 pos;
layout (location = 1) in vec4 texC;

out gl_PerVertex
{
    vec4 gl_Position;
};

void main()
{
   gl_Position = pos;
}