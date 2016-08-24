/***************************************************************************
Copyright (c) 2014, NVIDIA CORPORATION.  All rights reserved.
***************************************************************************/
#version 450

#include "ShaderCommon.h"
#include "shading.h"
#include "Effects/CsmData.h"

layout(binding = 0) uniform PerFrameCB
{
#foreach p in _LIGHT_SOURCES
    LightData $(p);
#endforeach

	vec3 gAmbient;
    CsmData gCsmData[_LIGHT_COUNT];
    bool visualizeCascades;
    int lightIndex;
    mat4 camVpAtLastCsmUpdate;
};

in vec3 normalW;
in vec3 posW;
in vec3 tangentW;
in vec3 bitangentW;
in vec2 texC;
in float ShadowsDepthC;
out vec4 fragColor;

void main()
{
    float shadowFactor;
    ShadingAttribs shAttr;
    prepareShadingAttribs(gMaterial, posW, gCam.position, normalW, tangentW, bitangentW, texC, 0, shAttr);
    ShadingOutput result;
    fragColor = vec4(0,0,0,1);
#foreach p in _LIGHT_SOURCES
    {
        evalMaterial(shAttr, $(p), result, $(_valIndex) == 0);
    	shadowFactor = calcShadowFactor(gCsmData[$(_valIndex)], ShadowsDepthC, shAttr.P);
        fragColor.rgb += result.diffuseAlbedo * result.diffuseIllumination * shadowFactor;
        fragColor.rgb += result.specularAlbedo * result.specularIllumination * (0.01f + shadowFactor * 0.99f);
    }
#endforeach
    fragColor.rgb += gAmbient * result.diffuseAlbedo * 0.1;
    if(visualizeCascades)
    {
        fragColor.rgb *= getCascadeColor(getCascadeIndex(gCsmData[lightIndex], ShadowsDepthC));
    }
}
