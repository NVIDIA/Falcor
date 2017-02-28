/***************************************************************************
Copyright (c) 2014, NVIDIA CORPORATION.  All rights reserved.
***************************************************************************/
#version 450

#define _COMPILE_DEFAULT_VS
#include "VertexAttrib.h"
#include "shading.h"
#include "Effects/CsmData.h"

cbuffer PerFrameCB : register(b0)
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

struct ShadowsVSOut
{
    VS_OUT vsData;
    float shadowsDepthC : PSIZE;
};

vec4 main(ShadowsVSOut pIn) : SV_TARGET0
{
    float shadowFactor;
    ShadingAttribs shAttr;
    prepareShadingAttribs(gMaterial, pIn.vsData.posW, gCam.position, pIn.vsData.normalW, pIn.vsData.tangentW, pIn.vsData.bitangentW, pIn.vsData.texC, 0, shAttr);
    ShadingOutput result;
    float4 fragColor = vec4(0,0,0,1);
    
#foreach p in _LIGHT_SOURCES
    {
        shadowFactor = calcShadowFactor(gCsmData[$(_valIndex)], pIn.shadowsDepthC, shAttr.P, pIn.vsData.posH.xy);
        evalMaterial(shAttr, $(p), result, $(_valIndex) == 0);
        fragColor.rgb += result.diffuseAlbedo * result.diffuseIllumination * shadowFactor;
        fragColor.rgb += result.specularAlbedo * result.specularIllumination * (0.01f + shadowFactor * 0.99f);
    }
#endforeach
    fragColor.rgb += gAmbient * result.diffuseAlbedo * 0.1;
    if(visualizeCascades)
    {
        //This was light index, but cascade color only uses cascade count and cascade 
        //start count, which should be uniform across csmdatas 
        //using light index complained sampler array index must be literal
        fragColor.rgb *= getCascadeColor(getCascadeIndex(gCsmData[0], pIn.shadowsDepthC));
    }

    return fragColor;
}
