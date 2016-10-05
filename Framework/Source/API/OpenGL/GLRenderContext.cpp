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
#ifdef FALCOR_GL
#include "API/RenderContext.h"
#include "API/RasterizerState.h"
#include "API/BlendState.h"
#include "API/FBO.h"
#include "API/ProgramVersion.h"
#include "API/VAO.h"
#include "API/Texture.h"
#include "API/ConstantBuffer.h"
#include "API/Buffer.h"
#include "API/ShaderStorageBuffer.h"

namespace Falcor
{
    RenderContext::~RenderContext() = default;

    RenderContext::SharedPtr RenderContext::create()
    {
        uint32_t vpCount;
        gl_call(glGetIntegerv(GL_MAX_VIEWPORTS, (int32_t*)&vpCount));

        auto pCtx = SharedPtr(new RenderContext(vpCount));

        // Allocate space for the constant buffers and shader storage blocks
        int uniformBlockCount;
        gl_call(glGetIntegerv(GL_MAX_COMBINED_UNIFORM_BLOCKS, &uniformBlockCount));
        pCtx->mState.pUniformBuffers.assign(uniformBlockCount, nullptr);

        int shaderStorageBlockCount;
        gl_call(glGetIntegerv(GL_MAX_COMBINED_SHADER_STORAGE_BLOCKS, &shaderStorageBlockCount));
        pCtx->mState.pShaderStorageBuffers.assign(shaderStorageBlockCount, nullptr);

        // Enable some global settings
        gl_call(glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS));
        gl_call(glEnable(GL_SAMPLE_MASK));

        return pCtx;
    }

    GLenum getGlComparisonFunc(DepthStencilState::Func func)
    {
        switch(func)
        {
        case DepthStencilState::Func::Never:
            return GL_NEVER;
        case DepthStencilState::Func::Always:
            return GL_ALWAYS;
        case DepthStencilState::Func::Less:
            return GL_LESS;
        case DepthStencilState::Func::Equal:
            return GL_EQUAL;
        case DepthStencilState::Func::NotEqual:
            return GL_NOTEQUAL;
        case DepthStencilState::Func::LessEqual:
            return GL_LEQUAL;
        case DepthStencilState::Func::Greater:
            return GL_GREATER;
        case DepthStencilState::Func::GreaterEqual:
            return GL_GEQUAL;
        default:
            should_not_get_here();
            return GL_NONE;
        }
    }

    GLenum getGlStencilOp(DepthStencilState::StencilOp op)
    {
        switch(op)
        {
        case DepthStencilState::StencilOp::Keep:
            return GL_KEEP;
        case DepthStencilState::StencilOp::Zero:
            return GL_ZERO;
        case DepthStencilState::StencilOp::Replace:
            return GL_REPLACE;
        case DepthStencilState::StencilOp::Increase:
            return GL_INCR_WRAP;
        case DepthStencilState::StencilOp::IncreaseSaturate:
            return GL_INCR;
        case DepthStencilState::StencilOp::Decrease:
            return GL_DECR_WRAP;
        case DepthStencilState::StencilOp::DecreaseSaturate:
            return GL_DECR;
        case DepthStencilState::StencilOp::Invert:
            return GL_INVERT;
        default:
            should_not_get_here();
            return GL_NONE;
        }
    }

    void SetStencilState(GLenum face, const DepthStencilState::StencilDesc& stencilDesc, uint32_t stencilRef, uint8_t readMask, uint8_t writeMask)
    {
        gl_call(glStencilFuncSeparate(face, getGlComparisonFunc(stencilDesc.func), stencilRef, readMask));
        gl_call(glStencilMaskSeparate(face, writeMask));
        gl_call(glStencilOpSeparate(face, getGlStencilOp(stencilDesc.stencilFailOp), getGlStencilOp(stencilDesc.depthFailOp), getGlStencilOp(stencilDesc.depthStencilPassOp)));
    }

    void RenderContext::applyDepthStencilState() const
    {
        const auto pDepthStencil = mState.pDsState;
        const uint8_t stencilRef = mState.stencilRef;

        if(pDepthStencil->isDepthTestEnabled())
        {
            gl_call(glEnable(GL_DEPTH_TEST));
            gl_call(glDepthFunc(getGlComparisonFunc(pDepthStencil->getDepthFunc())));
            gl_call(glDepthMask(pDepthStencil->isDepthWriteEnabled() ? GL_TRUE : GL_FALSE));
        }
        else
        {
            gl_call(glDisable(GL_DEPTH_TEST));
        }

        if(pDepthStencil->isStencilTestEnabled())
        {
            gl_call(glEnable(GL_STENCIL_TEST));
            SetStencilState(GL_FRONT, pDepthStencil->getStencilDesc(DepthStencilState::Face::Front), stencilRef, pDepthStencil->getStencilReadMask(), pDepthStencil->getStencilWriteMask());
            SetStencilState(GL_BACK, pDepthStencil->getStencilDesc(DepthStencilState::Face::Back), stencilRef, pDepthStencil->getStencilReadMask(), pDepthStencil->getStencilWriteMask());
        }
        else
        {
            gl_call(glDisable(GL_STENCIL_TEST));
        }
    }

    void RenderContext::applyRasterizerState() const
    {
        const auto pRastState = mState.pRastState;

        // Cull mode
        if(pRastState->getCullMode() == RasterizerState::CullMode::None)
        {
            gl_call(glDisable(GL_CULL_FACE));
        }
        else
        {
            gl_call(glEnable(GL_CULL_FACE));
            GLenum glFace;
            switch(pRastState->getCullMode())
            {
            case RasterizerState::CullMode::Front:
                glFace = GL_FRONT;
                break;
            case RasterizerState::CullMode::Back:
                glFace = GL_BACK;
                break;
            default:
                should_not_get_here();
            }
            gl_call(glCullFace(glFace));
        }

        // Fill mode
        GLenum polygonOffsetMode;
        GLenum glPolygon;
        switch(pRastState->getFillMode())
        {
        case RasterizerState::FillMode::Wireframe:
            glPolygon = GL_LINE;
            polygonOffsetMode = GL_POLYGON_OFFSET_LINE;
            break;
        case RasterizerState::FillMode::Solid:
            glPolygon = GL_FILL;
            polygonOffsetMode = GL_POLYGON_OFFSET_FILL;
            break;
        default:
            should_not_get_here();
        }

        gl_call(glPolygonMode(GL_FRONT_AND_BACK, glPolygon));

        // Polygon offset
        if(pRastState->getDepthBias() == 0 && pRastState->getSlopeScaledDepthBias() == 0)
        {
            gl_call(glDisable(polygonOffsetMode));
        }
        else
        {
            gl_call(glEnable(polygonOffsetMode));
            gl_call(glPolygonOffset(pRastState->getSlopeScaledDepthBias(), (float)pRastState->getDepthBias()));
        }

        // Depth clamp
        if(pRastState->isDepthClampEnabled())
        {
            gl_call(glEnable(GL_DEPTH_CLAMP));
        }
        else
            gl_call(glDisable(GL_DEPTH_CLAMP));

        if(pRastState->isScissorTestEnabled())
        {
            gl_call(glEnable(GL_SCISSOR_TEST))
        }
        else
            gl_call(glDisable(GL_SCISSOR_TEST));

        // Line mode
        if(pRastState->isLineAntiAliasingEnabled())
        {
            gl_call(glEnable(GL_LINE_SMOOTH));
        }
        else
        {
            gl_call(glDisable(GL_LINE_SMOOTH));
        }

        // Set front CCW
        gl_call(glFrontFace(pRastState->isFrontCounterCW() ? GL_CCW : GL_CW));
    }

    GLenum getGlBlendFunc(BlendState::BlendFunc func)
    {
        switch(func)
        {
        case BlendState::BlendFunc::Zero:
            return GL_ZERO;
        case BlendState::BlendFunc::One:
            return GL_ONE;
        case BlendState::BlendFunc::SrcColor:
            return GL_SRC_COLOR;
        case BlendState::BlendFunc::OneMinusSrcColor:
            return GL_ONE_MINUS_SRC_COLOR;
        case BlendState::BlendFunc::DstColor:
            return GL_DST_COLOR;
        case BlendState::BlendFunc::OneMinusDstColor:
            return GL_ONE_MINUS_DST_COLOR;
        case BlendState::BlendFunc::SrcAlpha:
            return GL_SRC_ALPHA;
        case BlendState::BlendFunc::OneMinusSrcAlpha:
            return GL_ONE_MINUS_SRC_ALPHA;
        case BlendState::BlendFunc::DstAlpha:
            return GL_DST_ALPHA;
        case BlendState::BlendFunc::OneMinusDstAlpha:
            return GL_ONE_MINUS_DST_ALPHA;
        case BlendState::BlendFunc::RgbaFactor:
            return GL_CONSTANT_COLOR;
        case BlendState::BlendFunc::OneMinusRgbaFactor:
            return GL_ONE_MINUS_CONSTANT_COLOR;
        case BlendState::BlendFunc::SrcAlphaSaturate:
            return GL_SRC_ALPHA_SATURATE;
        case BlendState::BlendFunc::Src1Color:
            return GL_SRC1_COLOR;
        case BlendState::BlendFunc::OneMinusSrc1Color:
            return GL_ONE_MINUS_SRC1_COLOR;
        case BlendState::BlendFunc::Src1Alpha:
            return GL_SRC1_ALPHA;
        case BlendState::BlendFunc::OneMinusSrc1Alpha:
            return GL_ONE_MINUS_SRC1_ALPHA;
        default:
            should_not_get_here();
            return GL_NONE;
        }
    }

    GLenum getGlBlendOp(BlendState::BlendOp op)
    {
        switch(op)
        {
        case BlendState::BlendOp::Add:
            return GL_FUNC_ADD;
        case BlendState::BlendOp::Subtract:
            return GL_FUNC_SUBTRACT;
        case BlendState::BlendOp::ReverseSubtract:
            return GL_FUNC_REVERSE_SUBTRACT;
        case BlendState::BlendOp::Min:
            return GL_MIN;
        case BlendState::BlendOp::Max:
            return GL_MAX;
        default:
            should_not_get_here();
            return GL_NONE;
        }
    }

    void RenderContext::applyBlendState() const
    {
        const auto pBlendState = mState.pBlendState;
        uint32_t sampleMask = mState.sampleMask;

        gl_call(glSampleMaski(0, sampleMask));
        for(uint32_t rtIndex = 0; rtIndex < Fbo::getMaxColorTargetCount(); rtIndex++)
        {
            uint32_t FalcorID = pBlendState->isIndependentBlendEnabled() ? rtIndex : 0;
            
            const auto& writeMask = pBlendState->getRtWriteMask(rtIndex);
            gl_call(glColorMaski(rtIndex, writeMask.writeRed, writeMask.writeGreen, writeMask.writeBlue, writeMask.writeAlpha));

            if(pBlendState->isBlendEnabled(FalcorID))
            {
                gl_call(glEnablei(GL_BLEND, rtIndex));

                GLenum srcRgbFunc = getGlBlendFunc(pBlendState->getSrcRgbFunc(FalcorID));
                GLenum dstRgbFunc = getGlBlendFunc(pBlendState->getDstRgbFunc(FalcorID));
                GLenum srcAlphaFunc = getGlBlendFunc(pBlendState->getSrcAlphaFunc(FalcorID));
                GLenum dstAlphaFunc = getGlBlendFunc(pBlendState->getDstAlphaFunc(FalcorID));
                gl_call(glBlendFuncSeparatei(rtIndex, srcRgbFunc, dstRgbFunc, srcAlphaFunc, dstAlphaFunc));

                GLenum rgbOp = getGlBlendOp(pBlendState->getRgbBlendOp(FalcorID));
                GLenum alphaOp = getGlBlendOp(pBlendState->getAlphaBlendOp(FalcorID));
                gl_call(glBlendEquationSeparatei(rtIndex, rgbOp, alphaOp));
            }
            else
            {
                gl_call(glDisablei(GL_BLEND, rtIndex));
            }
        }

        const auto& factor = pBlendState->getBlendFactor();
        gl_call(glBlendColor(factor.r, factor.g, factor.b, factor.a));

        if(pBlendState->isAlphaToCoverageEnabled())
        {
            gl_call(glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE));
        }
        else
        {
            gl_call(glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE));
        }
    }

    void RenderContext::applyProgram() const
    {
        const auto pProgram = mState.pProgram;
        uint32_t apiHandle = pProgram ? pProgram->getApiHandle() : 0;
        gl_call(glUseProgram(apiHandle));
    }

    void RenderContext::applyVao() const
    {
        const auto pVao = mState.pVao;
        uint32_t apiHandle = pVao ? pVao->getApiHandle() : 0;
        gl_call(glBindVertexArray(apiHandle));
    }

    void RenderContext::applyFbo()
    {
        auto pFbo = mState.pFbo;
        if(pFbo->checkStatus())
        {
            gl_call(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pFbo->getApiHandle()));
        }
    }

    void RenderContext::blitFbo(const Fbo* pSource, const Fbo* pTarget, const glm::ivec4& srcRegion, const glm::ivec4& dstRegion, const bool useLinearFiltering, const FboAttachmentType copyFlags, uint32_t srcIdx, uint32_t dstIdx)
	{
		assert(copyFlags != FboAttachmentType::None);
        pTarget = pTarget ? pTarget : mState.pFbo.get();

		GLint glCopyFlags = 0;
		if((copyFlags & FboAttachmentType::Color) != FboAttachmentType::None)
        {
            glCopyFlags |= GL_COLOR_BUFFER_BIT;
        }
		if((copyFlags & FboAttachmentType::Depth) != FboAttachmentType::None)
        {
            glCopyFlags |= GL_DEPTH_BUFFER_BIT;
        }
		if((copyFlags & FboAttachmentType::Stencil) != FboAttachmentType::None)
        {
            glCopyFlags |= GL_STENCIL_BUFFER_BIT;
        }

        // Check that we have not more than one color attachment in each FBO
        if((copyFlags & FboAttachmentType::Color) != FboAttachmentType::None)
        {
            bool srcValid = (srcIdx < Fbo::getMaxColorTargetCount()) && (pSource->getColorTexture(srcIdx) != nullptr);
            bool dstValid = (dstIdx < Fbo::getMaxColorTargetCount()) && (pTarget->getColorTexture(dstIdx) != nullptr);
            if((srcValid && dstValid) == false)
            {
                Logger::log(Logger::Level::Error, "RenderContext::BlitFbo() - source/target FBO has no attachment with this index.");
            }

            GLenum srcAttch = pSource->getApiHandle() ? GL_COLOR_ATTACHMENT0 + srcIdx : GL_BACK_LEFT;
            GLenum dstAttch = pTarget->getApiHandle() ? GL_COLOR_ATTACHMENT0 + dstIdx : GL_BACK_LEFT;

            gl_call(glNamedFramebufferReadBuffer(pSource->getApiHandle(), srcAttch));
            gl_call(glNamedFramebufferDrawBuffer(pTarget->getApiHandle(), dstAttch));
        }

        // We need to check if the framebuffer is sRGB. There seems to be a bug
        // (or a feature) which causes the sRGB mode to be either ignored or
        // misused during glBlitFramebuffer, and images look washed out.

        // To avoid that problem, we disable sRGB before blit.

        GLboolean isFramebufferSRGB;
        gl_call(glGetBooleanv(GL_FRAMEBUFFER_SRGB, &isFramebufferSRGB));
        const bool isSourceSRGB = isSrgbFormat(pSource->getColorTexture(srcIdx)->getFormat());
        const bool isTargetSRGB = isSrgbFormat(pTarget->getColorTexture(dstIdx)->getFormat());
        if(isSourceSRGB && !isTargetSRGB)
        {
            Logger::log(Logger::Level::Error, "RenderContext::BlitFbo() - source is sRGB and target is linear. Don't know how to convert");
        }
        const bool needDestSRGBConversion = isTargetSRGB && !isSourceSRGB;

        if(isFramebufferSRGB && !needDestSRGBConversion) 
        { 
            gl_call(glDisable(GL_FRAMEBUFFER_SRGB)); 
        }
        else if(!isFramebufferSRGB && needDestSRGBConversion) 
        { 
            gl_call(glEnable(GL_FRAMEBUFFER_SRGB));  
        }

        gl_call(glBlitNamedFramebuffer(pSource->getApiHandle(), pTarget->getApiHandle(),
										srcRegion.x, srcRegion.y, srcRegion.z, srcRegion.w,
										dstRegion.x, dstRegion.y, dstRegion.z, dstRegion.w,
										glCopyFlags, useLinearFiltering ? GL_LINEAR : GL_NEAREST));

        if (isFramebufferSRGB)  
        { 
            gl_call(glEnable(GL_FRAMEBUFFER_SRGB));   
        }
        else
        { 
            gl_call(glDisable(GL_FRAMEBUFFER_SRGB));  
        }
	}

    void RenderContext::applyConstantBuffer(uint32_t index) const
    {
        const auto pBuffer = mState.pUniformBuffers[index];
        uint32_t apiHandle = pBuffer ? pBuffer->getBuffer()->getApiHandle() : 0;
        glBindBufferBase(GL_UNIFORM_BUFFER, index, apiHandle);
    }

    void RenderContext::applyShaderStorageBuffer(uint32_t index) const
    {
        const auto pBuffer = mState.pShaderStorageBuffers[index];
        uint32_t apiHandle = pBuffer ? pBuffer->getBuffer()->getApiHandle() : 0;
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, apiHandle);
    }

    void RenderContext::applyTopology() const
    {
    }

    GLenum getGlTopology(RenderContext::Topology topology)
    {
        switch(topology)
        {
        case Falcor::RenderContext::Topology::PointList:
            return GL_POINTS;
        case Falcor::RenderContext::Topology::LineList:
            return GL_LINES;
        case Falcor::RenderContext::Topology::LineStrip:
            return GL_LINE_STRIP;
        case Falcor::RenderContext::Topology::TriangleList:
            return GL_TRIANGLES;
        case Falcor::RenderContext::Topology::TriangleStrip:
            return GL_TRIANGLE_STRIP;
        default:
            should_not_get_here();
            return GL_NONE;
        }
    }

    void RenderContext::draw(uint32_t vertexCount, uint32_t startVertexLocation)
    {
        prepareForDraw();
        GLenum glTopology = getGlTopology(mState.topology);
        gl_call(glDrawArrays(glTopology, startVertexLocation, vertexCount));
    }

    void RenderContext::drawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int baseVertexLocation)
    {
        prepareForDraw();
        GLenum glTopology = getGlTopology(mState.topology);
        uint32_t offset = sizeof(uint32_t) * startIndexLocation;

        gl_call(glDrawElementsBaseVertex(glTopology, indexCount, GL_UNSIGNED_INT, (void*)(uintptr_t)offset, baseVertexLocation));
    }

    void RenderContext::drawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int baseVertexLocation, uint32_t startInstanceLocation)
    {
        prepareForDraw();
        GLenum glTopology = getGlTopology(mState.topology);
        uint32_t offset = sizeof(uint32_t) * startIndexLocation;

        gl_call(glDrawElementsInstancedBaseVertexBaseInstance(glTopology, indexCount, GL_UNSIGNED_INT, (void*)(uintptr_t)offset, instanceCount, baseVertexLocation, startInstanceLocation));
    }

    void RenderContext::applyViewport(uint32_t index) const
    {
        const Viewport& vp = mState.viewports[index];
        gl_call(glViewportIndexedf(index, vp.originX, vp.originY, vp.width, vp.height));
        gl_call(glDepthRangeIndexed(index, vp.minDepth, vp.maxDepth));
    }

    void RenderContext::applyScissor(uint32_t index) const
    {
        const Scissor& sc = mState.scissors[index];
        gl_call(glScissorIndexed(index, sc.originX, sc.originY, sc.width, sc.height));
    }

    void RenderContext::prepareForDrawApi() const
    {

    }
}
#endif //#ifdef FALCOR_GL
