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
#include "glm/vec4.hpp"

namespace Falcor
{
    /** Abstract the API sampler state object
    */
    class Sampler : public std::enable_shared_from_this<Sampler>
    {
    public:
        using SharedPtr = std::shared_ptr<Sampler>;
        using SharedConstPtr = std::shared_ptr<const Sampler>;
        using ApiHandle = SamplerHandle;

        /** Filter mode
        */
        enum class Filter
        {
            Point,
            Linear,
        };

        /** Addressing mode in case the texture coordinates are out of [0, 1] range
        */
        enum class AddressMode
        {
            Wrap,               ///< Wrap around
            Mirror,             ///< Wrap around and mirror on every integer junction
            Clamp,              ///< Clamp the normalized coordinates to [0, 1]
            Border,             ///< If out-of-bound, use the sampler's border color 
            MirrorOnce          ///< Same as Mirror, but mirrors only once around 0
        };

        /** Comparison mode for the sampler.
        */
        enum class ComparisonMode
        {
            Disabled,           ///< No comparison. Regular filtering.
            LessEqual,          ///< Passes if source is less than or equal to the destination
            GreaterEqual,       ///< Passes if source is greater than or equal to the destination
            Less,               ///< Passes if source is less than the destination
            Greater,            ///< Passes if source is greater than to the destination
            Equal,              ///< Passes if source is equal to the destination
            NotEqual,           ///< Passes if source is not equal to the destination
            Always,             ///< Comparison always passes
            Never,              ///< Comparison always fails
        };

        /** Descriptor used to create a new Sampler object
        */
        class Desc
        {
        public:
            friend class Sampler;
            /** Set the filter mode
                \param minFilter Filter mode in case of minification.
                \param magFilter Filter mode in case of magnification.
                \param mipFilter Mip-level sampling mode

            */
            Desc& setFilterMode(Filter minFilter, Filter magFilter, Filter mipFilter);
            /** Set the maximum anisotropic filtering value. If MaxAnisotropy > 1, min/mag/mip filter modes are ignored
            */
            Desc& setMaxAnisotropy(uint32_t maxAnisotropy);
            /** Set the lod clamp parameters
                \param minLod - Minimum LOD that will be used when sampling
                \param maxLod - Maximum LOD that will be used when sampling
                \param lodBias - Bias to apply to the LOD
            */
            Desc& setLodParams(float minLod, float maxLod, float lodBias);
            /** Set the sampler comparison mode
            */
            Desc& setComparisonMode(ComparisonMode mode);
            /** Set the sampler addressing mode
                \param modeU Addressing mode for U texcoord channel
                \param modeV Addressing mode for V texcoord channel
                \param modeW Addressing mode for W texcoord channel
            */
            Desc& setAddressingMode(AddressMode modeU, AddressMode modeV, AddressMode modeW);
            /** Set the border color. Only applies when the addressing mode is ClampToBorder
            */
            Desc& setBorderColor(const glm::vec4& borderColor);
        protected:
            Filter mMagFilter = Filter::Point;
            Filter mMinFilter = Filter::Point;
            Filter mMipFilter = Filter::Point;
            uint32_t mMaxAnisotropy = 1;
            float mMaxLod = 1000;
            float mMinLod = -1000;
            float mLodBias = 0;
            ComparisonMode mComparisonMode = ComparisonMode::Disabled;
            AddressMode mModeU = AddressMode::Wrap;
            AddressMode mModeV = AddressMode::Wrap;
            AddressMode mModeW = AddressMode::Wrap;
            glm::vec4 mBorderColor = glm::vec4(0, 0, 0, 0);
        };

        /** create a new sampler object
            \return A new object, or nullptr if an error occurred
        */
        static SharedPtr create(const Desc& desc);
        ~Sampler();

        /** Get the API handle
        */
        ApiHandle getApiHandle() const { return mApiHandle; }

        /** Get the magnification filter
        */
        Filter getMagFilter() const { return mDesc.mMagFilter; }
        /** Get the minification filter
        */
        Filter getMinFilter() const { return mDesc.mMinFilter; }

        /** Get the mip-levels filter
        */
        Filter getMipFilter() const { return mDesc.mMipFilter; }

        /** Get the maximum anisotropy
        */
        uint32_t getMaxAnisotropy() const { return mDesc.mMaxAnisotropy; }

        /** Get the minimum LOD value
        */
        float getMinLod() const { return mDesc.mMinLod; }
        /** Get the maximum LOD value
        */
        float getMaxLod() const { return mDesc.mMaxLod; }
        /** Get the LOD bias
        */
        float getLodBias() const { return mDesc.mLodBias; }

        /** Get the comparison mode
        */
        ComparisonMode getComparisonMode() const { return mDesc.mComparisonMode; }

        /** Get the addressing mode for the U texcoord
        */
        AddressMode getAddressModeU() const { return mDesc.mModeU; }
        /** Get the addressing mode for the V texcoord
        */
        AddressMode getAddressModeV() const { return mDesc.mModeV; }
        /** Get the addressing mode for the W texcoord
        */
        AddressMode getAddressModeW() const { return mDesc.mModeW; }

        /** Get the border color
        */
        const glm::vec4& getBorderColor() const { return mDesc.mBorderColor; }

        static SharedPtr getDefault();
    private:
        static SharedPtr sNullSampler;
        Sampler(const Desc& desc);
        Desc mDesc;
        ApiHandle mApiHandle = { 0 };
        static uint32_t getApiMaxAnisotropy();
    };
}