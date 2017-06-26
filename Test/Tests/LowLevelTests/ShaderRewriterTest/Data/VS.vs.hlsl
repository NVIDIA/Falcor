

struct VS_OUT 
{ 
float4 output0 : OUTPUT0; 
float4 output1 : OUTPUT1; 
float4 output2 : OUTPUT2; 
float4 output3 : OUTPUT3; 
float4 output4 : OUTPUT4; 
float4 output5 : OUTPUT5; 
}; 







cbuffer cbR0 
{ 
float4 cbR0_sv0;
float4 cbR0_sv1;
float4 cbR0_sv2;


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

RWStructuredBuffer<float4> rwSBR0 : register ( u2 );

RWStructuredBuffer<float4> rwSBR3;

RWStructuredBuffer<float4> rwSBR4 : register ( u4 );


SamplerState samplerR0;

SamplerState samplerR2;

SamplerState samplerR4;


Texture2D<float4> textureR0 : register ( t3 );

Texture2D<float4> textureR3;

Texture2D<float4> textureR5;

RWTexture2D<float4> rwTextureR2 : register ( u8 );

RWTexture2D<float4> rwTextureR3;


ByteAddressBuffer  rbR0;

ByteAddressBuffer  rbR2;


VS_OUT main(float4 pos : POSITION) 
{ 

VS_OUT vsOut; 

vsOut.output0 = cbR0_sv0 + cbR2_sv0 + cbR3_sv1 + textureR0.SampleLevel(samplerR0, float2(0,0), 0) + textureR3.Load(int3(0, 0, 0)) + textureR5.SampleLevel(samplerR0, float2(0,0), 0) + rwTextureR2.Load(0) + rwTextureR3.Load(0) + rwSBR3[0] + rbR0.Load4(0) + rbR2.Load4(0) + float4(0.05, 0.05, 0.05, 0.05);
vsOut.output1 = cbR0_sv0 + cbR2_sv0 + cbR3_sv1 + textureR0.SampleLevel(samplerR0, float2(0,0), 0) + textureR3.Load(int3(0, 0, 0)) + textureR5.SampleLevel(samplerR0, float2(0,0), 0) + rwTextureR2.Load(0) + rwTextureR3.Load(0) + rwSBR3[0] + rbR0.Load4(0) + rbR2.Load4(0) + float4(0.05, 0.05, 0.05, 0.05);
vsOut.output2 = cbR0_sv0 + cbR2_sv0 + cbR3_sv1 + textureR0.SampleLevel(samplerR0, float2(0,0), 0) + textureR3.Load(int3(0, 0, 0)) + textureR5.SampleLevel(samplerR0, float2(0,0), 0) + rwTextureR2.Load(0) + rwTextureR3.Load(0) + rwSBR3[0] + rbR0.Load4(0) + rbR2.Load4(0) + float4(0.05, 0.05, 0.05, 0.05);
vsOut.output3 = cbR0_sv0 + cbR2_sv0 + cbR3_sv1 + textureR0.SampleLevel(samplerR0, float2(0,0), 0) + textureR3.Load(int3(0, 0, 0)) + textureR5.SampleLevel(samplerR0, float2(0,0), 0) + rwTextureR2.Load(0) + rwTextureR3.Load(0) + rwSBR3[0] + rbR0.Load4(0) + rbR2.Load4(0) + float4(0.05, 0.05, 0.05, 0.05);
vsOut.output4 = cbR0_sv0 + cbR2_sv0 + cbR3_sv1 + textureR0.SampleLevel(samplerR0, float2(0,0), 0) + textureR3.Load(int3(0, 0, 0)) + textureR5.SampleLevel(samplerR0, float2(0,0), 0) + rwTextureR2.Load(0) + rwTextureR3.Load(0) + rwSBR3[0] + rbR0.Load4(0) + rbR2.Load4(0) + float4(0.05, 0.05, 0.05, 0.05);
vsOut.output5 = cbR0_sv0 + cbR2_sv0 + cbR3_sv1 + textureR0.SampleLevel(samplerR0, float2(0,0), 0) + textureR3.Load(int3(0, 0, 0)) + textureR5.SampleLevel(samplerR0, float2(0,0), 0) + rwTextureR2.Load(0) + rwTextureR3.Load(0) + rwSBR3[0] + rbR0.Load4(0) + rbR2.Load4(0) + float4(0.05, 0.05, 0.05, 0.05);


return vsOut; 
} 
