

struct PS_IN 
{ 
float4 input0 : OUTPUT0; 
float4 input1 : OUTPUT1; 
}; 


struct PS_OUT 
{ 
float4 output0 : SV_TARGET0; 
float4 output1 : SV_TARGET1; 
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

cbuffer cbR3 
{ 
float4 cbR3_sv0;
float4 cbR3_sv1;


}; 

StructuredBuffer<float4> sbR0;

RWStructuredBuffer<float4> rwSBR0 : register ( u2 );

RWStructuredBuffer<float4> rwSBR2;

RWStructuredBuffer<float4> rwSBR3;

RWStructuredBuffer<float4> rwSBR4 : register ( u4 );


SamplerState samplerR4;


Texture2D<float4> erTex : register ( t1 );

Texture2D<float4> textureR2;

Texture2D<float4> textureR4;

Texture2D<float4> textureR5;

RWTexture2D<float4> rwTextureR0;

RWTexture2D<float4> rwTextureR1 : register ( u6 );

RWTexture2D<float4> rwTextureR3;


ByteAddressBuffer  rbR0 : register ( t2 );

ByteAddressBuffer  rbR1;

RWByteAddressBuffer  rwRBR0;

RWByteAddressBuffer  rwRBR1;


PS_OUT main(PS_IN psIn) 
{ 

PS_OUT psOut; 

psOut.output0 = cbR0_sv0 + cbR2_sv0 + cbR3_sv1 + erTex.Load(int3(0, 0, 0)) + textureR2.Load(int3(0, 0, 0)) + textureR4.Load(int3(0, 0, 0)) + textureR5.Load(int3(0, 0, 0)) + rwTextureR0.Load(0) + rwTextureR1.Load(0) + rwTextureR3.Load(0) + rwSBR3[0] + rwSBR4[0] + rbR0.Load4(0) + psIn.input0 + psIn.input1 + float4(0.05, 0.05, 0.05, 0.05);
psOut.output1 = cbR0_sv0 + cbR2_sv0 + cbR3_sv1 + erTex.Load(int3(0, 0, 0)) + textureR2.Load(int3(0, 0, 0)) + textureR4.Load(int3(0, 0, 0)) + textureR5.Load(int3(0, 0, 0)) + rwTextureR0.Load(0) + rwTextureR1.Load(0) + rwTextureR3.Load(0) + rwSBR3[0] + rwSBR4[0] + rbR0.Load4(0) + psIn.input0 + psIn.input1 + float4(0.05, 0.05, 0.05, 0.05);


return psOut; 
} 
