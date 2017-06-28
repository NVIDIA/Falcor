

struct VS_OUT 
{ 
float4 output0 : OUTPUT0; 
float4 output1 : OUTPUT1; 
}; 







cbuffer cbR0 
{ 
float4 cbR0_sv0;
float4 cbR0_sv1;


}; 

cbuffer cbR1 
{ 
float4 cbR1_sv0;


}; 

RWStructuredBuffer<float4> erRWUAV : register ( u7 );

StructuredBuffer<float4> sbR0;



RWTexture2D<float4> rwTextureR0;

RWTexture2D<float4> rwTextureR1;

RWTexture2D<float4> rwTextureR2;

RWTexture2D<float4> rwTextureR3;


ByteAddressBuffer  rbR0;

RWByteAddressBuffer  rwRBR0 : register ( u9 );

RWByteAddressBuffer  rwRBR1;


VS_OUT main(float4 pos : POSITION) 
{ 

VS_OUT vsOut; 

vsOut.output0 = cbR1_sv0 + rwTextureR0.Load(0) + rwTextureR1.Load(0) + rwTextureR2.Load(0) + rwTextureR3.Load(0) + rbR0.Load4(0) + float4(0.05, 0.05, 0.05, 0.05);
vsOut.output1 = cbR1_sv0 + rwTextureR0.Load(0) + rwTextureR1.Load(0) + rwTextureR2.Load(0) + rwTextureR3.Load(0) + rbR0.Load4(0) + float4(0.05, 0.05, 0.05, 0.05);


return vsOut; 
} 
