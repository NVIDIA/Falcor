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

#ifndef _FALCOR_SHADING_H_
#define _FALCOR_SHADING_H_

#include "Helpers.h"

/*******************************************************************
Documentation
*******************************************************************/

// Possible user-defined global macros to configure shading 
//#define _MS_DISABLE_ALPHA_TEST	///< Disables alpha test (handy for early-z etc.)
//#define _MS_DISABLE_TEXTURES		///< Disables material texture fetches, all materials are solid color
//#define _MS_USER_DERIVATIVES		///< Use user-passed dx/dy derivatives for texture filtering during shading
//#define _MS_USER_NORMAL_MAPPING	///< Use user-defined callback of a form void PerturbNormal(in const SMaterial mat, inout ShadingAttribs ShAttr) to perform normal mapping
//#define _MS_NUM_LAYERS			///< Use a pre-specified number of layers to use in the material
//#define _MS_IMPORTANCE_SAMPLING   ///< Use BSDF importance sampling for path tracer next event estimation

/*******************************************************************
Shading routines
*******************************************************************/

/**
This stores the information about the current light source at the shading point.
This includes a direction from a shading point towards the light,
radiance emitted from the light souce, which is *received* at the shading point.
*/
struct LightAttribs
{
    vec3	L;				///< Normalized direction to the light at shading hit
    vec3	lightIntensity;	///< Radiance of the emitted light at shading hit

    vec3    P;              ///< Sampled point on the light source
    vec3    N;              ///< Normal of the sampled point on the light source
	float   pdf;            ///< Probability density function of sampling the light source

    vec3    points[4];
};

/**
This is an output resulting radiance of a full pass over all material layers
after evaluating the material for a single light source.
This result can be accumulated to a total shading result for all light sources.
*/
struct PassOutput
{
    vec3	value;					///< Outgoing radiance after evaluating the complete material for a single light
    vec3	diffuseAlbedo;			///< Total view-independent albedo color component of the material
    vec3	diffuseIllumination;	///< Total view-independent outgoing radiance from one light before multiplication by albedo
    vec3	specularAlbedo;			///< Total view-dependent albedo (specular) color component of the material
    vec3	specularIllumination;	///< Total view-dependent outgoing radiance from one light before multiplication by albedo

    vec2    roughness;              ///< Roughness from the last layer
    vec2    effectiveRoughness;     ///< Accumulated effective roughness
};

/**
This is an accumulative structure that collects the resulting output
of material evaluations with all light sources.
*/
struct ShadingOutput
{
    vec3	diffuseAlbedo;			///< Total view-independent albedo color component of the material
    vec3	diffuseIllumination;	///< Total view-independent outgoing radiance from all lights before multiplication by albedo
    vec3	specularAlbedo;			///< Total view-dependent albedo (specular) color component of the material
    vec3	specularIllumination;	///< Total view-dependent outgoing radiance from all light before multiplication by albedo

    vec3	finalValue;				///< Outgoing radiance after evaluating the complete material for all lights

	vec3    wi;                     ///< Incident direction after importance sampling the BRDF
	float   pdf;                    ///< Probability density function for choosing the incident direction
	vec3    thp;                    ///< Current path throughput

	vec2    effectiveRoughness;     ///< Sampled brdf roughness, or for evaluated material, an effective roughness

#ifdef CUDA_CODE
    _fn ShadingOutput()
    {
        diffuseAlbedo = v3(0.f);
        diffuseIllumination = v3(0.f);
        specularAlbedo = v3(0.f);
        specularIllumination = v3(0.f);
        finalValue = v3(0.f);
        wi = v3(0.f);
        pdf = 0.f;
        thp = v3(0.f);
    }
#endif
};

/* Include all shading routines, including endpoints (camera and light) */
#include "Cameras.h"
#include "Lights.h"
#include "BSDFs.h"

/*******************************************************************
Material shading building blocks
*******************************************************************/

/**
Prepares all light-independent attributes needed to perform shading at a hit point.
This includes fetching all textures and computing final shading attributes, like albedo and roughness.
After this step, one can save these shading attributes, e.g., in a G-Buffer to perform lighting afterwards.
This routine also applies all material modifiers, like performs alpha test and applies a normal map.
*/
void _fn prepareShadingAttribs(in const MaterialData material, in vec3 P, in vec3 camPos,
    in vec3 normal, in vec3 tangent, in vec3 bitangent, in vec2 uv,
#ifdef _MS_USER_DERIVATIVES
    in const vec2 dPdx, in const vec2 dPdy,
#else
    in const float lodBias,
#endif
    _ref(ShadingAttribs) shAttr)
{
    /* Prepare shading attributes */
    shAttr.P = P;
    shAttr.E = normalize(camPos - P);
    shAttr.N = normalize(normal);
    shAttr.B = normalize(bitangent - shAttr.N * (dot(bitangent, shAttr.N)));
    shAttr.T = normalize(cross(shAttr.B, shAttr.N));
    shAttr.UV = uv;
#ifdef _MS_USER_DERIVATIVES
    shAttr.DPDX = dPdx;
    shAttr.DPDY = dPdy;
#else
    shAttr.lodBias = lodBias;
#endif


    /* Copy the input material parameters */
#ifdef _MS_STATIC_MATERIAL_DESC
    const MaterialDesc desc = _MS_STATIC_MATERIAL_DESC;
#else
    const MaterialDesc desc = material.desc;
#endif

    shAttr.preparedMat.values = material.values;
    shAttr.preparedMat.desc = desc;

    /* Evaluate alpha test material modifier */
    applyAlphaTest(shAttr.preparedMat, shAttr);
    shAttr.aoFactor = 1;

    /* Prefetch textures */
    FOR_MAT_LAYERS(iLayer, shAttr.preparedMat)
    {
        shAttr.preparedMat.values.layers[iLayer].albedo.constantColor =
            evalWithColor(desc.layers[iLayer].hasAlbedoTexture, material.values.layers[iLayer].albedo, shAttr);
        //ShAttr.PreparedMat.Layers[iLayer].Roughness.ConstantColor = EvalWithColor(Material.Layers[iLayer].Roughness, ShAttr);
        //ShAttr.PreparedMat.Layers[iLayer].ExtraParam.ConstantColor = EvalWithColor(Material.Layers[iLayer].ExtraParam, ShAttr);
    }

    /* Perturb shading normal is needed */
    perturbNormal(shAttr.preparedMat, shAttr);
}

/**
Another overload of PrepareShadingAttribs(), which does not require tangents.
Instead it constructs a world-space axis-aligned tangent frame on the fly.
*/
void _fn prepareShadingAttribs(in const MaterialData material, in vec3 P, in vec3 camPos,
    in vec3 normal, in vec2 uv,
#ifdef _MS_USER_DERIVATIVES
    in const vec2 dPdx, in const vec2 dPdy,
#else
    in const float lodBias,
#endif
    _ref(ShadingAttribs) shAttr)
{
    /* Generate an axis-aligned tangent frame */
    vec3 tangent; vec3 bitangent;
    createTangentFrame(normal, tangent, bitangent);

    prepareShadingAttribs(material, P, camPos, normal, tangent, bitangent, uv,
#ifdef _MS_USER_DERIVATIVES
        dPdx, dPdy,
#else
        lodBias,
#endif
        shAttr);
}

#ifndef _MS_USER_DERIVATIVES
// Legacy version of prepareShadingAttribs() without LOD bias
void _fn prepareShadingAttribs(in const MaterialData material, in vec3 P, in vec3 camPos, in vec3 normal, in vec2 uv, _ref(ShadingAttribs) shAttr)
{
    prepareShadingAttribs(material, P, camPos, normal, uv, 0, shAttr);
}

void _fn prepareShadingAttribs(in const MaterialData material, in vec3 P, in vec3 camPos, in vec3 normal, in vec3 tangent, in vec3 bitangent, in vec2 uv, _ref(ShadingAttribs) shAttr)
{
    prepareShadingAttribs(material, P, camPos, normal, tangent, bitangent, uv, 0, shAttr);
}
#endif

vec4 _fn evalEmissiveLayer(in const MaterialLayerValues layer, _ref(PassOutput) result)
{
    result.diffuseAlbedo += v3(1.f);
    result.diffuseIllumination += v3(layer.albedo.constantColor);
    return v4(1.f);
}

vec4 _fn evalDiffuseLayer(in const MaterialLayerValues layer, in const vec3 lightIntensity, in const vec3 lightDir, in const vec3 normal, _ref(PassOutput) result)
{
    vec3 value = lightIntensity;
    float weight = 0;
    vec4 albedo = layer.albedo.constantColor;
    result.roughness = v2(1.f);
#ifndef _MS_DISABLE_DIFFUSE
    value *= evalDiffuseBSDF(normal, lightDir);
    weight = albedo.w;
    result.diffuseAlbedo += v3(albedo) * layer.pmf;
    result.diffuseIllumination += value;
#else
    value = v3(0.f);
#endif
    return v4(value, weight);
}

#ifdef _MS_FILTER_ROUGHNESS
// Implementation of NDF filtering code from the HPG'16 submission
// The writeup is here: //research/graphics/projects/halfvectorSpace/SAA/paper/specaa-sub.pdf
vec2 _fn filterRoughness(in const ShadingAttribs shAttr, in const LightAttribs lAttr, in vec2 roughness)
{
#ifdef _MS_USER_HALF_VECTOR_DERIVATIVES
    vec2  hppDx = shAttr.DHDX;
    vec2  hppDy = shAttr.DHDY;
#else
    // Compute half-vector derivatives
    vec3  H = normalize(shAttr.E + lAttr.L);
    vec2  hpp = v2(dot(H, shAttr.T), dot(H, shAttr.B));
    vec2  hppDx = dFdx(hpp);
    vec2  hppDy = dFdy(hpp);
#endif
    // Compute filtering region
    vec2 rectFp = (abs(hppDx) + abs(hppDy)) * 0.5f;

    // For grazing angles where the first-order footprint goes to very high values
    // Usually you don’t need such high values and the maximum value of 1.0 or even 0.1 is enough for filtering.
    rectFp = min(v2(0.7f), rectFp);

    // Covariance matrix of pixel filter's Gaussian (remapped in roughness units)
    vec2 covMx = rectFp * rectFp * 2.f;   // Need to x2 because roughness = sqrt(2) * pixel_sigma_hpp

    roughness = sqrt(roughness*roughness + covMx);          // Beckmann proxy convolution for GGX

    return roughness;
}
#endif

vec4 _fn evalSpecularLayer(in const MaterialLayerDesc desc, in const MaterialLayerValues data, in const ShadingAttribs shAttr, in const LightAttribs lAttr, _ref(PassOutput) result)
{
#ifndef _MS_DISABLE_SPECULAR
    /* Add albedo regardless of facing */
    result.specularAlbedo += v3(data.albedo.constantColor) * data.pmf;

    /* Ignore the layer if it's a transmission or backfacing */
	if (dot(lAttr.L, shAttr.N)  * dot(shAttr.E, shAttr.N) <= 0.f)
        return v4(0.f);

    vec3 value = lAttr.lightIntensity;

    vec2 roughness;
    if(desc.hasRoughnessTexture != 0)
    {
        roughness = v2(data.albedo.constantColor.w, data.albedo.constantColor.w);
    }
    else
    {
        roughness = v2(data.roughness.constantColor.x, data.roughness.constantColor.y);
    }
#ifdef _MS_FILTER_ROUGHNESS
    roughness = filterRoughness(shAttr, lAttr, roughness);
#endif

    // Respect perfect specular cutoff
    if(max(roughness.x, roughness.y) < 1e-3f)
        return v4(0.f);

    // compute halfway vector
    const vec3 hW = normalize(shAttr.E + lAttr.L);
    const vec3 h = normalize(v3(dot(hW, shAttr.T), dot(hW, shAttr.B), dot(hW, shAttr.N)));

    result.roughness = roughness;

    switch(desc.ndf)
    {
    case NDFBeckmann: /* Beckmann microfacet distribution */
    {
        value *= evalBeckmannDistribution(h, roughness);
    }
    break;
    case NDFGGX: /* GGX */
    {
        value *= evalGGXDistribution(h, roughness);
    }
    break;
    }

    /* Evaluate standard microfacet model terms */
    value *= evalMicrofacetTerms(shAttr.T, shAttr.B, shAttr.N, h, shAttr.E, lAttr.L, roughness, desc.ndf, (desc.type) == MatDielectric);

    /* Cook-Torrance Jacobian */
    value /= 4.f * dot(shAttr.E, shAttr.N);

    /* Fresnel conductor/dielectric term */
    const float HoE = dot(hW, shAttr.E);
    const float IoR = data.extraParam.constantColor.x;
    const float kappa = data.extraParam.constantColor.y;
    const float F_term = (desc.type == MatConductor) ? conductorFresnel(HoE, IoR, kappa) : 1.f - dielectricFresnel(HoE, IoR);
    value *= F_term;
    float weight = F_term;

    result.specularIllumination += value;

    return v4(value, weight);
#else
    return v4(0);
#endif
}

vec3 _fn blendLayer(in const vec4 albedo, in const vec4 layerOut, in const uint blendType, in const vec3 currentValue)
{
    /* Account for albedo */
    vec3 scaledLayerOut = v3(layerOut) * v3(albedo);
    float weight = layerOut.w;

    /* Perform layer blending */
    if(blendType == BlendConstant)
    {
        weight = albedo.w;
    }

    vec3 result;
    if(blendType != BlendAdd)
    {
        result = mix(currentValue, scaledLayerOut, weight);
    }
    else
    {
        result = currentValue + scaledLayerOut;
    }
    return result;
}

/**
A helper routine for evaluating a single layer of a layered material, given a shading point and a point on a light source.
The routine takes a layer index, as well as shading attributes, including prepared layers of the material;
it also takes prepared attributes of a light source, such as incident radiance and a world-space direction to the light.
The output is the illumination of the current layer, blended into the results of the previous layers with a specified blending mode.
*/
void _fn evalMaterialLayer(in const int iLayer, in const ShadingAttribs attr, in const LightAttribs lAttr,
    _ref(PassOutput) result)
{
    vec4 value = v4(0.f);

    const MaterialLayerValues data = attr.preparedMat.values.layers[iLayer];
    const MaterialLayerDesc desc = attr.preparedMat.desc.layers[iLayer];
    switch(desc.type)
    {
    case MatLambert: /* Diffuse BRDF */
        value = evalDiffuseLayer(data, lAttr.lightIntensity, lAttr.L, attr.N, result);
        break;
    case MatEmissive:
        value = evalEmissiveLayer(data, result);
        break;
    case MatConductor:
    case MatDielectric:
        value = evalSpecularLayer(desc, data, attr, lAttr, result);
        break;
    };

	vec3 oldValue = result.value;
	result.value = blendLayer(data.albedo.constantColor, value, desc.blending, result.value);

	float delta = max(1e-3f, luminance(result.value - oldValue));
	result.effectiveRoughness += result.roughness * delta;
}


/**	The highest-level material evaluation function.
This is the main routing for evaluating a complete PBR material, given a shading point and a light source.
Should be called once per light.
*/
void _fn evalMaterial(
    in const ShadingAttribs shAttr,
    in const LightAttribs lAttr,
    _ref(ShadingOutput) result,
    in const bool initializeShadingOut DEFVAL(false))
{
    /* If it's the first pass, initialize all the aggregates to zero */
    if(initializeShadingOut)
    {
        result.diffuseAlbedo = v3(0.f);
        result.diffuseIllumination = v3(0.f);
        result.specularAlbedo = v3(0.f);
        result.specularIllumination = v3(0.f);
        result.finalValue = v3(0.f);
    }

    /* Go through all layers and perform a layer-by-layer shading and compositing */
    PassOutput passResult;
    passResult.value = v3(0.f);
    passResult.diffuseAlbedo = v3(0.f);
    passResult.diffuseIllumination = v3(0.f);
    passResult.specularAlbedo = v3(0.f);
    passResult.specularIllumination = v3(0.f);
    passResult.roughness = v2(0.f);
    passResult.effectiveRoughness = v2(0.f);
    FOR_MAT_LAYERS(iLayer, shAttr.preparedMat)
    {
        evalMaterialLayer(iLayer, shAttr, lAttr, passResult);
    }

    /* Accumulate the results of the pass */
    result.finalValue += passResult.value;
    result.effectiveRoughness += passResult.effectiveRoughness / max(1e-3f, luminance(passResult.value));
    result.diffuseIllumination += passResult.diffuseIllumination;
    result.specularIllumination += passResult.specularIllumination;
    result.diffuseAlbedo = passResult.diffuseAlbedo;
    result.specularAlbedo = passResult.specularAlbedo;
}

/**
Another overload of material evaluation function, which prepares light attributes internally.
*/
void _fn evalMaterial(
    in const ShadingAttribs shAttr,
    in const LightData light,
    _ref(ShadingOutput) result,
    in const bool initializeShadingOut DEFVAL(false))
{
    /* Prepare lighting attributes */
    LightAttribs LAttr;
    prepareLightAttribs(light, shAttr, LAttr);

    /* Evaluate material with lighting attributes */
    evalMaterial(shAttr, LAttr, result, initializeShadingOut);
}

/*******************************************************************
Material building helpers
*******************************************************************/

/**
Initializes a material layer with an empty layer
*/
void _fn initNullLayer(_ref(MaterialLayerDesc) layer)
{
    layer.type = MatNone;
}

/**
Initializes a material layer with diffuse BRDF
*/
void _fn initDiffuseLayer(_ref(MaterialLayerDesc) desc, _ref(MaterialLayerValues) data, in const vec3 albedo)
{
    desc.type = MatLambert;
    desc.blending = BlendAdd;
    data.albedo.constantColor = v4(albedo, 1.f);
}

/**
Initializes a material layer with conductor BRDF. IoR and Kappa are set to the values of chrome by default.
*/
void _fn initConductorLayer(_ref(MaterialLayerDesc) desc, _ref(MaterialLayerValues) data, in const vec3 color, in const float roughness, in const float IoR = 3.f, in const float kappa = 4.2f)
{
    desc.type = MatConductor;
    desc.blending = BlendAdd;
    data.albedo.constantColor = v4(color, 1.f);
    data.roughness.constantColor = v4(roughness);
    data.extraParam.constantColor.x = IoR;
    data.extraParam.constantColor.y = kappa;
}

/**
Initializes a material layer with dielectric BRDF.
*/
void _fn initDielectricLayer(_ref(MaterialLayerDesc) desc, _ref(MaterialLayerValues) data, in const vec3 color, in const float roughness, in const float IoR)
{
    desc.type = MatDielectric;
    desc.blending = BlendFresnel;
    data.albedo.constantColor = v4(color, 1.f);
    data.roughness.constantColor = v4(roughness);
    data.extraParam.constantColor.x = IoR;
}

/*******************************************************************
Simple material helpers
*******************************************************************/

/**
Tries to find a layer data for a given material type (diffuse/conductor/etc).
\param[in] material         Material to look in
\param[in] layerType        Material layer Type (MatLambert/MatConductor etc.)
\param[out] data           Layer data, if found
returns false if the layer is not found, true otherwise
*/
bool _fn getLayerByType(in const MaterialData material, in const int layerType, _ref(MaterialLayerValues) data, _ref(MaterialLayerDesc) desc)
{
    int layerId = material.desc.layerIdByType[layerType].id;
    if(layerId != -1)
    {
        data = material.values.layers[layerId];
        desc = material.desc.layers[layerId];
        return true;
    }
    return false;
}

/**
Tries to find a diffuse albedo color for a given material
\param[in] Material Material to look in
returns black if the layer is not found, diffuse albedo color otherwise
*/
vec4 _fn getDiffuseColor(in const ShadingAttribs shAttr)
{
    vec4 ret = v4(0.f);
    MaterialLayerValues data;
    MaterialLayerDesc   desc;
    if(getLayerByType(shAttr.preparedMat, MatLambert, data, desc))
    {
        ret = data.albedo.constantColor;
    }
    return ret;
}

/**
Tries to override a diffuse albedo color for all layers within a given material
\param[in] Material Material to look in
returns true if succeeded
*/
bool _fn overrideDiffuseColor(_ref(ShadingAttribs) shAttr, const vec4 albedo, const bool allLayers = true)
{
    bool found = false;
    if(!allLayers)
    {
        int layerId = shAttr.preparedMat.desc.layerIdByType[MatLambert].id;
        if(layerId != -1)
        {
            found = true;
            shAttr.preparedMat.values.layers[layerId].albedo.constantColor = albedo;
        }
    }
    else
    {
        FOR_MAT_LAYERS(iLayer, shAttr.preparedMat)
        {
            if(shAttr.preparedMat.desc.layers[iLayer].type == MatLambert)
            {
                shAttr.preparedMat.values.layers[iLayer].albedo.constantColor = albedo;
                found = true;
            }
        }
    }
    return found;
}

/**
Tries to find a specular albedo color for a given material
\param[in] material Material to look in
returns black if the layer is not found, specular color otherwise
*/
vec4 _fn getSpecularColor(in const ShadingAttribs shAttr)
{
    vec4 ret = v4(0.f);
    MaterialLayerValues data;
    MaterialLayerDesc   desc;
    if(getLayerByType(shAttr.preparedMat, MatConductor, data, desc))
    {
        ret = data.albedo.constantColor;
    }
    return ret;
}

/**
    This routine importance samples the BRDF for path tracer next event
    estimation. A new direction is chosen based on the material properties.

    \param[in] shAttr Complete information about the shading point
    \param[in] rSample Random numbers for sampling
    \param[out] result Gather incident direction, probability density function, and path throughput
*/
void _fn sampleMaterial(
	in const ShadingAttribs shAttr,
	in vec2 rSample,
	_ref(ShadingOutput) result)
{
	bool sampleDiffuse = true;

	// Set the initial path throughput
	result.thp = v3(0.f);
    result.diffuseAlbedo = v3(0.f);
    result.specularAlbedo = v3(0.f);

	// Sample specular layer if it exists
	MaterialLayerValues specData;
    MaterialLayerDesc   specDesc;
    if(getLayerByType(shAttr.preparedMat, MatConductor, specData, specDesc))
	{
		// Randomly sample the specular component by using the probability mass function
		if (rSample.x < specData.pmf)
		{
            // Rescale random number value
            rSample.x /= specData.pmf;

			sampleDiffuse = false;

			vec3 m;
			const vec3 wo = toLocal(shAttr.E, shAttr.T, shAttr.B, shAttr.N);
			const vec2 roughness = v2(specData.roughness.constantColor.x, specData.roughness.constantColor.y);

			// Set the specular reflectivity
			result.specularAlbedo = v3(specData.albedo.constantColor);
            result.thp = result.specularAlbedo;

			// Choose the appropriate normal distribution function
            switch(specDesc.ndf)
			{
			case NDFBeckmann: // Beckmann normal distribution function 
			{
				result.thp *= sampleBeckmannDistribution(wo, roughness, rSample, m, result.wi, result.pdf);
			}
			break;

			case NDFGGX: // GGX normal distribution function
			{
				result.thp *= sampleGGXDistribution(wo, roughness, rSample, m, result.wi, result.pdf);
			}
			break;
			}

            // Convert direction vector from local to global frame
            // global space coordinates
            result.wi = fromLocal(result.wi, shAttr.T, shAttr.B, shAttr.N);

			// Evaluate standard microfacet model terms
            result.thp *= evalMicrofacetTerms(shAttr.T, shAttr.B, shAttr.N, m, shAttr.E, result.wi, roughness, specDesc.ndf, (specDesc.type) == MatDielectric);

			// Fresnel conductor/dielectric term
			const float HoE = dot(m, shAttr.E);
			const float IoR = specData.extraParam.constantColor.x;
			const float kappa = specData.extraParam.constantColor.y;
            const float F_term = (specDesc.type == MatConductor) ? conductorFresnel(HoE, IoR, kappa) : 1.f - dielectricFresnel(HoE, IoR);
			result.thp *= F_term;

			result.effectiveRoughness = roughness;
		}
        else
        {
            // Rescale random number value
            rSample.x -= specData.pmf;
            rSample.x /= 1.f - specData.pmf;
        }
	}

	// Sample the diffuse component
	if (sampleDiffuse)
	{
		// Cosine lobe sampling
        result.wi = cosine_sample_hemisphere(rSample.x, rSample.y);

        // Convert direction vector from local to global frame
        // global space coordinates
        result.wi = fromLocal(result.wi, shAttr.T, shAttr.B, shAttr.N);

		// Ideally thp = (\rho / \pi) * |\omega_i . n| / bsdfPdf
		// By importance sampling the cosine lobe, we can set bsdfPdf = |\omega_i . n| / \pi
		// Thus, we can simplify thp = \rho
        result.diffuseAlbedo = v3(getDiffuseColor(shAttr));
		result.thp = result.diffuseAlbedo;

		// Probability density function for perfect importance sampling of cosine lobe 
		result.pdf = M_1_PIf;

		// Record roughness for diffuse brdf
		result.effectiveRoughness = v2(1.f);
	}
}

#endif	// _FALCOR_SHADING_H_
