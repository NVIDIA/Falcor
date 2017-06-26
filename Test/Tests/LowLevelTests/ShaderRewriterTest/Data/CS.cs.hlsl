




cbuffer cbR0  : register ( b0 )
{ 
float4 cbR0_sv0;


}; 

cbuffer cbR1 
{ 
float4 cbR1_sv0;
float4 cbR1_sv1;
float4 cbR1_sv2;


}; 

cbuffer cbR2  : register ( b1 )
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

StructuredBuffer<float4> sbR1;

RWStructuredBuffer<float4> rwSBR0 : register ( u0 );

RWStructuredBuffer<float4> rwSBR1;

RWStructuredBuffer<float4> rwSBR2 : register ( u1 );

RWStructuredBuffer<float4> rwSBR3;

RWStructuredBuffer<float4> rwSBR4 : register ( u2 );



Texture2D<float4> textureR0;

Texture2D<float4> textureR1;


ByteAddressBuffer  rbR0 : register ( t0 );

ByteAddressBuffer  rbR1;

ByteAddressBuffer  rbR2 : register ( t1 );

ByteAddressBuffer  rbR3;

RWByteAddressBuffer  rwRBR0 : register ( u4 );

RWByteAddressBuffer  rwRBR1;



[numthreads(4, 4, 4)] 
void main() 
{ 
} 
