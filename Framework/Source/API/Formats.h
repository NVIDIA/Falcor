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
#pragma once
#include <vector>

namespace Falcor
{
    /*!
    *  \addtogroup Falcor
    *  @{
    */

    /** Resource formats
    */
    enum class ResourceFormat : uint32_t
    {
        Unknown,
        R8Unorm,
        R8Snorm,
        R16Unorm,
        R16Snorm,
        RG8Unorm,
        RG8Snorm,
        RG16Unorm,
        RG16Snorm,
        RGB16Unorm,
        RGB16Snorm,
        RGB5A1Unorm,
        RGBA8Unorm,
        RGBA8Snorm,
        RGB10A2Unorm,
        RGB10A2Uint,
        RGBA16Unorm,
        RGBA8UnormSrgb,
        R16Float,
        RG16Float,
        RGB16Float,
        RGBA16Float,
        R32Float,
        RG32Float,
        RGB32Float,
        RGBA32Float,
        R11G11B10Float,
        RGB9E5Float,
        R8Int,
        R8Uint,
        R16Int,
        R16Uint,
        R32Int,
        R32Uint,
        RG8Int,
        RG8Uint,
        RG16Int,
        RG16Uint,
        RG32Int,
        RG32Uint,
        RGB16Int,
        RGB16Uint,
        RGB32Int,
        RGB32Uint,
        RGBA8Int,
        RGBA8Uint,
        RGBA16Int,
        RGBA16Uint,
        RGBA32Int,
        RGBA32Uint,

        BGRA8Unorm,
        BGRA8UnormSrgb,
        
        RGBX8Unorm,
        RGBX8UnormSrgb,
        BGRX8Unorm,
        BGRX8UnormSrgb,
        Alpha8Unorm,
        Alpha32Float,
        R5G6B5Unorm,

        // Depth-stencil
        D32Float,
        D16Unorm,
        D32FloatS8X24,
        D24UnormS8,

        // Compressed formats
        BC1Unorm,   // DXT1
        BC1UnormSrgb, 
        BC2Unorm,   // DXT3
        BC2UnormSrgb,
        BC3Unorm,   // DXT5
        BC3UnormSrgb,
        BC4Unorm,   // RGTC Unsigned Red
        BC4Snorm,   // RGTC Signed Red
        BC5Unorm,   // RGTC Unsigned RG
        BC5Snorm,   // RGTC Signed RG
    };
    
    /** Falcor format Type
    */
    enum class FormatType
    {
        Unknown,        ///< Unknown format Type
        Float,          ///< Floating-point formats
        Unorm,          ///< Unsigned normalized formats
        UnormSrgb,      ///< Unsigned normalized SRGB formats
        Snorm,          ///< Signed normalized formats
        Uint,           ///< Unsigned integer formats
        Sint            ///< Signed integer formats
    };

    struct FormatDesc
    {
        ResourceFormat format;
        const std::string name;
        uint32_t bytesPerBlock;
        uint32_t channelCount;
        FormatType Type;
        struct  
        {
            bool isDepth;
            bool isStencil;
            bool isCompressed;
        };
        struct 
        {
            uint32_t width;
            uint32_t height;
        } compressionRatio;
    };

    extern const FormatDesc kFormatDesc[];

    /** Get the number of bytes per format
    */
    inline uint32_t getFormatBytesPerBlock(ResourceFormat format)
    {
        assert(kFormatDesc[(uint32_t)format].format == format);
		return kFormatDesc[(uint32_t)format].bytesPerBlock;
    }

	inline uint32_t getFormatPixelsPerBlock(ResourceFormat format)
	{
		assert(kFormatDesc[(uint32_t)format].format == format);
		return kFormatDesc[(uint32_t)format].compressionRatio.width * kFormatDesc[(uint32_t)format].compressionRatio.height;
	}

    /** Check if the format has a depth component
    */
    inline bool isDepthFormat(ResourceFormat format)
    {
        assert(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].isDepth;
    }

    /** Check if the format has a stencil component
    */
    inline bool isStencilFormat(ResourceFormat format)
    {
        assert(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].isStencil;
    }

    /** Check if the format has depth or stencil components
    */
    inline bool isDepthStencilFormat(ResourceFormat format)
    {
        return isDepthFormat(format) || isStencilFormat(format);
    }

    /** Check if the format is a compressed format
    */
    inline bool isCompressedFormat(ResourceFormat format)
    {
        assert(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].isCompressed;
    }

    /** Get the format compression ration along the x-axis
    */
    inline uint32_t getFormatWidthCompressionRatio(ResourceFormat format)
    {
        assert(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].compressionRatio.width;
    }

    /** Get the format compression ration along the y-axis
    */
    inline uint32_t getFormatHeightCompressionRatio(ResourceFormat format)
    {
        assert(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].compressionRatio.height;
    }

    /** Get the number of channels
    */
    inline uint32_t getFormatChannelCount(ResourceFormat format)
    {
        assert(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].channelCount;
    }

    /** Get the format Type
    */
    inline FormatType getFormatType(ResourceFormat format)
    {
        assert(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].Type;
    }

    /** Check if a format represents sRGB color space
    */
    inline bool isSrgbFormat(ResourceFormat format)
    {
        return (getFormatType(format) == FormatType::UnormSrgb);
    }

	/** Convert an SRGB format to linear. If the format is alread linear, will return it
	*/
	inline ResourceFormat srgbToLinearFormat(ResourceFormat format)
	{
		switch (format)
		{
		case ResourceFormat::BC1UnormSrgb:
			return ResourceFormat::BC1Unorm;
		case ResourceFormat::BC2UnormSrgb:
			return ResourceFormat::BC2Unorm;
		case ResourceFormat::BC3UnormSrgb:
			return ResourceFormat::BC3Unorm;
		case ResourceFormat::BGRA8UnormSrgb:
			return ResourceFormat::BGRA8Unorm;
		case ResourceFormat::BGRX8UnormSrgb:
			return ResourceFormat::BGRX8Unorm;
		case ResourceFormat::RGBA8UnormSrgb:
			return ResourceFormat::RGBA8Unorm;
		case ResourceFormat::RGBX8UnormSrgb:
			return ResourceFormat::RGBX8Unorm;
		default:
			assert(isSrgbFormat(format) == false);
			return format;
		}
	}

    /** Convert an linear format to sRGB. If the format doesn't have a matching sRGB format, will return the original
    */
    inline ResourceFormat linearToSrgbFormat(ResourceFormat format)
    {
        switch (format)
        {
        case ResourceFormat::BC1Unorm:
            return ResourceFormat::BC1UnormSrgb;
        case ResourceFormat::BC2Unorm:
            return ResourceFormat::BC2UnormSrgb;
        case ResourceFormat::BC3Unorm:
            return ResourceFormat::BC3UnormSrgb;
        case ResourceFormat::BGRA8Unorm:
            return ResourceFormat::BGRA8UnormSrgb;
        case ResourceFormat::BGRX8Unorm:
            return ResourceFormat::BGRX8UnormSrgb;
        case ResourceFormat::RGBA8Unorm:
            return ResourceFormat::RGBA8UnormSrgb;
        case ResourceFormat::RGBX8Unorm:
            return ResourceFormat::RGBX8UnormSrgb;
        default:
            return format;
        }
    }

    inline const std::string& to_string(ResourceFormat format)
    {
        assert(kFormatDesc[(uint32_t)format].format == format);
        return kFormatDesc[(uint32_t)format].name;
    }

    inline const std::string to_string(FormatType Type)
    {
#define type_2_string(a) case FormatType::a: return #a;
        switch(Type)
        {
        type_2_string(Unknown);
        type_2_string(Float);
        type_2_string(Unorm);
        type_2_string(UnormSrgb);
        type_2_string(Snorm);
        type_2_string(Uint);
        type_2_string(Sint);
        default:
            should_not_get_here();
            return "";
        }
#undef type_2_string
    }
    /*! @} */
}