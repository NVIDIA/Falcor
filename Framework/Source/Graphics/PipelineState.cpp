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
#include "Framework.h"
#include "PipelineState.h"

namespace Falcor
{
    // FIXME this breaks our convention that API code doesn't appear in common files
#ifdef FALCOR_D3D11
    static const uint32_t kViewportCount = D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
#else if defined FALCOR_D3D12
    static const uint32_t kViewportCount = D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE;
#endif


    std::vector<PipelineStateObject::SharedPtr> gStates;

    static PipelineStateObject::PrimitiveType topology2Type(Vao::Topology t)
    {
        switch (t)
        {
        case Vao::Topology::PointList:
            return PipelineStateObject::PrimitiveType::Point;
        case Vao::Topology::LineList:
        case Vao::Topology::LineStrip:
            return PipelineStateObject::PrimitiveType::Line;
        case Vao::Topology::TriangleList:
        case Vao::Topology::TriangleStrip:
            return PipelineStateObject::PrimitiveType::Triangle;
        default:
            should_not_get_here();
            return PipelineStateObject::PrimitiveType::Undefined;
        }
    }

    PipelineState::PipelineState()
    {
#ifdef FALCOR_GL
        uint32_t kViewportCount;
        gl_call(glGetIntegerv(GL_MAX_VIEWPORTS, (int32_t*)&kViewportCount));
#endif
        // create the viewports
        mViewports.resize(kViewportCount);
        mScissors.resize(kViewportCount);
        mVpStack.resize(kViewportCount);
        mScStack.resize(kViewportCount);
        for (uint32_t i = 0; i < kViewportCount; i++)
        {
            setViewport(i, mViewports[i], true);
        }
    }

    PipelineStateObject::SharedPtr PipelineState::getPSO()
    {
        // Check if we need to create a root-signature
        // FIXME Is this the correct place for this?
        if (mpProgram && mpVao)
        {
            mpVao->getVertexLayout()->addVertexAttribDclToProg(mpProgram.get());
        }
        const ProgramVersion* pProgVersion = mpProgram ? mpProgram->getActiveVersion().get() : nullptr;
        if (pProgVersion != mCachedData.pProgramVersion)
        {
            mCachedData.pProgramVersion = pProgVersion;
            if (mCachedData.isUserRootSignature == false)
            {
                mpRootSignature = RootSignature::create(pProgVersion->getReflector().get());
            }
        }

        mDesc.setProgramVersion(mpProgram ? mpProgram->getActiveVersion() : nullptr);
        mDesc.setFboFormats(mpFbo ? mpFbo->getDesc() : Fbo::Desc());
        mDesc.setVertexLayout(mpVao->getVertexLayout());
        mDesc.setPrimitiveType(topology2Type(mpVao->getPrimitiveTopology()));
        mDesc.setRootSignature(mpRootSignature);

        // FIXME D3D12 Need real cache
        auto p = PipelineStateObject::create(mDesc);
        gStates.push_back(p);
        return p;
    }

    PipelineState& PipelineState::setFbo(const Fbo::SharedConstPtr& pFbo, bool setViewportScissors)
    {
        mpFbo = pFbo;
        if (setViewportScissors && pFbo)
        {
            // OPTME: do we really need to run on the entire VP array?
            uint32_t w = pFbo->getWidth();
            uint32_t h = pFbo->getHeight();
            Viewport vp(0, 0, float(w), float(h), 0, 1);
            for (uint32_t i = 0; i < kViewportCount; i++)
            {
                setViewport(i, vp, true);
            }
        }
        return *this;
    }

    void PipelineState::pushFbo(const Fbo::SharedPtr& pFbo, bool setViewportScissors)
    {
        mFboStack.push(mpFbo);
        setFbo(pFbo, setViewportScissors);
    }

    void PipelineState::popFbo(bool setViewportScissors)
    {
        if (mFboStack.empty())
        {
            Logger::log(Logger::Level::Error, "PipelineState::popFbo() - can't pop FBO since the viewport stack is empty.");
            return;
        }
        setFbo(mFboStack.top(), setViewportScissors);
        mFboStack.pop();
    }

    void PipelineState::pushViewport(uint32_t index, const Viewport& vp, bool setScissors)
    {
        mVpStack[index].push(mViewports[index]);
        setViewport(index, vp, setScissors);
    }

    void PipelineState::popViewport(uint32_t index, bool setScissors)
    {
        if (mVpStack[index].empty())
        {
            Logger::log(Logger::Level::Error, "PipelineState::popViewport() - can't pop viewport since the viewport stack is empty.");
            return;
        }
        const auto& VP = mVpStack[index].top();
        setViewport(index, VP, setScissors);
        mVpStack[index].pop();
    }

    void PipelineState::pushScissors(uint32_t index, const Scissor& sc)
    {
        mScStack[index].push(mScissors[index]);
        setScissors(index, sc);
    }

    void PipelineState::popScissors(uint32_t index)
    {
        if (mScStack[index].empty())
        {
            Logger::log(Logger::Level::Error, "PipelineState::popScissors() - can't pop scissors since the scissors stack is empty.");
            return;
        }
        const auto& sc = mScStack[index].top();
        setScissors(index, sc);
        mScStack[index].pop();
    }

    void PipelineState::setViewport(uint32_t index, const Viewport& vp, bool setScissors)
    {
        mViewports[index] = vp;
        if (setScissors)
        {
            Scissor sc(0, 0, (int32_t)vp.width, (int32_t)vp.height);
            this->setScissors(index, sc);
        }
    }

    void PipelineState::setScissors(uint32_t index, const Scissor& sc)
    {
        mScissors[index] = sc;
    }
}