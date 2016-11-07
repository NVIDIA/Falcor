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
#include <map>
#include <string>
#include "Graphics/Program.h"
#include "API/VAO.h"
#include "API/DepthStencilState.h"
#include "API/Buffer.h"
#include "API/ProgramVersion.h"
#include "Graphics/PipelineState.h"

namespace Falcor
{
    class RenderContext;

    /** Helper class to simplify full-screen passes
    */
    class FullScreenPass
    {
    public:
        using UniquePtr = std::unique_ptr<FullScreenPass>;
        using UniqueConstPtr = std::unique_ptr<const FullScreenPass>;

        /** create a new object
            \param[in] fragmentShaderFile Fragment shader filename
            \param[in] shaderDefines Optional. A list of macro definitions to be patched into the shaders.
            \param[in] disableDepth Optional. Disable depth test (and therefore depth writes).  This is the common case; however, e.g. writing depth in fullscreen passes can sometimes be useful.
            \param[in] disableStencil Optional. As DisableDepth for stencil.
            \param[in] viewportMask Optional. If different than zero, than will be used to initialize the gl_Layer and gl_ViewportMask[]. Useful for multi-projection passes
        */
        static UniquePtr create(const std::string& fragmentShaderFile, const Program::DefineList& programDefines = Program::DefineList(), bool disableDepth = true, bool disableStencil = true, uint32_t viewportMask = 0);

        /** Execute the pass\n.The function will change the state of the rendering context. You can wrap this call with RenderContext#PushState() and RenderContext#PopState() to save and restore the render state.
            \param[in] pRenderContext The render context.
        */
        void execute(RenderContext* pRenderContext, bool overrideDepthStencil = true) const;

        /** Get the program.
        */
        const Program::SharedConstPtr getProgram() const { return mpProgram; }
        Program::SharedPtr getProgram() { return mpProgram; }

    protected:
        FullScreenPass() = default;

        void init(const std::string & fragmentShaderFile, const Program::DefineList& programDefines, bool disableDepth, bool disableStencil, uint32_t viewportMask);

    private:
        Program::SharedPtr mpProgram;
        PipelineState::SharedPtr mpPipelineStateCache;
        // Static
        static Buffer::SharedPtr spVertexBuffer;
        static Vao::SharedPtr    spVao;
    };
}