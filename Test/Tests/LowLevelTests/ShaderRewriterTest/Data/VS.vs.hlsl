

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

cbuffer cbR3 
{ 
float4 cbR3_sv0;
float4 cbR3_sv1;


}; 

StructuredBuffer<float4> sbR1;

RWStructuredBuffer<float4> rwSBR1;

RWStructuredBuffer<float4> rwSBR2 : register ( u3 );

RWStructuredBuffer<float4> rwSBR4;


SamplerState samplerR0 : register ( s0 );

SamplerState samplerR1 : register ( s1 );

SamplerState samplerR3 : register ( s3 );


Texture2D<float4> erTex : register ( t0 );

Texture2D<float4> textureR2 : register ( t5 );

Texture2D<float4> textureR3;

Texture2D<float4> textureR4;

Texture2D<float4> textureR5;

RWTexture2D<float4> rwTextureR0 : register ( u5 );

RWTexture2D<float4> rwTextureR2;

RWTexture2D<float4> rwTextureR3;


ByteAddressBuffer  rbR0;

ByteAddressBuffer  rbR1;

ByteAddressBuffer  rbR2;

RWByteAddressBuffer  rwRBR0;

RWByteAddressBuffer  rwRBR1;


VS_OUT main(float4 pos : POSITION) 
{ 

VS_OUT vsOut; 

vsOut.output0 = cbR0_sv0 + cbR3_sv1 + erTex.Load(int3(0, 0, 0)) + textureR2.Load(int3(0, 0, 0)) + textureR3.Load(int3(0, 0, 0)) + textureR4.Load(int3(0, 0, 0)) + textureR5.Load(int3(0, 0, 0)) + rwTextureR0.Load(0) + rwTextureR2.Load(0) + rwTextureR3.Load(0) + sbR1[0] + rwSBR1[0] + rwSBR4[0] + rbR0.Load4(0) + rbR2.Load4(0) + float4(0.05, 0.05, 0.05, 0.05);
vsOut.output1 = cbR0_sv0 + cbR3_sv1 + erTex.Load(int3(0, 0, 0)) + textureR2.Load(int3(0, 0, 0)) + textureR3.Load(int3(0, 0, 0)) + textureR4.Load(int3(0, 0, 0)) + textureR5.Load(int3(0, 0, 0)) + rwTextureR0.Load(0) + rwTextureR2.Load(0) + rwTextureR3.Load(0) + sbR1[0] + rwSBR1[0] + rwSBR4[0] + rbR0.Load4(0) + rbR2.Load4(0) + float4(0.05, 0.05, 0.05, 0.05);
vsOut.output2 = cbR0_sv0 + cbR3_sv1 + erTex.Load(int3(0, 0, 0)) + textureR2.Load(int3(0, 0, 0)) + textureR3.Load(int3(0, 0, 0)) + textureR4.Load(int3(0, 0, 0)) + textureR5.Load(int3(0, 0, 0)) + rwTextureR0.Load(0) + rwTextureR2.Load(0) + rwTextureR3.Load(0) + sbR1[0] + rwSBR1[0] + rwSBR4[0] + rbR0.Load4(0) + rbR2.Load4(0) + float4(0.05, 0.05, 0.05, 0.05);
vsOut.output3 = cbR0_sv0 + cbR3_sv1 + erTex.Load(int3(0, 0, 0)) + textureR2.Load(int3(0, 0, 0)) + textureR3.Load(int3(0, 0, 0)) + textureR4.Load(int3(0, 0, 0)) + textureR5.Load(int3(0, 0, 0)) + rwTextureR0.Load(0) + rwTextureR2.Load(0) + rwTextureR3.Load(0) + sbR1[0] + rwSBR1[0] + rwSBR4[0] + rbR0.Load4(0) + rbR2.Load4(0) + float4(0.05, 0.05, 0.05, 0.05);
vsOut.output4 = cbR0_sv0 + cbR3_sv1 + erTex.Load(int3(0, 0, 0)) + textureR2.Load(int3(0, 0, 0)) + textureR3.Load(int3(0, 0, 0)) + textureR4.Load(int3(0, 0, 0)) + textureR5.Load(int3(0, 0, 0)) + rwTextureR0.Load(0) + rwTextureR2.Load(0) + rwTextureR3.Load(0) + sbR1[0] + rwSBR1[0] + rwSBR4[0] + rbR0.Load4(0) + rbR2.Load4(0) + float4(0.05, 0.05, 0.05, 0.05);
vsOut.output5 = cbR0_sv0 + cbR3_sv1 + erTex.Load(int3(0, 0, 0)) + textureR2.Load(int3(0, 0, 0)) + textureR3.Load(int3(0, 0, 0)) + textureR4.Load(int3(0, 0, 0)) + textureR5.Load(int3(0, 0, 0)) + rwTextureR0.Load(0) + rwTextureR2.Load(0) + rwTextureR3.Load(0) + sbR1[0] + rwSBR1[0] + rwSBR4[0] + rbR0.Load4(0) + rbR2.Load4(0) + float4(0.05, 0.05, 0.05, 0.05);


return vsOut; 
} 
