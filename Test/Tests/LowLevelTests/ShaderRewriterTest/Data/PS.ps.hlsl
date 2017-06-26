

struct VS_OUT 
{ 
float4 input0 : OUTPUT0; 
float4 input1 : OUTPUT1; 
}; 


struct PS_OUT 
{ 
float4 output0 : SV_TARGET0; 
float4 output1 : SV_TARGET1; 
}; 






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

RWStructuredBuffer<float4> rwSBR1;

RWStructuredBuffer<float4> rwSBR2 : register ( u3 );

RWStructuredBuffer<float4> rwSBR3;

RWStructuredBuffer<float4> rwSBR4;


SamplerState samplerR0;

SamplerState samplerR1;

SamplerState samplerR4;


Texture2D<float4> textureR0;

Texture2D<float4> textureR1;

Texture2D<float4> textureR2;

RWTexture2D<float4> rwTextureR1 : register ( u7 );

RWTexture2D<float4> rwTextureR3;


ByteAddressBuffer  rbR0;

RWByteAddressBuffer  rwRBR1;


PS_OUT main(VS_OUT vOut) 
{ 

PS_OUT psOut; 

psOut.output0 = cbR2_sv0 + cbR3_sv1 + textureR0.SampleLevel(samplerR4, float2(0,0), 0) + textureR1.SampleLevel(samplerR1, float2(0,0), 0) + textureR2.SampleLevel(samplerR1, float2(0,0), 0) + rwTextureR1.Load(0) + rwTextureR3.Load(0) + rwSBR2[0] + rwSBR3[0] + rbR0.Load4(0) + vOut.input0 + vOut.input1 + float4(0.05, 0.05, 0.05, 0.05);
psOut.output1 = cbR2_sv0 + cbR3_sv1 + textureR0.SampleLevel(samplerR4, float2(0,0), 0) + textureR1.SampleLevel(samplerR1, float2(0,0), 0) + textureR2.SampleLevel(samplerR1, float2(0,0), 0) + rwTextureR1.Load(0) + rwTextureR3.Load(0) + rwSBR2[0] + rwSBR3[0] + rbR0.Load4(0) + vOut.input0 + vOut.input1 + float4(0.05, 0.05, 0.05, 0.05);


return psOut; 
} 
