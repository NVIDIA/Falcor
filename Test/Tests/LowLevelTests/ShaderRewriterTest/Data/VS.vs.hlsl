

struct VS_OUT 
{ 
float4 output0 : OUTPUT0; 
float4 output1 : OUTPUT1; 
float4 output4 : OUTPUT4; 
}; 






cbuffer cbR0 
{ 
float4 cbR0_sv0;
float4 cbR0_sv1;


}; 

cbuffer cbR2 
{ 
float4 cbR2_sv0;


}; 

StructuredBuffer<float4> sbR0;

StructuredBuffer<float4> sbR2 : register ( t2 );

StructuredBuffer<float4> sbR3;

StructuredBuffer<float4> sbR4;

RWStructuredBuffer<float4> rwSBR0;

RWStructuredBuffer<float4> rwSBR1 : register ( u5 );

RWStructuredBuffer<float4> rwSBR2;



Texture2D<float4> textureR0;

Texture2D<float4> textureR1;



VS_OUT main(float4 pos : POSITION) 
{ 

VS_OUT vsOut; 

vsOut.output0 = textureR0.Load(int3(0, 0, 0)) + textureR1.Load(int3(0, 0, 0)) + sbR0[0] + sbR2[0] + sbR3[0] + sbR4[0] + rwSBR0[0] + rwSBR1[0] + float4(0.05, 0.05, 0.05, 0.05);
vsOut.output1 = textureR0.Load(int3(0, 0, 0)) + textureR1.Load(int3(0, 0, 0)) + sbR0[0] + sbR2[0] + sbR3[0] + sbR4[0] + rwSBR0[0] + rwSBR1[0] + float4(0.05, 0.05, 0.05, 0.05);
vsOut.output4 = textureR0.Load(int3(0, 0, 0)) + textureR1.Load(int3(0, 0, 0)) + sbR0[0] + sbR2[0] + sbR3[0] + sbR4[0] + rwSBR0[0] + rwSBR1[0] + float4(0.05, 0.05, 0.05, 0.05);


return vsOut; 
} 
