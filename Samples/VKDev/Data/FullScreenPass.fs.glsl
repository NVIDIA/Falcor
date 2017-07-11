#version 450
#extension GL_ARB_separate_shader_objects : enable

//__import ShaderCommon;
//__import Shading;

layout(set = 1, binding = 1) uniform texture2D gTex;
layout(set = 4, binding = 2) uniform sampler gSampler;
layout(location = 0) in vec2 texC;
layout (location = 0) out vec4 outColor;

layout(set = 1, binding = 9, r32ui) uniform uimageBuffer typedBuffer;

layout(set = 1, binding = 4) buffer outImage
{
    uint count;
};

layout(set = 7, binding = 10) uniform PerFrameCB
{
    vec2 offset;
};

void main()
{
    outColor = texture(sampler2D(gTex, gSampler), texC + offset);
    atomicAdd(count, 1);
    imageAtomicAdd(typedBuffer, 0, 2);
}