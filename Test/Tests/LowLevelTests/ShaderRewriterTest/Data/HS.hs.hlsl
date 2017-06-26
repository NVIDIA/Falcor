
struct VS_CONTROL_POINT_OUTPUT
{
    float3 vPosition : WORLDPOS;
    float2 vUV       : TEXCOORD0;
    float3 vTangent  : TANGENT;
};

struct BEZIER_CONTROL_POINT
{
    float3 vPosition	: BEZIERPOS;
};

struct HS_CONSTANT_DATA_OUTPUT
{
    float Edges[4]        : SV_TessFactor;
    float Inside[2]       : SV_InsideTessFactor;
    
    float3 vTangent[4]    : TANGENT;
    float2 vUV[4]         : TEXCOORD;
    float3 vTanUCorner[4] : TANUCORNER;
    float3 vTanVCorner[4] : TANVCORNER;
    float4 vCWts          : TANWEIGHTS;
};


#define MAX_POINTS 32

HS_CONSTANT_DATA_OUTPUT SubDToBezierConstantsHS(InputPatch<VS_CONTROL_POINT_OUTPUT, MAX_POINTS> ip, uint PatchID : SV_PrimitiveID )
{	
    HS_CONSTANT_DATA_OUTPUT Output;
    Output.Edges[0] = 0.0;
    Output.Edges[1] = 0.0;
    Output.Edges[2] = 0.0;
    Output.Edges[3] = 0.0;

    Output.Inside[0] = 0.0;
    Output.Inside[1] = 0.0;
    
    
    Output.vTangent[0] = float3(0.0, 0.0, 0.0);
    Output.vUV[0] = float2(0.0, 0.0);
    Output.vTanUCorner[0] = float3(0.0, 0.0, 0.0);
    Output.vCWts = float4(0.0, 0.0, 0.0, 0.0);
    
    return Output;
}


[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(16)]
[patchconstantfunc("SubDToBezierConstantsHS")]
BEZIER_CONTROL_POINT main(InputPatch<VS_CONTROL_POINT_OUTPUT, MAX_POINTS> ip, uint i : SV_OutputControlPointID, uint PatchID : SV_PrimitiveID )
{
    BEZIER_CONTROL_POINT Output;

    Output.vPosition = float3(0.0, 0.0, 0.0);
    return Output;
}

