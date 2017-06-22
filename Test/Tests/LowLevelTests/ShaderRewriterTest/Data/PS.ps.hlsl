

struct VS_OUT 
{ 
float4 output0 : OUTPUT0; 
float4 output1 : OUTPUT1; 
float4 output4 : OUTPUT4; 
float4 output2 : OUTPUT2; 
}; 


struct PS_OUT 
{ 
float4 output0 : SV_TARGET0; 
float4 output3 : SV_TARGET3; 
float4 output5 : SV_TARGET5; 
float4 output7 : SV_TARGET7; 
}; 





cbuffer cbR0  : register ( b0 )
{ 
float4 cbR0_sv0;
float4 cbR0_sv1;


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


}; 

cbuffer cbR3 
{ 
float4 cbR3_sv0;
float4 cbR3_sv1;


}; 

StructuredBuffer<float4> sbR0 : register ( t0 );

StructuredBuffer<float4> sbR1;

StructuredBuffer<float4> sbR2;

StructuredBuffer<float4> sbR3;

StructuredBuffer<float4> sbR4 : register ( t3 );

RWStructuredBuffer<float4> rwSBR0 : register ( u4 );

RWStructuredBuffer<float4> rwSBR1;



Texture2D<float4> textureR0;

Texture2D<float4> textureR1;



PS_OUT main(VS_OUT vOut) 
{ 

PS_OUT psOut; 

psOut.output0 = cbR3_sv0 + textureR0.Load(int3(0, 0, 0)) + textureR1.Load(int3(0, 0, 0)) + sbR0[0] + sbR2[0] + sbR3[0] + sbR4[0] + rwSBR0[0] + rwSBR1[0] + float4(0.05, 0.05, 0.05, 0.05);
psOut.output3 = cbR3_sv0 + textureR0.Load(int3(0, 0, 0)) + textureR1.Load(int3(0, 0, 0)) + sbR0[0] + sbR2[0] + sbR3[0] + sbR4[0] + rwSBR0[0] + rwSBR1[0] + float4(0.05, 0.05, 0.05, 0.05);
psOut.output5 = cbR3_sv0 + textureR0.Load(int3(0, 0, 0)) + textureR1.Load(int3(0, 0, 0)) + sbR0[0] + sbR2[0] + sbR3[0] + sbR4[0] + rwSBR0[0] + rwSBR1[0] + float4(0.05, 0.05, 0.05, 0.05);
psOut.output7 = cbR3_sv0 + textureR0.Load(int3(0, 0, 0)) + textureR1.Load(int3(0, 0, 0)) + sbR0[0] + sbR2[0] + sbR3[0] + sbR4[0] + rwSBR0[0] + rwSBR1[0] + float4(0.05, 0.05, 0.05, 0.05);


return psOut; 
} 
