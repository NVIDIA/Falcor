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
#include "RenderContext.h"
#include "RasterizerState.h"
#include "DepthStencilState.h"
#include "BlendState.h"
#include "FBO.h"

namespace Falcor
{
    void RenderContext::initCommon(uint32_t viewportCount)
    {
        // create an empty FBO
        mpEmptyFBO = Fbo::create();
        setFbo(mpEmptyFBO);

        // create the viewports
        mState.viewports.resize(viewportCount);
        mState.scissors.resize(viewportCount);
        mVpStack.resize(viewportCount);
        mScStack.resize(viewportCount);
        for(uint32_t i = 0; i < viewportCount; i++)
        {
            applyViewport(i);
            applyScissor(i);
        }
    }

    const RenderContext::Viewport& RenderContext::getViewport(uint32_t index) const
    {
        if(index >= mState.viewports.size())
        {
            Logger::log(Logger::Level::Error, "RenderContext::getViewport() - Viewport index out of range. Max possible index is " + std::to_string(mState.viewports.size()) + ", got " + std::to_string(index));
            return mState.viewports[0];
        }
        return mState.viewports[index];
    }

    const RenderContext::Scissor& RenderContext::getScissor(uint32_t index) const
    {
        if (index >= mState.scissors.size())
        {
            Logger::log(Logger::Level::Error, "RenderContext::getScissor() - Scissor index out of range. Max possible index is " + std::to_string(mState.scissors.size()) + ", got " + std::to_string(index));
            return mState.scissors[0];
        }
        return mState.scissors[index];
    }

    void RenderContext::pushState()
    {
        mStateStack.push(mState);
    }

    void RenderContext::popState()
    {
        if(mStateStack.empty())
        {
            Logger::log(Logger::Level::Error, "RenderContext::popState() - can't pop state since the state stack is empty.");
            return;
        }
        mState = mStateStack.top();

        // And set all state objects
        applyFbo();
        applyVao();
        applyTopology();
        applyPipelineState();
        applyProgramVars();

        for(uint32_t i = 0; i < mState.viewports.size(); i++)
        {
            applyViewport(i);
        }

        mStateStack.pop();
    }

    void RenderContext::pushFbo(const Fbo::SharedPtr& pFbo)
    {
        mFboStack.push(mState.pFbo);
        setFbo(pFbo);
    }

    void RenderContext::popFbo()
    {
        if(mFboStack.empty())
        {
            Logger::log(Logger::Level::Error, "RenderContext::popFbo() - can't pop FBO since the FBO stack is empty.");
            return;
        }
        const auto& pFbo = mFboStack.top();
        setFbo(pFbo);
        mFboStack.pop();
    }

    void RenderContext::pushViewport(uint32_t index, const Viewport& vp)
    {
        mVpStack[index].push(mState.viewports[index]);
        setViewport(index, vp);
    }

    void RenderContext::popViewport(uint32_t index)
    {
        if(mVpStack[index].empty())
        {
            Logger::log(Logger::Level::Error, "RenderContext::popViewport() - can't pop viewport since the viewport stack is empty.");
            return;
        }
        const auto& VP = mVpStack[index].top();
        setViewport(index, VP);
        mVpStack[index].pop();
    }

    void RenderContext::pushScissor(uint32_t index, const Scissor& sc)
    {
        mScStack[index].push(mState.scissors[index]);
        setScissor(index, sc);
    }

    void RenderContext::popScissor(uint32_t index)
    {
        if(mScStack[index].empty())
        {
            Logger::log(Logger::Level::Error, "RenderContext::popScissor() - can't pop scissor since the scissor stack is empty.");
            return;
        }
        const auto& sc = mScStack[index].top();
        setScissor(index, sc);
        mScStack[index].pop();
    }

    void RenderContext::setPipelineState(const PipelineState::SharedPtr& pState)
    {
        mState.pRenderState = pState;
        applyPipelineState();
    }

    void RenderContext::setVao(const Vao::SharedConstPtr& pVao)
    {
        mState.pVao = pVao;
        applyVao();
    }

    Fbo::SharedConstPtr RenderContext::getFbo() const
    {
        return mState.pFbo;
    }

    void RenderContext::setFbo(const Fbo::SharedPtr& pFbo)
    {
        const auto& pTemp = (pFbo == nullptr) ? mpEmptyFBO : pFbo;
        mState.pFbo = pTemp;
        if(pTemp->checkStatus())
        {
            applyFbo();
        }
    }

    void RenderContext::setTopology(Topology topology)
    {
        mState.topology = topology;
        applyTopology();
    }

    void RenderContext::setViewport(uint32_t index, const Viewport& vp)
    {
        if(index >= mState.viewports.size())
        {
            Logger::log(Logger::Level::Error, "RenderContext::setViewport() - Viewport index out of range. Max possible index is " + std::to_string(mState.viewports.size()) + ", got " + std::to_string(index));
            return;
        }

        mState.viewports[index] = vp;
        applyViewport(index);
    }


    void RenderContext::setScissor(uint32_t index, const Scissor& sc)
    {
        if (index >= mState.scissors.size())
        {
            Logger::log(Logger::Level::Error, "RenderContext::setScissor() - Scissor index out of range. Max possible index is " + std::to_string(mState.scissors.size()) + ", got " + std::to_string(index));
            return;
        }

        mState.scissors[index] = sc;
        applyScissor(index);
    }

    void RenderContext::prepareForDraw()
    {
        // DISABLED_FOR_D3D12
//         for(auto& pUBO : mState.pUniformBuffers)
//         {
//             if(pUBO)
//             {
//                 pUBO->uploadToGPU();
//             }
//         }
// 
//         for(auto& pSSBO : mState.pShaderStorageBuffers)
//         {
//             if(pSSBO)
//             {
//                 pSSBO->uploadToGPU();
//                 pSSBO->setGpuCopyDirty();
//             }
//         }
        prepareForDrawApi();
    }
}
