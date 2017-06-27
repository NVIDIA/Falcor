
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





cbuffer cbR0 
{ 
float4 cbR0_sv0;
float4 cbR0_sv1;
float4 cbR0_sv2;


}; 

cbuffer cbR1  : register ( b0 )
{ 
float4 cbR1_sv0;
float4 cbR1_sv1;


}; 

cbuffer cbR2 
{ 
float4 cbR2_sv0;
float4 cbR2_sv1;


}; 

StructuredBuffer<float4> sbR0 : register ( t0 );

RWStructuredBuffer<float4> rwSBR3;



Texture2D<float4> textureR0;

Texture2D<float4> textureR1;

Texture2D<float4> textureR3;

Texture2D<float4> textureR4 : register ( t6 );


ByteAddressBuffer  rbR0;

RWByteAddressBuffer  rwRBR0 : register ( u5 );



struct DS_OUT 
{ 
}; 


[domain("quad")] 
DS_OUT main( HS_CONSTANT_DATA_OUTPUT input,  float2 UV : SV_DomainLocation, const OutputPatch<DS_OUT, 16> bezpatch) 
{ 
 
DS_OUT dsOut; 




return dsOut; 

} 
 
