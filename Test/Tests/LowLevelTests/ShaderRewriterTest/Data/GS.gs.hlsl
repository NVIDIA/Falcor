

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






cbuffer cbR2 
{ 
float4 cbR2_sv0;
float4 cbR2_sv1;


}; 

RWStructuredBuffer<float4> rwSBR0;

RWStructuredBuffer<float4> rwSBR3;

RWStructuredBuffer<float4> rwSBR4;


SamplerState samplerR0;

SamplerState samplerR4;


Texture2D<float4> textureR0;

Texture2D<float4> textureR2 : register ( t5 );

Texture2D<float4> textureR3;

Texture2D<float4> textureR4;

Texture2D<float4> textureR5;

RWTexture2D<float4> rwTextureR1;

RWTexture2D<float4> rwTextureR3 : register ( u9 );


ByteAddressBuffer  rbR0;

ByteAddressBuffer  rbR2 : register ( t1 );



[maxvertexcount(3)] 
 
void main(triangle GS_IN input[3], inout TriangleStream<GS_OUT> triStream) 
{ 
GS_OUT gsOut; 

gsOut.output0 = cbR2_sv0 + textureR0.Load(int3(0, 0, 0)) + textureR2.SampleLevel(samplerR4, float2(0,0), 0) + textureR3.Load(int3(0, 0, 0)) + textureR4.SampleLevel(samplerR0, float2(0,0), 0) + textureR5.Load(int3(0, 0, 0)) + rwTextureR1.Load(0) + rwTextureR3.Load(0) + rwSBR3[0] + rbR0.Load4(0) + rbR2.Load4(0) + float4(0.05, 0.05, 0.05, 0.05);
gsOut.output1 = cbR2_sv0 + textureR0.Load(int3(0, 0, 0)) + textureR2.SampleLevel(samplerR4, float2(0,0), 0) + textureR3.Load(int3(0, 0, 0)) + textureR4.SampleLevel(samplerR0, float2(0,0), 0) + textureR5.Load(int3(0, 0, 0)) + rwTextureR1.Load(0) + rwTextureR3.Load(0) + rwSBR3[0] + rbR0.Load4(0) + rbR2.Load4(0) + float4(0.05, 0.05, 0.05, 0.05);


} 
