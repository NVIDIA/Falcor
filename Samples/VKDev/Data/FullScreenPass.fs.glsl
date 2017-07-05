#version 450
#extension GL_ARB_separate_shader_objects : enable


layout(set = 0, binding = 1) uniform texture2D gTex;
layout(set = 0, binding = 2) uniform sampler gSampler;
layout(location = 0) in vec2 texC;
layout (location = 0) out vec4 outColor;

void main()
{
    outColor = texture(sampler2D(gTex, gSampler), texC);
}