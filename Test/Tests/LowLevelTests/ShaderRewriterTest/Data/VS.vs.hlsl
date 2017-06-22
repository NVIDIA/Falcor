

struct VS_OUT 
{ 
float4 output0 : OUTPUT0; 
float4 output1 : OUTPUT1; 
float4 output4 : OUTPUT4; 
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



Texture2D<float4> textureR0;



VS_OUT main(float4 pos : POSITION) 
{ 

VS_OUT vsOut; 

vsOut.output0 = cbR3_sv0 + textureR0.Load(int3(0, 0, 0)) + sbR0[0] + sbR2[0] + sbR3[0] + sbR4[0] + float4(0.05, 0.05, 0.05, 0.05);
vsOut.output1 = cbR3_sv0 + textureR0.Load(int3(0, 0, 0)) + sbR0[0] + sbR2[0] + sbR3[0] + sbR4[0] + float4(0.05, 0.05, 0.05, 0.05);
vsOut.output4 = cbR3_sv0 + textureR0.Load(int3(0, 0, 0)) + sbR0[0] + sbR2[0] + sbR3[0] + sbR4[0] + float4(0.05, 0.05, 0.05, 0.05);


return vsOut; 
} 
