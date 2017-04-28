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
#ifndef _HOST_DEVICE_DATA_H
#define _HOST_DEVICE_DATA_H

/*******************************************************************
                    Common structures & routines
*******************************************************************/


/*******************************************************************
                    Glue code for CPU/GPU compilation
*******************************************************************/

#if (defined(__STDC_HOSTED__) || defined(__cplusplus)) && !defined(__CUDACC__)    // we're in C-compliant compiler, probably host
#    define HOST_CODE 1
#elif defined(__CUDACC__)
#   define CUDA_CODE
#else
#   define HLSL_CODE
#endif

#ifdef HLSL_CODE
//#extension GL_NV_shader_buffer_load : enable
#endif

#ifdef HOST_CODE

namespace Falcor {
/*******************************************************************
                    CPU declarations
*******************************************************************/
    class Sampler;
    class Texture;
#define loop_unroll
#define v2 vec2
#define v3 vec3
#define v4 vec4
#define _fn
#define DEFAULTS(x_) = ##x_
#define SamplerState std::shared_ptr<Sampler>
#define Texture2D std::shared_ptr<Texture>
#elif defined(CUDA_CODE)
/*******************************************************************
                    CUDA declarations
*******************************************************************/
// Modifiers
#define DEFAULTS(x_)
#define in
#define out &
#define _ref(__x) __x&
#define discard
#define sampler2D int
#define inline __forceinline
#define _fn inline __device__
// Types
#define int32_t int
#define uint unsigned int
#define uint32_t uint
// Vector math
#define vec2 float2
#define vec3 float3
#define vec4 float4
typedef float mat4_t[16];
#ifndef mat4
#define mat4 mat4_t
#endif
typedef float mat3_t [12];
#ifndef mat3
#define mat3 mat3_t
#endif
#define mul(mx, v) ((v) * (mx))
_fn float clamp(float t, float mn, float mx) { return fminf(mx, fmaxf(mn, t)); }
_fn vec2 clamp(vec2 t, vec2 mn, vec2 mx) { return fminf(mx, fmaxf(mn, t)); }
#define v2 make_float2
#define v3 make_float3
#define v4 make_float4
_fn float dot(const vec2& a, const vec2& b) { return a.x*b.x + a.y*b.y; }
_fn float dot(const vec3& a, const vec3& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
_fn float dot(const vec4& a, const vec4& b) { return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w; }
_fn vec3 cross(const vec3& x, const vec3& y) {
    return v3(
        x.y * y.z - y.y * x.z,
        x.z * y.x - y.z * x.x,
        x.x * y.y - y.x * x.y);
}
_fn float length(const vec3& a) { return sqrt(dot(a, a)); }
_fn float length(const vec2& a) { return sqrt(dot(a, a)); }
_fn vec3 normalize(const vec3& a) { return a / length(a); }
_fn vec3 mix(const vec3& a, const vec3& b, const float w) { return a + w * (b - a); }
_fn vec2 sqrt(const vec2& a) { return v2(sqrt(a.x), sqrt(a.y)); }
// Texture access
_fn bool isSamplerBound(sampler2D sampler) { return sampler > 0; }
_fn vec4 texture2D(sampler2D sampler, vec2 uv) { return tex2D<float4>(sampler, uv.x, uv.y); }
_fn vec4 textureLod(sampler2D sampler, vec2 uv, float lod) { return tex2DLod<float4>(sampler, uv.x, uv.y, lod); }
_fn vec4 textureGrad(sampler2D sampler, vec2 uv, vec2 dPdx, vec2 dPdy) { return tex2DGrad<float4>(sampler, uv.x, uv.y, dPdx, dPdy); }
_fn vec4 textureBias(sampler2D sampler, vec2 uv, float bias) { return texture2D(sampler, uv); }
struct TexPtr
{
    int            ptr;
    uint        pad[7];
};
typedef TexPtr BufPtr;
#else
/*******************************************************************
                    HLSL declarations
*******************************************************************/
#define loop_unroll [unroll]
#define _fn 
#define __device__ 
typedef float2 vec2;
typedef float3 vec3;
typedef float4 vec4;
typedef float3x3 mat3;
typedef float4x4 mat4;
typedef uint uint32_t;
typedef int int32_t;
typedef vec2 v2;
typedef vec3 v3;
typedef vec4 v4;
#define inline 
#define _ref(__x) inout __x
#define DEFAULTS(x_)
#endif

/*******************************************************************
                    Lights
*******************************************************************/

/**
    Types of light sources. Used in LightData structure.
*/
#define LightPoint           0    ///< Point light source, can be a spot light if its opening angle is < 2pi
#define LightDirectional     1    ///< Directional light source
#define LightArea            2    ///< Area light source, potentially with arbitrary geometry
//#define LightVolume        3    ///< Volumetric light source

/*******************************************************************
Camera
*******************************************************************/
/**
This is a general host/device structure that describe a camera.
*/
struct CameraData
{
    mat4            viewMat                DEFAULTS(mat4());                ///< Camera view matrix.
    mat4            projMat                DEFAULTS(mat4());                ///< Camera projection matrix.
    mat4            viewProjMat            DEFAULTS(mat4());                ///< Camera view-projection matrix.
    mat4            invViewProj            DEFAULTS(mat4());                ///< Camera inverse view-projection matrix.
    mat4            prevViewProjMat        DEFAULTS(mat4());                ///< Camera view-projection matrix associated to previous frame.

    vec3            position               DEFAULTS(vec3(0, 0, 0));         ///< Camera world-space position.
    float           focalLength            DEFAULTS(21.0f);                 ///< Camera focal length in mm. Default is 59 degree vertical, 90 horizontal FOV at 16:9 aspect ratio.
    vec3            up                     DEFAULTS(vec3(0, 1, 0));         ///< Camera world-space up vector.
    float           aspectRatio            DEFAULTS(1.f);                   ///< Camera aspect ratio.
    vec3            target                 DEFAULTS(vec3(0, 0, -1));        ///< Camera target point in world-space.
    float           nearZ                  DEFAULTS(0.1f);                  ///< Camera near plane.
    vec3            cameraU                DEFAULTS(vec3(0, 0, 1));         ///< Camera base vector U. normalized it indicates the left image plane vector. The length is dependent on the FOV. 
    float           farZ                   DEFAULTS(10000.0f);              ///< Camera far plane.
    vec3            cameraV                DEFAULTS(vec3(0, 1, 0));         ///< Camera base vector V. normalized it indicates the up image plane vector. The length is dependent on the FOV. 
    float           jitterX                DEFAULTS(0.0f);                  ///< Eventual camera jitter in the x coordinate
    vec3            cameraW                DEFAULTS(vec3(1, 0, 0));         ///< Camera base vector U. normalized it indicates the forward direction. The length is the camera focal distance.
    float           jitterY                DEFAULTS(0.0f);                  ///< Eventual camera jitter in the y coordinate

    mat4            rightEyeViewMat;
    mat4            rightEyeProjMat;
    mat4            rightEyeViewProjMat;
    mat4            rightEyePrevViewProjMat;
};
/*******************************************************************
                    Material
*******************************************************************/

/** Type of the material layer:
    Diffuse (Lambert model, can be Oren-Nayar if roughness is not 1),
    Reflective material (conductor),
    Refractive material (dielectric)
*/
#define     MatNone            0            ///< A "null" material. Used to end the list of layers
#define     MatLambert         1            ///< A simple diffuse Lambertian BRDF layer
#define     MatConductor       2            ///< A conductor material, metallic reflection, no refraction nor subscattering
#define     MatDielectric      3            ///< A refractive dielectric material, if applied on top of others acts like a coating
#define     MatEmissive        4            ///< An emissive material. Can be assigned to a geometry to create geometric a light source (will be supported only with ray tracing)
#define     MatUser            5            ///< User-defined material, should be parsed and processed by user
#define     MatNumTypes        (MatUser+1)  ///< Number of material types

/** Type of used Normal Distribution Function (NDF). Options so far
    Beckmann distribution (original Blinn-Phong)
    GGX distribution (smoother highlight, better fit for some materials, default)
*/
#define     NDFBeckmann        0    ///< Beckmann distribution for NDF
#define     NDFGGX             1    ///< GGX distribution for NDF
#define     NDFUser            2    ///< User-defined distribution for NDF, should be processed by user

#define     BlendFresnel       0    ///< Material layer is blended according to Fresnel
#define     BlendConstant      1    ///< Material layer is blended according to a constant factor stored in w component of constant color
#define     BlendAdd           2    ///< Material layer is added to the previous layers

/**
    This number specifies a maximum possible number of layers in a material.
    There seems to be a good trade-off between performance and flexibility.
    With three layers, we can represent e.g. a base conductor material with diffuse component, coated with a dielectric.
    If this number is changed, the scene serializer should make sure the new number of layers is saved/loaded correctly.
*/
#define     MatMaxLayers    3

/**
    A description for a single material layer.
    Contains information about underlying BRDF, NDF, and rules for blending with other layers.
    Also contains material properties, such as albedo and roughness.
*/
#define ROUGHNESS_CHANNEL_BIT 2
struct MaterialLayerDesc
{
    uint32_t    type            DEFAULTS(MatNone);             ///< Specifies a material Type: diffuse/conductor/dielectric/etc. None means there is no material
    uint32_t    ndf             DEFAULTS(NDFGGX);              ///< Specifies a model for normal distribution function (NDF): Beckmann, GGX, etc.
    uint32_t    blending        DEFAULTS(BlendAdd);            ///< Specifies how this layer should be combined with previous layers. E.g., blended based on Fresnel (useful for dielectric coatings), or just added on top, etc.
    uint32_t    hasTexture      DEFAULTS(0);                   ///< Specifies whether or not the material has textures. For dielectric and conductor layers this has special meaning - if the 2nd bit is on, it means we have roughness channel
};

struct MaterialLayerValues
{
    vec4     albedo;                                       ///< Material albedo/specular color/emitted color
    vec4     roughness;                                    ///< Material roughness parameter [0;1] for NDF
    vec4     extraParam;                                   ///< Additional user parameter, can be IoR for conductor and dielectric
    vec3     pad             DEFAULTS(vec3(0, 0, 0));
    float    pmf             DEFAULTS(0.f);                 ///< Specifies the current value of the PMF of all layers. E.g., first layer just contains a probability of being selected, others accumulate further
};

/**
    The auxiliary structure that provides the first occurrence of the layer by its type.
*/
struct LayerIdxByType
{
    vec3 pad;               // This is here due to HLSL alignment rules
    int32_t id DEFAULTS(-1);
};

/**
    The main material description structure. Contains a dense list of layers. The layers are stored from inner to outer, ending with a MatNone layer.
    Besides, the material contains its scene-unique id, as well as various modifiers, like normal/displacement map and alpha test map.
*/
struct MaterialDesc
{
    MaterialLayerDesc   layers[MatMaxLayers];     // First one is a terminal layer, usually either opaque with coating, or dielectric; others are optional layers, usually a transparent dielectric coating layer or a mixture with conductor
    uint32_t            hasAlphaMap     DEFAULTS(0);
    uint32_t            hasNormalMap    DEFAULTS(0);
    uint32_t            hasHeightMap    DEFAULTS(0);
    uint32_t            hasAmbientMap   DEFAULTS(0);
    LayerIdxByType      layerIdByType[MatNumTypes];             ///< Provides a layer idx by its type, if there is no layer of this type, the idx is -1
};

struct MaterialValues
{
    MaterialLayerValues layers[MatMaxLayers];
    vec2  height;                        // Height (displacement) map modifier (scale, offset). If texture is non-null, one can apply a displacement or parallax mapping
    float alphaThreshold DEFAULTS(1.0f); // Alpha test threshold, in cast alpha-test is enabled (alphaMap is not nullptr)
    int32_t id           DEFAULTS(-1);   // Scene-unique material id, -1 is a wrong material
};

struct MaterialTextures
{
    Texture2D layers[3];        // A single texture per layer
    Texture2D alphaMap;         // Alpha test parameter, if texture is non-null, alpha test is enabled, alpha threshold is stored in the constant color
    Texture2D normalMap;        // Normal map modifier, if texture is non-null, shading normal is perturbed
    Texture2D heightMap;        // Height (displacement) map modifier, if texture is non-null, one can apply a displacement or parallax mapping
    Texture2D ambientMap;       // Ambient occlusion map
};

struct MaterialData
{
    MaterialDesc desc;
    MaterialValues values;
    MaterialTextures textures;
    SamplerState samplerState;  // The sampler state to use when sampling the object
};

/**
    The structure stores the complete information about the shading point,
    except for a light source information.
    It stores pre-evaluated material parameters with pre-fetched textures,
    shading point position, normal, viewing direction etc.
*/
struct ShadingAttribs
{
    vec3    P;                                  ///< Shading hit position in world space
    vec3    E;                                  ///< Direction to the eye at shading hit
    vec3    N;                                  ///< Shading normal at shading hit
    vec3    T;                                  ///< Shading tangent at shading hit
    vec3    B;                                  ///< Shading bitangent at shading hit
    vec2    UV;                                 ///< Texture mapping coordinates

#ifdef _MS_USER_DERIVATIVES
    vec2    DPDX            DEFAULTS(v2(0, 0));                                  
    vec2    DPDY            DEFAULTS(v2(0, 0)); ///< User-provided 2x2 full matrix of duv/dxy derivatives of a shading point footprint in texture space
#else
    float   lodBias         DEFAULTS(0);        ///< LOD bias to use when sampling textures
#endif

#ifdef _MS_USER_HALF_VECTOR_DERIVATIVES
    vec2    DHDX            DEFAULTS(v2(0, 0));
    vec2    DHDY            DEFAULTS(v2(0, 0));  ///< User-defined half-vector derivatives
#endif
    MaterialData preparedMat;                   ///< Copy of the original material with evaluated parameters (i.e., textures are fetched etc.)
    float aoFactor;
};

/*******************************************************************
                    Lights
*******************************************************************/

/**
    This is a general host/device structure that describe a light source.
*/
struct LightData
{
    vec3            worldPos           DEFAULTS(v3(0, 0, 0));     ///< World-space position of the center of a light source
    uint32_t        type               DEFAULTS(LightPoint);      ///< Type of the light source (see above)
    vec3            worldDir           DEFAULTS(v3(0, -1, 0));    ///< World-space orientation of the light source
    float           openingAngle       DEFAULTS(3.14159265f);     ///< For point (spot) light: Opening angle of a spot light cut-off, pi by default - full-sphere point light
    vec3            intensity          DEFAULTS(v3(1, 1, 1));     ///< Emitted radiance of th light source
    float           cosOpeningAngle    DEFAULTS(-1.f);            ///< For point (spot) light: cos(openingAngle), -1 by default because openingAngle is pi by default
    vec3            aabbMin            DEFAULTS(v3(1e20f));       ///< For area light: minimum corner of the AABB
    float           penumbraAngle      DEFAULTS(0.f);             ///< For point (spot) light: Opening angle of penumbra region in radians, usually does not exceed openingAngle. 0.f by default, meaning a spot light with hard cut-off
    vec3            aabbMax            DEFAULTS(v3(-1e20f));      ///< For area light: maximum corner of the AABB
    float           surfaceArea        DEFAULTS(0.f);             ///< Surface area of the geometry mesh
	vec3            tangent            DEFAULTS(vec3());          ///< Tangent vector of the geometry mesh
	uint32_t        numIndices         DEFAULTS(0);               ///< Number of triangle indices in a polygonal area light
	vec3            bitangent          DEFAULTS(vec3());          ///< BiTangent vector of the geometry mesh
	float           pad;
    mat4            transMat           DEFAULTS(mat4());          ///< Transformation matrix of the model instance for area lights

    // For area light
// 	BufPtr          indexPtr;                                     ///< Buffer id for indices
// 	BufPtr          vertexPtr;                                    ///< Buffer id for vertices
// 	BufPtr          texCoordPtr;                                  ///< Buffer id for texcoord
// 	BufPtr          meshCDFPtr;                                   ///< Pointer to probability distributions of triangle meshes

    /*HACK: Spire can't hanlde this
    // Keep that last
    MaterialData    material;                                     ///< Emissive material of the geometry mesh
    */
};

/*******************************************************************
                    Shared material routines
*******************************************************************/


/** Converts specular power to roughness. Note there is no "the conversion".
    Reference: http://simonstechblog.blogspot.com/2011/12/microfacet-brdf.html
    \param shininess specular power of an obsolete Phong BSDF
*/
inline float _fn convertShininessToRoughness(const float shininess)
{
    return clamp(sqrt(2.0f / (shininess + 2.0f)), 0.f, 1.f);
}

inline vec2 _fn convertShininessToRoughness(const vec2 shininess)
{
    return clamp(sqrt(2.0f / (shininess + 2.0f)), 0.f, 1.f);
}

inline float _fn convertRoughnessToShininess(const float a)
{
    return 2.0f / clamp(a*a, 1e-8f, 1.f) - 2.0f;
}

inline vec2 _fn convertRoughnessToShininess(const vec2 a)
{
    return 2.0f / clamp(a*a, 1e-8f, 1.f) - 2.0f;
}

/*******************************************************************
Other helpful shared routines
*******************************************************************/


/** Returns a relative luminance of an input linear RGB color in the ITU-R BT.709 color space
\param RGBColor linear HDR RGB color in the ITU-R BT.709 color space
*/
inline float _fn luminance(const vec3 rgb)
{
    return dot(rgb, v3(0.2126f, 0.7152f, 0.0722f));
}

/** Converts color from RGB to YCgCo space
\param RGBColor linear HDR RGB color
*/
inline vec3 _fn RGBToYCgCo(const vec3 rgb)
{
    const float Y = dot(rgb, v3(0.25f, 0.50f, 0.25f));
    const float Cg = dot(rgb, v3(-0.25f, 0.50f, -0.25f));
    const float Co = dot(rgb, v3(0.50f, 0.00f, -0.50f));

    return v3(Y, Cg, Co);
}

/** Converts color from YCgCo to RGB space
\param YCgCoColor linear HDR YCgCo color
*/
inline vec3 _fn YCgCoToRGB(const vec3 YCgCo)
{
    const float tmp = YCgCo.x - YCgCo.y;
    const float r = tmp + YCgCo.z;
    const float g = YCgCo.x + YCgCo.y;
    const float b = tmp - YCgCo.z;

    return v3(r, g, b);
}

/** Returns a YUV version of an input linear RGB color in the ITU-R BT.709 color space
\param RGBColor linear HDR RGB color in the ITU-R BT.709 color space
*/
inline vec3 _fn RGBToYUV(const vec3 rgb)
{
    vec3 ret;

    ret.x = dot(rgb, v3(0.2126f, 0.7152f, 0.0722f));
    ret.y = dot(rgb, v3(-0.09991f, -0.33609f, 0.436f));
    ret.z = dot(rgb, v3(0.615f, -0.55861f, -0.05639f));

    return ret;
}

/** Returns a RGB version of an input linear YUV color in the ITU-R BT.709 color space
\param YUVColor linear HDR YUV color in the ITU-R BT.709 color space
*/
inline vec3 _fn YUVToRGB(const vec3 yuv)
{
    vec3 ret;

    ret.x = dot(yuv, v3(1.0f, 0.0f, 1.28033f));
    ret.y = dot(yuv, v3(1.0f, -0.21482f, -0.38059f));
    ret.z = dot(yuv, v3(1.0f, 2.12798f, 0.0f));

    return ret;
}

/** Returns a linear-space RGB version of an input RGB channel value in the ITU-R BT.709 color space
\param sRGBColor sRGB input channel value
*/
inline float _fn SRGBToLinear(const float srgb)
{
    if (srgb <= 0.04045f)
    {
        return srgb * (1.0f / 12.92f);
    }
    else
    {
        return pow((srgb + 0.055f) * (1.0f / 1.055f), 2.4f);
    }
}

/** Returns a linear-space RGB version of an input RGB color in the ITU-R BT.709 color space
\param sRGBColor sRGB input color
*/
inline vec3 _fn SRGBToLinear(const vec3 srgb)
{
    return v3(
        SRGBToLinear(srgb.x),
        SRGBToLinear(srgb.y),
        SRGBToLinear(srgb.z));
}

/** Returns a sRGB version of an input linear RGB channel value in the ITU-R BT.709 color space
\param LinearColor linear input channel value
*/
inline float _fn LinearToSRGB(const float lin)
{
    if (lin <= 0.0031308f)
    {
        return lin * 12.92f;
    }
    else
    {
        return pow(lin, (1.0f / 2.4f)) * (1.055f) - 0.055f;
    }
}

/** Returns a sRGB version of an input linear RGB color in the ITU-R BT.709 color space
\param LinearColor linear input color
*/
inline vec3 _fn LinearToSRGB(const vec3 lin)
{
    return v3(
        LinearToSRGB(lin.x),
        LinearToSRGB(lin.y),
        LinearToSRGB(lin.z));
}

/** Returns Michelson contrast given minimum and maximum intensities of an image region
\param Imin minimum intensity of an image region
\param Imax maximum intensity of an image region
*/
inline float _fn computeMichelsonContrast(const float iMin, const float iMax)
{
    if (iMin == 0.0f && iMax == 0.0f) return 0.0f;
    else return (iMax - iMin) / (iMax + iMin);
}

#ifdef HOST_CODE
static_assert((sizeof(MaterialValues) % sizeof(vec4)) == 0, "MaterialValue has a wrong size");
static_assert((sizeof(MaterialLayerDesc) % sizeof(vec4)) == 0, "MaterialLayerDesc has a wrong size");
static_assert((sizeof(MaterialLayerValues) % sizeof(vec4)) == 0, "MaterialLayerValues has a wrong size");
static_assert((sizeof(MaterialDesc) % sizeof(vec4)) == 0, "MaterialDesc has a wrong size");
static_assert((sizeof(MaterialValues) % sizeof(vec4)) == 0, "MaterialValues has a wrong size");
static_assert((sizeof(MaterialData) % sizeof(vec4)) == 0, "MaterialData has a wrong size");
#undef SamplerState
#undef Texture2D
} // namespace Falcor
#endif // HOST_CODE
#endif //_HOST_DEVICE_DATA_H