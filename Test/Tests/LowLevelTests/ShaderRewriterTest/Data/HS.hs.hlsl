




cbuffer cbR2 
{ 
float4 cbR2_sv0;
float4 cbR2_sv1;


}; 

cbuffer cbR3 
{ 
float4 cbR3_sv0;
float4 cbR3_sv1;


}; 

StructuredBuffer<float4> sbR1;

RWStructuredBuffer<float4> rwSBR2;

RWStructuredBuffer<float4> rwSBR3;


SamplerState samplerR0;

SamplerState samplerR2 : register ( s0 );

SamplerState samplerR3;

SamplerState samplerR4 : register ( s1 );


Texture2D<float4> textureR1 : register ( t4 );

Texture2D<float4> textureR2;

Texture2D<float4> textureR3;

Texture2D<float4> textureR4;

RWTexture2D<float4> rwTextureR0 : register ( u6 );

RWTexture2D<float4> rwTextureR1;

RWTexture2D<float4> rwTextureR3;


ByteAddressBuffer  rbR1;

ByteAddressBuffer  rbR2;

ByteAddressBuffer  rbR3 : register ( t2 );



struct HS_OUT 
{ 
}; 



struct VS_CONTROL_POINT_OUTPUT
{
    float3 vPosition : WORLDPOS;
    float2 vUV       : TEXCOORD0;
    float3 vTangent  : TANGENT;
};

struct HS_CONSTANT_DATA_OUTPUT
{
    float Edges[4]        : SV_TessFactor;
    float Inside[2]       : SV_InsideTessFactor;
    
    float3 vTangent[4]    : TANGENT;
    float2 vUV[4]         : TEXCOORD;
    float3 vTanUCorner[4] : TANUCORNER;
    float3 vTanVCorner[4] : TANVCORNER;
    float4 vCWts          : TANWEIGHTS;
};

#define MAX_POINTS 32

HS_CONSTANT_DATA_OUTPUT patchConstantFunction(InputPatch<VS_CONTROL_POINT_OUTPUT, MAX_POINTS> ip, uint PatchID : SV_PrimitiveID )
{	
    HS_CONSTANT_DATA_OUTPUT hscOut;
    hscOut.Edges[0] = 0.0;
    hscOut.Edges[1] = 0.0;
    hscOut.Edges[2] = 0.0;
    hscOut.Edges[3] = 0.0;

    hscOut.Inside[0] = 0.0;
    hscOut.Inside[1] = 0.0;
    
    
    hscOut.vTangent[0] = float3(0.0, 0.0, 0.0);
    hscOut.vTangent[1] = float3(0.0, 0.0, 0.0);
    hscOut.vTangent[2] = float3(0.0, 0.0, 0.0);
    hscOut.vTangent[3] = float3(0.0, 0.0, 0.0);
    hscOut.vUV[0] = float2(0.0, 0.0);
    hscOut.vUV[1] = float2(0.0, 0.0);
    hscOut.vUV[2] = float2(0.0, 0.0);
    hscOut.vUV[3] = float2(0.0, 0.0);
    hscOut.vTanUCorner[0] = float3(0.0, 0.0, 0.0);
    hscOut.vTanUCorner[1] = float3(0.0, 0.0, 0.0);
    hscOut.vTanUCorner[2] = float3(0.0, 0.0, 0.0);
    hscOut.vTanUCorner[3] = float3(0.0, 0.0, 0.0);

    hscOut.vTanVCorner[0] = float3(0.0, 0.0, 0.0);
    hscOut.vTanVCorner[1] = float3(0.0, 0.0, 0.0);
    hscOut.vTanVCorner[2] = float3(0.0, 0.0, 0.0);
    hscOut.vTanVCorner[3] = float3(0.0, 0.0, 0.0);
    hscOut.vCWts = float4(0.0, 0.0, 0.0, 0.0);
    
    return hscOut;
}

[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(16)]
[patchconstantfunc("patchConstantFunction")]
HS_OUT main(InputPatch<VS_CONTROL_POINT_OUTPUT, MAX_POINTS> ip, uint i : SV_OutputControlPointID, uint PatchID : SV_PrimitiveID )
{ 

HS_OUT hsOut; 



return hsOut; 
} 

