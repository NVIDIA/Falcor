/***************************************************************************
Copyright (c) 2014, NVIDIA CORPORATION.  All rights reserved.
***************************************************************************/
#define _COMPILE_DEFAULT_VS
#include "VertexAttrib.h"
__import Shading;
#include "Effects/CsmData.h"

cbuffer PerFrameCB : register(b0)
{
	float3 gAmbient;
    CsmData gCsmData[_LIGHT_COUNT];
    bool visualizeCascades;
    float4x4 camVpAtLastCsmUpdate;
};

struct ShadowsVSOut
{
    VS_OUT vsData;
    float shadowsDepthC : DEPTH;
};

float4 main(ShadowsVSOut pIn) : SV_TARGET0
{
    ShadingAttribs shAttr;
    prepareShadingAttribs(gMaterial, pIn.vsData.posW, gCam.position, pIn.vsData.normalW, pIn.vsData.bitangentW, pIn.vsData.texC, 0, shAttr);
    ShadingOutput result;
    float4 fragColor = float4(0,0,0,1);
    
    [unroll]
    for(uint l = 0 ; l < _LIGHT_COUNT ; l++)
    {
        float shadowFactor = calcShadowFactor(gCsmData[l], pIn.shadowsDepthC, shAttr.P, pIn.vsData.posH.xy/pIn.vsData.posH.w);
        evalMaterial(shAttr, gLights[l], result, l == 0);
        fragColor.rgb += result.diffuseAlbedo * result.diffuseIllumination * shadowFactor;
        fragColor.rgb += result.specularAlbedo * result.specularIllumination * (0.01f + shadowFactor * 0.99f);
    }

    fragColor.rgb += gAmbient * result.diffuseAlbedo * 0.1;
    if(visualizeCascades)
    {
        //Ideally this would be light index so you can visualize the cascades of the 
        //currently selected light. However, because csmData contains Textures, it doesn't
        //like getting them with a non literal index.
        fragColor.rgb *= getCascadeColor(getCascadeIndex(gCsmData[_LIGHT_INDEX], pIn.shadowsDepthC));
    }

    return fragColor;
}
