/***************************************************************************
# Copyright (c) 2015, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************/

#ifndef _FALCOR_BSDFS_H_
#define _FALCOR_BSDFS_H_

// Make sure we get the macros like `_fn` and `_ref`
// TODO: just eliminate these since we know this is pure Slang.
#include "HostDeviceData.h"

__import Helpers;

/**********************************************************************************
BRDF helpers
**********************************************************************************/

/**********************************************************************************
Microfacet routines: Fresnel (conductor/dielectric), shadowing/masking
**********************************************************************************/

/**
Schlick's approximation for reflection of a dielectric media
*/
float _fn dielectricFresnelSchlick(in const float VdH, in const float IoR)
{
    float R0 = (1.f - IoR) / (1.f + IoR);	R0 *= R0;
    const float OneMinusCos = 1.f - VdH;
    const float OneMinusCos2 = OneMinusCos*OneMinusCos;
    return R0 + (1.f - R0) * OneMinusCos*OneMinusCos2*OneMinusCos2;	// to the power of 5
}

/**
Simplified Fresnel factor (w/o polarization) of a planar interface
between two dielectrics
*/
float _fn dielectricFresnelFast(in const float NdE, in const float NdL, in const float IoR)
{
    const float Rs = (NdE - IoR * NdL) / (NdE + IoR * NdL);	// Transmitted-to-incident wave ratio, perpendicular component
    const float Rp = (IoR * NdE - NdL) / (IoR * NdE + NdL);	// Transmitted-to-incident wave ratio, parallel component
    return (Rs * Rs + Rp * Rp) * .5f;						// Total amplitude of the transmitted wave
}

/**
Full Fresnel factor (w/o polarization) of a planar interface
between two dielectrics
*/
float _fn dielectricFresnel(in float NdE, in const float IoR)
{
    const float realIoR = (NdE >= 0.f) ? 1.f / IoR : IoR;
    // Perform a refraction
    const float NdL2 = 1.f - realIoR * realIoR * (1.f - NdE * NdE);
    if(NdL2 <= 0.f)	// Total internal reflection case
        return 1.f;
    const float NdL = sqrt(NdL2);
    NdE = abs(NdE);		// Wrap the incident direction
    const float Rp = (IoR * NdL - NdE) / (IoR * NdL + NdE);	// Reflected-to-incident wave ratio, parallel component
    const float Rs = (NdE - IoR * NdL) / (NdE + IoR * NdL);	// Reflected-to-incident wave ratio, parallel component
    return (Rp * Rp + Rs * Rs) * .5f;						// Total amplitude of the reflected wave
}

/**
Full Fresnel factor (w/o polarization) of a planar interface
between a dielectric (usually air) and a conductive media
*/
float _fn conductorFresnel(in float NdE, in const float IoR, in const float Kappa)
{
    const float Kappa2 = Kappa * Kappa;
    const float TotalIoR2 = IoR * IoR + Kappa2;	// Total magnitude of IoR: Real plus complex parts of IoR
    NdE = saturate(NdE);			// No refraction allowed
    const float NdE2 = NdE * NdE;
    const float ReducedNdE2 = TotalIoR2 * NdE2;
    const float Rp2 = (ReducedNdE2 - IoR * NdE * 2.f + 1.f) / (ReducedNdE2 + IoR * NdE * 2.f + 1.f);	// Reflected-to-incident wave ratio, perpendicular component
    const float Rs2 = (TotalIoR2 - IoR * NdE * 2.f + NdE2) / (TotalIoR2 + IoR * NdE * 2.f + NdE2);		// Reflected-to-incident wave ratio, parallel component
    return (Rp2 + Rs2) * .5f;					// Total amplitude of the reflected wave
}

/**********************************************************************************
Distribution functions
**********************************************************************************/

/*******************************************************************************************
Lambertian diffuse BSDF. Returns a clamped N.L factor.
*/
float _fn evalDiffuseBSDF(in const float3 shadeNormal, in const float3 lightDir)
{
    return max(0.f, dot(shadeNormal, lightDir)) * M_1_PIf;
}

/*******************************************************************************************
Blinn-Phong normal distribution function (NDF).
*/
float _fn evalPhongDistribution(in float3 N, in float3 V, in float3 L, in float roughness)
{
    const float spec_power = convertRoughnessToShininess(roughness);
    const float3 H = normalize(L + V);
    const float NoH = max(0.f, dot(N, H));
    const float normalization = (spec_power + 2.f) / (2.f * M_PIf);
    return pow(NoH, spec_power) * normalization;
}

/*******************************************************************************************
Beckmann normal distribution function (NDF).
*/
float _fn evalBeckmannDistribution(in const float3 N, in const float3 H, in float roughness)
{
    const float a2 = roughness * roughness;
    // dot products that we need
    const float NoH = dot(N, H);
    const float NoH2 = NoH * NoH;
    // Compute Beckmann distribution
    const float exponent = max(0.f, (1.f - NoH2) / (a2 * NoH2));
    return exp(-exponent) / (M_PIf * a2 * NoH2 * NoH2);
}

float _fn evalBeckmannDistribution(in const float3 H, in const float2 rgns)
{
    const float NoH2 = H.z * H.z;
    const float2 Hproj = float2(H.x, H.y);
    // Compute Beckmann distribution
    const float exponent = dot(Hproj / (rgns*rgns), Hproj) / NoH2;
    return exp(-exponent) / (M_PIf * rgns.x * rgns.y * NoH2 * NoH2);
}

/**
Returns a standard deviation of the Beckmann distribution
as a cone apex angle in parallel plane domain based on the roughness.
*/
float _fn beckmannStdDevAngle(in const float roughness)
{
    return atan(sqrt(0.5f) * roughness);
}

/**
An approximation of the off specular peak;
due to the other approximations we found this one performs better than Frostbite PBS'15.
N is the normal direction
R is the mirror vector
This approximation works fine for G smith correlated and uncorrelated
*/
float3 _fn getBeckmannDominantDir(in const float3 N, in const float3 R,
    in const float roughness)
{
    const float smoothness = clamp(1.f - roughness, 0.0f, 1.0f);
    const float lerpFactor = smoothness * (sqrt(smoothness) + roughness);
    // The result is not normalized as we fetch in a cubemap
    return lerp(N, R, lerpFactor);
}


/*******************************************************************************************
GGX normal distribution function (NDF).
*/
float _fn evalGGXDistribution(in const float3 N, in const float3 H, in const float roughness)
{
    // This doesn't look correct, so disabled    
    const float a2 = roughness * roughness;
    // dot products that we need
    const float NoH = saturate(dot(N, H));
    // D term
    float D_denom = (NoH * a2 - NoH) * NoH + 1.0f;
    D_denom = M_PIf * D_denom * D_denom;
    return a2 / D_denom;
}

float _fn evalGGXDistribution(in const float3 H, in const float2 rgns)
{
    // Numerically robust (w.r.t rgns=0) implementation of anisotropic GGX
    const float anisoU = rgns.y < rgns.x ? rgns.y / rgns.x : 1.f;
    const float anisoV = rgns.x < rgns.y ? rgns.x / rgns.y : 1.f;
    const float r = min(rgns.x, rgns.y);
    const float NoH2 = H.z * H.z;
    const float2 Hproj = float2(H.x, H.y);
    const float exponent = dot(Hproj / float2(anisoU*anisoU, anisoV*anisoV), Hproj);
    const float root = NoH2 * r*r + exponent;
    return r*r / (M_PIf * anisoU * anisoV * root * root);
}

/*******************************************************************************************
Compute shadowing and masking
*******************************************************************************************/

/**
Computes the intermediate effective microfacet roughness observed from a particular direction
\param[in] dir view direction in local shading frame
\param[in] rghns original anisotropic roughness of the microfacet BSDF
returns effective visible roughness
*/
float _fn effectiveVisibleRoughness(const in float3 dir, const in float2 rghns)
{
    const float recipSinThSq = 1.f / (1.f - dir.z * dir.z);
    const float2 dirPlane = float2(dir.x, dir.y);
    const float2 cosSinPhiSq = dirPlane * dirPlane * recipSinThSq;
    const float2 res = rghns * rghns * cosSinPhiSq;
    return sqrt(res.x + res.y);
}

/**
Computes the Smith'67 shadowing or masking term from a direction.
\param[in] dir view direction in local shading frame
\param[in] h half vector (microfacet direction) in local shading frame
\param[in] rghns original anisotropic roughness of the microfacet BSDF
\param[in] ndfType type of NDF. Only Beckmann and GGX are supported so far
returns amount of visible microfacets
*/
float _fn GSmith(const in float3 dir, const in float3 h, const in float2 rghns, in const uint ndfType)
{
    if(dot(dir, h) * dir.z <= 0.f)
        return 0.f;
    const float sinThSq = 1.f - dir.z * dir.z;
    if(sinThSq <= 0.f)
        return 1.f;
    const float recipSlope = sqrt(sinThSq) / dir.z;
    const float alpha = effectiveVisibleRoughness(dir, rghns);
    if(ndfType == NDFBeckmann)
    {
        // Use the Beckmann G fit from [Walter07]
        const float a = 1.f / (alpha * recipSlope);
        if(a > 1.6f)
            return 1.f;
        const float aSq = a*a;
        return (3.535f * a + 2.181f * aSq) / (1.f + 2.276f * a + 2.577f * aSq);
    }

    // Otherwise it's GGX, use GGX shadowing/masking
    const float isectRoot = alpha * recipSlope;
    return 2.f / (1.f + length(float2(1.f, isectRoot)));
}

/**
Computes shadowing and masking term for microfacet BRDFs
*/
float _fn evalMicrofacetTerms(in const float3 T, in const float3 B, in const float3 N,
    in const float3 h, in const float3 V, in const float3 L,
    in const float2 roughness, in const uint ndfType, bool transmissive)
{
    const float3 lTg = float3(dot(T, L), dot(B, L), dot(N, L));
    const float3 vTg = float3(dot(T, V), dot(B, V), dot(N, V));

    /* If it's not transmission, they should be on the same side of the hemisphere */
    if(!transmissive && lTg.z*vTg.z <= 0.f)
        return 0.f;

    /* Compute shadowing and masking separately */
    return GSmith(vTg, h, roughness, ndfType) * GSmith(lTg, h, roughness, ndfType);
}

/*******************************************************************************************
    Sampling functions
*******************************************************************************************/

/**
    Sample the microfacet BRDF using Beckmann normal distribution function

    \param[in] wo Outgoing direction towards the camera
    \param[in] roughness Material roughness
    \param[in] rSample Random numbers for sampling
    \param[out] m Microfacet normal
    \param[out] wi Incident direction after importance sampling the BRDF
    \param[out] pdf Probability density function for choosing the incident direction
*/
float _fn sampleBeckmannDistribution(
	in const float3 wo,
	in const float2 roughness,
	in const float2 rSample,
	_ref(float3) m,
	_ref(float3) wi,
	_ref(float) pdf)
{
	float alphaSqr = roughness.x * roughness.x;

	// Sample phi component
	float temp = 2.f * M_PIf * rSample.y;
	float sinPhiM = sin(temp);
	float cosPhiM = cos(temp);

	// Sample theta component
	float tanThetaMSqr = alphaSqr * -log(1.f - rSample.x);
	float cosThetaM = 1.f / sqrt(1.f + tanThetaMSqr);

	// Compute probability density function based on the sampled components
	pdf = (1.f - rSample.x) / (M_PIf * roughness.x * roughness.y * cosThetaM * cosThetaM * cosThetaM);

	// Sanity check
	if (pdf < 1e-20f)
		pdf = 0.f;

	// Sample microfacet normal
	float sinThetaM = sqrt(max(0.f, 1.f - cosThetaM * cosThetaM));

	m.x = sinThetaM * cosPhiM;
	m.y = sinThetaM * sinPhiM;
	m.z = cosThetaM;

	// Specular reflection based on the microfacet normal
	wi = -wo - 2.f * dot(-wo, m) * m;
	if (wi.z <= 0.f || pdf <= 0.f)
		return 0.f;

	float weight = evalBeckmannDistribution(m, roughness) * dot(wo, m) / (pdf * wo.z);

	// Cook-Torrance Jacobian
	pdf /= 4.f * dot(wi, m);

	return weight;
}

/**
    Sample the microfacet BRDF using GGX normal distribution function

    \param[in] wo Outgoing direction towards the camera
    \param[in] roughness Material roughness
    \param[in] rSample Random numbers for sampling
    \param[out] m Microfacet normal
    \param[out] wi Incident direction after importance sampling the BRDF
    \param[out] pdf Probability density function for choosing the incident direction
*/
float _fn sampleGGXDistribution(
	in const float3 wo,
	in float2 roughness,
	in const float2 rSample,
	_ref(float3) m,
	_ref(float3) wi,
	_ref(float) pdf)
{
	float alphaSqr = roughness.x * roughness.x;

    const bool perfectSpecular = alphaSqr <= 1e-6f;

    if(perfectSpecular)
    {
        alphaSqr = 0.f;
        roughness = 0;
    }

	// Sample phi component
	float temp = 2.f * M_PIf * rSample.y;
	float sinPhiM = sin(temp);
	float cosPhiM = cos(temp);

	// Sample theta component
	float tanThetaMSqr = alphaSqr * rSample.x / (1.f - rSample.x);
	float cosThetaM = 1.f / sqrt(1.f + tanThetaMSqr);

	// Compute probability density function based on the sampled components
	temp = 1.f + tanThetaMSqr / alphaSqr;
	pdf = M_1_PIf / (roughness.x * roughness.y * cosThetaM * cosThetaM * cosThetaM * temp * temp);

	// Sanity checks
    if(perfectSpecular)
        pdf = 1.f;
	else if (pdf < 1e-20f)
		pdf = 0.f;

	// Sample microfacet normal
	float sinThetaM = sqrt(max(0.f, 1.f - cosThetaM * cosThetaM));

	m.x = sinThetaM * cosPhiM;
	m.y = sinThetaM * sinPhiM;
	m.z = cosThetaM;

	// Specular reflection based on the microfacet normal
	wi = -wo - 2.f * dot(-wo, m) * m;

	// Sanity check
	if (wi.z <= 0.f || pdf <= 0.f)
		return 0.f;

	float weight = evalGGXDistribution(m, roughness);
    if(perfectSpecular)
        weight = 1.f;
    
    weight *= dot(wo, m) / (pdf * wo.z);

	// Cook-Torrance Jacobian
	pdf /= 4.f * dot(wi, m);

	return weight;
}

#endif	// _FALCOR_BSDFS_H_