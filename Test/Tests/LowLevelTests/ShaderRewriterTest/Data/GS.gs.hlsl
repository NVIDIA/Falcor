

struct GS_IN 
{ 
float4 input0 : INPUT0; 
float4 input1 : INPUT1; 
float4 input2 : INPUT2; 
float4 input3 : INPUT3; 
float4 input4 : INPUT4; 
float4 input5 : INPUT5; 
float4 input6 : INPUT6; 
}; 



struct GS_OUT 
{ 
float4 output0 : OUTPUT0; 
float4 output1 : OUTPUT1; 
}; 






cbuffer cbR3 
{ 
float4 cbR3_sv0;
float4 cbR3_sv1;


}; 

StructuredBuffer<float4> sbR0;

RWStructuredBuffer<float4> rwSBR1;

RWStructuredBuffer<float4> rwSBR3;

RWStructuredBuffer<float4> rwSBR4;


SamplerState samplerR2 : register ( s2 );

SamplerState samplerR3;

SamplerState samplerR4 : register ( s4 );


Texture2D<float4> erTex : register ( t0 );

Texture2D<float4> textureR0 : register ( t3 );

Texture2D<float4> textureR1 : register ( t4 );

Texture2D<float4> textureR2;

Texture2D<float4> textureR3;

Texture2D<float4> textureR4;

RWTexture2D<float4> rwTextureR0;

RWTexture2D<float4> rwTextureR1;

RWTexture2D<float4> rwTextureR2;

RWTexture2D<float4> rwTextureR3;


ByteAddressBuffer  rbR0;

ByteAddressBuffer  rbR3;

RWByteAddressBuffer  rwRBR0;

RWByteAddressBuffer  rwRBR1;



[maxvertexcount(3)] 
 
void main(triangle GS_IN input[3], inout TriangleStream<GS_OUT> triStream) 
{ 
GS_OUT gsOut; 

gsOut.output0 = cbR3_sv1 + erTex.Load(int3(0, 0, 0)) + textureR0.Load(int3(0, 0, 0)) + textureR1.Load(int3(0, 0, 0)) + textureR2.Load(int3(0, 0, 0)) + textureR3.Load(int3(0, 0, 0)) + textureR4.Load(int3(0, 0, 0)) + rwTextureR0.Load(0) + rwTextureR1.Load(0) + rwTextureR2.Load(0) + rwTextureR3.Load(0) + rwSBR1[0] + rwSBR3[0] + rwSBR4[0] + rbR0.Load4(0) + float4(0.05, 0.05, 0.05, 0.05);
gsOut.output1 = cbR3_sv1 + erTex.Load(int3(0, 0, 0)) + textureR0.Load(int3(0, 0, 0)) + textureR1.Load(int3(0, 0, 0)) + textureR2.Load(int3(0, 0, 0)) + textureR3.Load(int3(0, 0, 0)) + textureR4.Load(int3(0, 0, 0)) + rwTextureR0.Load(0) + rwTextureR1.Load(0) + rwTextureR2.Load(0) + rwTextureR3.Load(0) + rwSBR1[0] + rwSBR3[0] + rwSBR4[0] + rbR0.Load4(0) + float4(0.05, 0.05, 0.05, 0.05);


} 
