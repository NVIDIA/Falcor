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
#include "Framework.h"
#include "FullScreenPass.h"
#include "API/VAO.h"
#include "glm/vec2.hpp"
#include "API/Buffer.h"
#include "API/DepthStencilState.h"
#include "API/RenderContext.h"
#include "API/VertexLayout.h"
#include "API/Window.h"

namespace Falcor
{
    bool checkForViewportArray2Support()
    {
#ifdef FALCOR_GL
        return checkExtensionSupport("GL_NV_viewport_array2");
#elif defined FALCOR_D3D
        return false;
#else
#error Unknown API
#endif
    }
    struct Vertex
    {
        glm::vec2 screenPos;
        glm::vec2 texCoord;
    };

    Buffer::SharedPtr FullScreenPass::spVertexBuffer;
    Vao::SharedPtr FullScreenPass::spVao;


#ifdef FALCOR_D3D
#define INVERT_Y(a) ((a == 1) ? 0 : 1)
#elif defined FALCOR_GL
#define INVERT_Y(a) (a)
#endif
    static const Vertex kVertices[] =
    {
        {glm::vec2(-1,  1), glm::vec2(0, INVERT_Y(1))},
        {glm::vec2(-1, -1), glm::vec2(0, INVERT_Y(0))},
        {glm::vec2( 1,  1), glm::vec2(1, INVERT_Y(1))},
        {glm::vec2( 1, -1), glm::vec2(1, INVERT_Y(0))},
    };
#undef INVERT_Y

    FullScreenPass::UniquePtr FullScreenPass::create(const std::string& fragmentShaderFile, const Program::DefineList& programDefines, bool disableDepth, bool disableStencil, uint32_t viewportMask)
    {
        UniquePtr pPass = UniquePtr(new FullScreenPass());
        pPass->init(fragmentShaderFile, programDefines, disableDepth, disableStencil, viewportMask);
        return pPass;
    }

    void FullScreenPass::init(const std::string& fragmentShaderFile, const Program::DefineList& programDefines, bool disableDepth, bool disableStencil, uint32_t viewportMask)
    {
        mpPipelineStateCache = PipelineStateCache::create();

        // create depth stencil state
        DepthStencilState::Desc dsDesc;
        dsDesc.setDepthTest(!disableDepth);
        dsDesc.setDepthWriteMask(!disableDepth);
        dsDesc.setDepthFunc(DepthStencilState::Func::LessEqual);    // Equal is needed to allow overdraw when z is enabled (e.g., background pass etc.)
        dsDesc.setStencilTest(!disableStencil);
        dsDesc.setStencilWriteMask(!disableStencil);
        mpPipelineStateCache->setDepthStencilState(DepthStencilState::create(dsDesc));

        Program::DefineList defs = programDefines;
        std::string gs;

        if(viewportMask)
        {
            defs.add("_VIEWPORT_MASK", std::to_string(viewportMask));
            if(checkForViewportArray2Support())
            {
                defs.add("_USE_VP2_EXT");
            }
            else
            {
                defs.add("_OUTPUT_PRIM_COUNT", std::to_string(__popcnt(viewportMask)));
                gs = "Framework\\FullScreenPass.gs";
            }
        }

        const std::string vs("Framework\\FullScreenPass.vs");
        mpProgram = Program::createFromFile(vs, fragmentShaderFile, gs, "", "", defs);
        mpPipelineStateCache->setProgram(mpProgram);

        if (FullScreenPass::spVertexBuffer == nullptr)
        {
            // First time we got here. create VB and VAO
            const uint32_t vbSize = (uint32_t)(sizeof(Vertex)*arraysize(kVertices));
            FullScreenPass::spVertexBuffer = Buffer::create(vbSize, Buffer::BindFlags::Vertex, Buffer::CpuAccess::Write, (void*)kVertices);

            // create VAO
            VertexLayout::SharedPtr pLayout = VertexLayout::create();
            VertexBufferLayout::SharedPtr pBufLayout = VertexBufferLayout::create();
            pBufLayout->addElement("POSITION", 0, ResourceFormat::RG32Float, 1, 0);
            pBufLayout->addElement("TEXCOORD", 8, ResourceFormat::RG32Float, 1, 1);
            pLayout->addBufferLayout(0, pBufLayout);

            Vao::BufferVec buffers{ spVertexBuffer };
            FullScreenPass::spVao = Vao::create(buffers, pLayout, nullptr, ResourceFormat::Unknown);
        }
        mpPipelineStateCache->setVao(FullScreenPass::spVao);
        mpPipelineStateCache->setPrimitiveType(PipelineState::PrimitiveType::Triangle);
    }

    void FullScreenPass::execute(RenderContext* pRenderContext, bool overrideDepthStencil) const
    {
        mpPipelineStateCache->setFbo(pRenderContext->getFbo());
        pRenderContext->setPipelineState(mpPipelineStateCache->getPSO());
        pRenderContext->setVao(spVao);

        // DISABLED_FOR_D3D12
//        if (overrideDepthStencil) pRenderContext->setDepthStencilState(mpDepthStencilState, 0);
        pRenderContext->setTopology(RenderContext::Topology::TriangleStrip);
        pRenderContext->draw(arraysize(kVertices), 0);
    }
}