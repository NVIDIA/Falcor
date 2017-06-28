

struct PS_IN 
{ 
float4 input0 : OUTPUT0; 
float4 input1 : OUTPUT1; 
float4 input2 : OUTPUT2; 
float4 input3 : OUTPUT3; 
float4 input4 : OUTPUT4; 
float4 input5 : OUTPUT5; 
}; 


struct PS_OUT 
{ 
float4 output0 : SV_TARGET0; 
float4 output1 : SV_TARGET1; 
float4 output2 : SV_TARGET2; 
float4 output3 : SV_TARGET3; 
float4 output4 : SV_TARGET4; 
float4 output5 : SV_TARGET5; 
float4 output6 : SV_TARGET6; 
}; 






cbuffer cbR0 
{ 
float4 cbR0_sv0;
float4 cbR0_sv1;


}; 

cbuffer cbR2 
{ 
float4 cbR2_sv0;
float4 cbR2_sv1;


}; 

cbuffer cbR3 
{ 
float4 cbR3_sv0;


}; 

StructuredBuffer<float4> sbR0;



RWTexture2D<float4> erTex : register ( u7 );

RWTexture2D<float4> rwTextureR1;

RWTexture2D<float4> rwTextureR2;


ByteAddressBuffer  rbR0;

RWByteAddressBuffer  rwRBR0;

RWByteAddressBuffer  rwRBR1;


PS_OUT main(PS_IN psIn) 
{ 

PS_OUT psOut; 

psOut.output0 = cbR2_sv1 + erTex.Load(0) + rwTextureR1.Load(0) + rwTextureR2.Load(0) + rbR0.Load4(0) + psIn.input0 + psIn.input1 + psIn.input2 + psIn.input3 + psIn.input4 + psIn.input5 + float4(0.05, 0.05, 0.05, 0.05);
psOut.output1 = cbR2_sv1 + erTex.Load(0) + rwTextureR1.Load(0) + rwTextureR2.Load(0) + rbR0.Load4(0) + psIn.input0 + psIn.input1 + psIn.input2 + psIn.input3 + psIn.input4 + psIn.input5 + float4(0.05, 0.05, 0.05, 0.05);
psOut.output2 = cbR2_sv1 + erTex.Load(0) + rwTextureR1.Load(0) + rwTextureR2.Load(0) + rbR0.Load4(0) + psIn.input0 + psIn.input1 + psIn.input2 + psIn.input3 + psIn.input4 + psIn.input5 + float4(0.05, 0.05, 0.05, 0.05);
psOut.output3 = cbR2_sv1 + erTex.Load(0) + rwTextureR1.Load(0) + rwTextureR2.Load(0) + rbR0.Load4(0) + psIn.input0 + psIn.input1 + psIn.input2 + psIn.input3 + psIn.input4 + psIn.input5 + float4(0.05, 0.05, 0.05, 0.05);
psOut.output4 = cbR2_sv1 + erTex.Load(0) + rwTextureR1.Load(0) + rwTextureR2.Load(0) + rbR0.Load4(0) + psIn.input0 + psIn.input1 + psIn.input2 + psIn.input3 + psIn.input4 + psIn.input5 + float4(0.05, 0.05, 0.05, 0.05);
psOut.output5 = cbR2_sv1 + erTex.Load(0) + rwTextureR1.Load(0) + rwTextureR2.Load(0) + rbR0.Load4(0) + psIn.input0 + psIn.input1 + psIn.input2 + psIn.input3 + psIn.input4 + psIn.input5 + float4(0.05, 0.05, 0.05, 0.05);
psOut.output6 = cbR2_sv1 + erTex.Load(0) + rwTextureR1.Load(0) + rwTextureR2.Load(0) + rbR0.Load4(0) + psIn.input0 + psIn.input1 + psIn.input2 + psIn.input3 + psIn.input4 + psIn.input5 + float4(0.05, 0.05, 0.05, 0.05);


return psOut; 
} 
