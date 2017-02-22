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
        shadowFactor = calcShadowFactor(gCsmData[0], pIn.shadowsDepthC, shAttr.P);
        evalMaterial(shAttr, $(p), result, $(_valIndex) == 0);
        fragColor.rgb += result.diffuseAlbedo * result.diffuseIllumination * shadowFactor;
        fragColor.rgb += result.specularAlbedo * result.specularIllumination * (0.01f + shadowFactor * 0.99f);
    }
#endforeach
    fragColor.rgb += gAmbient * result.diffuseAlbedo * 0.1;
    if(visualizeCascades)
    {
        //TODO, this should be using lightindex, not 0, but that gives sampler array index must be a literal expression
        //bc csmdata encapsulates 2 sampler arrays
        //fragColor.rgb *= getCascadeColor(getCascadeIndex(gCsmData[0], pIn.shadowsDepthC));
        fragColor.rgb *= getCascadeColor(0);
    }

    return fragColor;
}
