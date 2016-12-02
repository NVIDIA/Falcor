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

namespace Falcor
{
    class RenderContext;

    class Resource
    {
    public:
        using ApiHandle = ResourceHandle;
        /** These flags are hints the driver to what pipeline stages the resource will be bound to.
        */
        enum class BindFlags : uint32_t
        {
            None = 0x0,             ///< The resource will not be bound the pipeline. Use this to create a staging resource
            Vertex = 0x1,           ///< The resource will be bound as a vertex-buffer
            Index = 0x2,            ///< The resource will be bound as a index-buffer
            Constant = 0x4,         ///< The resource will be bound as a constant-buffer
            StreamOutput = 0x8,     ///< The resource will be bound to the stream-output stage as an output buffer
            ShaderResource = 0x10,  ///< The resource will be bound as a shader-resource
            UnorderedAccess = 0x20, ///< The resource will be bound as an UAV
            RenderTarget = 0x40,    ///< The resource will be bound as a render-target
            DepthStencil = 0x80,    ///< The resource will be bound as a depth-stencil buffer
        };

        /** Resource types. Notice there are no array types. Array are controlled using the array size parameter on texture creation.
        */
        enum class Type
        {
            Buffer,                     ///< Buffer. Can be bound to all shader-stages
            Texture1D,                  ///< 1D texture. Can be bound as render-target, shader-resource and UAV
            Texture2D,                  ///< 2D texture. Can be bound as render-target, shader-resource and UAV
            Texture3D,                  ///< 3D texture. Can be bound as render-target, shader-resource and UAV
            TextureCube,                ///< Texture-cube. Can be bound as render-target, shader-resource and UAV
            Texture2DMultisample,       ///< 2D multi-sampled texture. Can be bound as render-target, shader-resource and UAV
        };

        /** Resource state. Keeps track of how the resource was last used
        */
        enum class State : uint32_t
        {
            Common,
            VertexBuffer,
            ConstantBuffer,
            IndexBuffer,
            RenderTarget,
            UnorderedAccess,
            DepthStencil,
            ShaderResource,
            StreamOut,
            IndirectArg,
            CopyDest,
            CopySource,
            ResolveDest,
            ResolveSource,
            Present,
            Predication,
        };

        virtual ~Resource() = 0;

        BindFlags getBindFlags() const { return mBindFlags; }
        State getState() const { return mState; }
        Type getType() const { return mType; }


        /** Get the API handle
        */
        ApiHandle getApiHandle() const { return mApiHandle; }
    protected:
        friend class RenderContext;

        Resource(Type type, BindFlags bindFlags) : mType(type), mBindFlags(bindFlags) {}
        Type mType;
        BindFlags mBindFlags;
        mutable State mState = State::Common;
        ApiHandle mApiHandle;
    };

    enum_class_operators(Resource::BindFlags);
    enum_class_operators(Resource::State);

    const std::string to_string(Resource::Type);
    const std::string to_string(Resource::BindFlags);
    const std::string to_string(Resource::State);
}