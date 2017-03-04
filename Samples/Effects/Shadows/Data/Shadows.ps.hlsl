/***************************************************************************
Copyright (c) 2014, NVIDIA CORPORATION.  All rights reserved.
***************************************************************************/
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
    mat4 camVpAtLastCsmUpdate;
};

struct ShadowsVSOut
{
    VS_OUT vsData;
    float shadowsDepthC : DEPTH;
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
        shadowFactor = calcShadowFactor(gCsmData[$(_valIndex)], pIn.shadowsDepthC, shAttr.P, pIn.vsData.posH.xy/pIn.vsData.posH.w);
        evalMaterial(shAttr, $(p), result, $(_valIndex) == 0);
        fragColor.rgb += result.diffuseAlbedo * result.diffuseIllumination * shadowFactor;
        fragColor.rgb += result.specularAlbedo * result.specularIllumination * (0.01f + shadowFactor * 0.99f);
    }
#endforeach
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
