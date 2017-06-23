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

#ifndef _FALCOR_LIGHTS_H_
#define _FALCOR_LIGHTS_H_

/*******************************************************************
					Lights
*******************************************************************/

/**
	This routine computes the position of the the light based on the point 'shadingPosition'.
*/
inline vec3 _fn getLightPos(in const LightData Light, in const vec3 shadingPosition)
{
    vec3 lightPos = Light.worldPos;
    if(Light.type == LightArea)
    {
        lightPos = mul(Light.transMat, v4(lightPos, 1.0)).xyz;
    }    
    else if(Light.type == LightDirectional)
    {
        float dist = length(shadingPosition - lightPos);
        lightPos = shadingPosition - Light.worldDir * dist;
    }
    return lightPos;
}

/**
	This routine computes the radiance from the light at the point 'shadingPosition'.
*/
inline vec3 _fn getLightRadiance(in const LightData Light, in const vec3 shadingPosition)
{
    vec3 lightRadiance = Light.intensity;
    if(Light.type == LightPoint || Light.type == LightArea)
    {
        // TODO: add point on the light for area lights
        vec3 lightDir = shadingPosition - getLightPos(Light, shadingPosition);
        lightRadiance *= 1.0f / (4.0f * M_PIf);    // per steradian
        lightRadiance /= dot(lightDir, lightDir);        // per square meter
    }
    return lightRadiance;
}

/**
	This routine prepares attributes for shading a point with a particular light source.
	The outputs are an incident radiance towards the shading point 
	and the direction from the shading point towards the light source.
*/
inline void _fn prepareLightAttribs(in const LightData Light, in const ShadingAttribs ShAttr, float shadowFactor, _ref(LightAttribs) LightAttr)
{
	/* Evaluate direction to the light */
    LightAttr.P = getLightPos(Light, ShAttr.P);
    LightAttr.pdf = 0;
    LightAttr.N = 0;
    LightAttr.shadowFactor = shadowFactor;

    [unroll]
    for(uint i = 0 ; i < 4 ; i++)
    {
        LightAttr.points[i] = 0;
    }
    vec3 PosToLight = LightAttr.P - ShAttr.P;
    if(dot(PosToLight, PosToLight) > 1e-3f)
    {
	    LightAttr.L = normalize(PosToLight);
    }
    else
    {
        LightAttr.L = 0;
    }
	LightAttr.lightIntensity = Light.intensity;
    if(Light.type == LightDirectional)
    {
		LightAttr.L = -Light.worldDir;
    }
    else if(Light.type == LightArea || Light.type == LightPoint)
	{
		/* Evaluate various attenuation factors: cosine, 1/r^2, etc. */
		float Atten = 1.f;

		const float cosTheta = -dot(LightAttr.L, Light.worldDir);	// cos of angle of light orientation with outgoing direction
        if(Light.type == LightArea)			// Cosine attenuation
        {
			Atten = max(0.f, cosTheta) * Light.surfaceArea;
        }
        else if(Light.type == LightPoint)
		{
			// Spot light cone angle
			if(cosTheta < Light.cosOpeningAngle)
				Atten = 0.f;
			if(Light.penumbraAngle > 0.f)	// Compute cone attenuation of a spot light
			{
				float deltaAngle = Light.openingAngle - acos(cosTheta);
				Atten *= clamp((deltaAngle - Light.penumbraAngle) / Light.penumbraAngle, 0.f, 1.f);
			}
		}

		// Quadratic attenuation
        Atten /= max(1e-3f, dot(PosToLight, PosToLight));

		LightAttr.lightIntensity *= Atten;
	}
    // DISABLED_FOR_D3D12
#if 0
	if (Light.type == LightArea)
	{
		for (int index = 0; index < 4; index++)
		{
			// Access the geometry buffers
#ifdef CUDA_CODE
			optix::bufferId<vec3, 1> vertices(Light.vertexPtr.ptr);
			// Get vertices pointed by the corresponding index
			vec3 p0 = vertices[index];
#else
			float* vertices = (float*)(Light.vertexPtr.ptr);
			// Get vertices pointed by the corresponding index
			vec3 p0 = vec3(vertices[index * 3 + 0], vertices[index * 3 + 1], vertices[index * 3 + 2]);
#endif
			// Apply model instance transformation matrix
			LightAttr.points[index] = v3(mul(Light.transMat, v4(p0, 1.0)));
		}
	}
#endif
}

/**
    This routine samples the light source.
*/
void _fn sampleLight(in const vec3 shadingHitPos, in const LightData lData, const vec3 rSample, _ref(LightAttribs) lAttr)
{
	// Sample the light based on its type: point, directional, or area
	switch (lData.type)
	{
	    case LightPoint:
	    {
			// Get the position
            lAttr.P = getLightPos(lData, shadingHitPos);

			vec3 PosToLight = lAttr.P - shadingHitPos;
            float lDist = length(PosToLight);
            lAttr.L = PosToLight / max(1e-3f, lDist);

			// For point light, its normal is always along the L direction
			lAttr.N = lAttr.L;

			// Compute the intensity and the PDF
			lAttr.lightIntensity = getLightRadiance(lData, shadingHitPos);
			lAttr.pdf = 1.f;
		}
	    break;

		case LightDirectional:
		{
			// Get the position
			lAttr.P = getLightPos(lData, shadingHitPos);
			lAttr.L = -lData.worldDir;

			// For directional light, its normal is always along the L direction
			lAttr.N = lAttr.L;
			
			// Compute the intensity and the PDF
			lAttr.lightIntensity = lData.intensity;
			lAttr.pdf = 1.f;
		}
		break;

#if defined(CUDA_CODE) || defined(FALCOR_GLSL)
		case LightArea:
		{
			if (lData.numIndices != 0)
			{
				// Randomly pick a triangle on the mesh
				// TODO: Pick a triangle mesh based on probability distributions
				int index = min((int)(rSample.z * lData.numIndices), (int)(lData.numIndices - 1));

				// Access the geometry buffers
#ifdef CUDA_CODE
				optix::bufferId<int3, 1> indices(lData.indexPtr.ptr);
				optix::bufferId<vec3, 1> vertices(lData.vertexPtr.ptr);
				// Retrieve indices
				const ivec3 pId = indices[index];
				// Get vertices pointed by the corresponding index
				vec3 p0 = vertices[pId.x];
				vec3 p1 = vertices[pId.y];
				vec3 p2 = vertices[pId.z];
#else
				int* indices = (int*)(lData.indexPtr.ptr);
				float* vertices = (float*)(lData.vertexPtr.ptr);
				// Retrieve indices
				const ivec3 pId = ivec3(indices[index * 3 + 0], indices[index * 3 + 1], indices[index * 3 + 2]);
				// Get vertices pointed by the corresponding index
				vec3 p0 = vec3(vertices[pId.x * 3 + 0], vertices[pId.x * 3 + 1], vertices[pId.x * 3 + 2]);
				vec3 p1 = vec3(vertices[pId.y * 3 + 0], vertices[pId.y * 3 + 1], vertices[pId.y * 3 + 2]);
				vec3 p2 = vec3(vertices[pId.z * 3 + 0], vertices[pId.z * 3 + 1], vertices[pId.z * 3 + 2]);
#endif

				// Apply model instance transformation matrix
				p0 = v3(mul(lData.transMat, v4(p0, 1.0)));
				p1 = v3(mul(lData.transMat, v4(p1, 1.0)));
				p2 = v3(mul(lData.transMat, v4(p2, 1.0)));

				// Sample a point on the triangle mesh using barycentric coordinates
				float a = sqrt(rSample.x);
				const vec2 bary = v2(1.f - a, a * rSample.y);

				lAttr.P = p0 * bary.x + p1 * bary.y + p2 * (1.f - bary.x - bary.y);

				lAttr.N = normalize(cross(p1 - p0, p2 - p0));
			}

			vec3 PosToLight = lAttr.P - shadingHitPos;
            float lDist = length(PosToLight);
            lAttr.L = PosToLight / max(1e-3f, lDist);

			lAttr.lightIntensity = lData.intensity;

			// Compute the PDF
			lAttr.pdf = lDist * lDist / (abs(dot(lAttr.N, lAttr.L)) * lData.surfaceArea);

			// Set light's contribution
			if (lAttr.pdf > 0.f && dot(lAttr.N, lAttr.L) < 0.f)
				lAttr.lightIntensity /= lAttr.pdf;
			else
				lAttr.lightIntensity = v3(0.f);
		}
		break;
#endif
	}
}

/**
This routine samples a rectangular area light source in a stratified way.
*/
void _fn stratifiedSampleRectangularAreaLight(in const vec3 shadingHitPos, in const LightData lData, const vec2 rSample, int x, int y, int numStrataX, int numStrataY, _ref(LightAttribs) lAttr)
{
	if (lData.type != LightArea)
		return;

	// Transform light position and light normal
	vec3 lightPos = mul(lData.transMat, v4(lData.worldPos, 1.0f)).rgb;
	lAttr.N = mul(lData.transMat, v4(lData.worldDir, 0.0f)).rgb;

	// Transform tangent frame
	vec3 transformedTangent = mul(lData.transMat, v4(lData.tangent, 0.0f)).rgb;
	vec3 transformedBitangent = mul(lData.transMat, v4(lData.bitangent, 0.0f)).rgb;

	float lightSizeY1 = length(transformedTangent);
	float lightSizeY2 = length(transformedBitangent);

	// Generate stratified samples
	float y1 = ((x + rSample.x) / float(numStrataX) - 0.5f) * lightSizeY1;
	float y2 = ((y + rSample.y) / float(numStrataY) - 0.5f) * lightSizeY2;

	lAttr.P = lightPos + y1 * normalize(transformedTangent) + y2 * normalize(transformedBitangent);

	vec3 PosToLight = lAttr.P - shadingHitPos;
	float lDist = length(PosToLight);
	lAttr.L = PosToLight / max(1e-3f, lDist);

	lAttr.lightIntensity = lData.intensity;

	// Compute the PDF
	lAttr.pdf = lDist * lDist / (abs(dot(lAttr.N, lAttr.L)) * lData.surfaceArea);

	// Set light's contribution
    if (lAttr.pdf > 0.f && dot(lAttr.N, lAttr.L) < 0.f)
        lAttr.lightIntensity /= lAttr.pdf;
    else
        lAttr.lightIntensity = 0;
}

#endif	// _FALCOR_LIGHTS_H_
