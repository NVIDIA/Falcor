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
#include "Gui.h"
#include <sstream>
#include "OS.h"
#include "Utils/UserInput.h"
#include "API/RenderContext.h"
#include "Externals/dear_imgui/imgui.h"
#include "Utils/Math/FalcorMath.h"

namespace Falcor
{
    void Gui::init()
    {
        ImGuiIO& io = ImGui::GetIO();
        io.KeyMap[ImGuiKey_Tab] = (uint32_t)KeyboardEvent::Key::Tab;
        io.KeyMap[ImGuiKey_LeftArrow] = (uint32_t)KeyboardEvent::Key::Left;
        io.KeyMap[ImGuiKey_RightArrow] = (uint32_t)KeyboardEvent::Key::Right;
        io.KeyMap[ImGuiKey_UpArrow] = (uint32_t)KeyboardEvent::Key::Up;
        io.KeyMap[ImGuiKey_DownArrow] = (uint32_t)KeyboardEvent::Key::Down;
        io.KeyMap[ImGuiKey_PageUp] = (uint32_t)KeyboardEvent::Key::PageUp;
        io.KeyMap[ImGuiKey_PageDown] = (uint32_t)KeyboardEvent::Key::PageDown;
        io.KeyMap[ImGuiKey_Home] = (uint32_t)KeyboardEvent::Key::Home;
        io.KeyMap[ImGuiKey_End] = (uint32_t)KeyboardEvent::Key::End;
        io.KeyMap[ImGuiKey_Delete] = (uint32_t)KeyboardEvent::Key::Del;
        io.KeyMap[ImGuiKey_Backspace] = (uint32_t)KeyboardEvent::Key::Backspace;
        io.KeyMap[ImGuiKey_Enter] = (uint32_t)KeyboardEvent::Key::Enter;
        io.KeyMap[ImGuiKey_Escape] = (uint32_t)KeyboardEvent::Key::Escape;
        io.KeyMap[ImGuiKey_A] = (uint32_t)KeyboardEvent::Key::A;
        io.KeyMap[ImGuiKey_C] = (uint32_t)KeyboardEvent::Key::C;
        io.KeyMap[ImGuiKey_V] = (uint32_t)KeyboardEvent::Key::V;
        io.KeyMap[ImGuiKey_X] = (uint32_t)KeyboardEvent::Key::X;
        io.KeyMap[ImGuiKey_Y] = (uint32_t)KeyboardEvent::Key::Y;
        io.KeyMap[ImGuiKey_Z] = (uint32_t)KeyboardEvent::Key::Z;

        // Create the pipeline state cache
        mpPipelineStateCache = PipelineStateCache::create();

        // Create the program
        mpProgram = Program::createFromFile("Framework//Gui.vs", "Framework//Gui.ps");
        mpProgramVars = ProgramVars::create(mpProgram->getActiveVersion()->getReflector());
        mpPipelineStateCache->setProgram(mpProgram);

        // Create and set the texture
        uint8_t* pFontData;
        int32_t width, height;
        io.Fonts->GetTexDataAsAlpha8(&pFontData, &width, &height);
        Texture::SharedPtr pTexture = Texture::create2D(width, height, ResourceFormat::Alpha8Unorm, 1, 1, pFontData);
        mpProgramVars->setTexture("gFont", pTexture);

        // Create the blend state
        BlendState::Desc blendDesc;
        blendDesc.setRtBlend(0, true).setRtParams(0, BlendState::BlendOp::Add, BlendState::BlendOp::Add, BlendState::BlendFunc::SrcAlpha, BlendState::BlendFunc::OneMinusSrcAlpha, BlendState::BlendFunc::OneMinusSrcAlpha, BlendState::BlendFunc::Zero);
        mpPipelineStateCache->setBlendState(BlendState::create(blendDesc));

        // Create the rasterizer state
        RasterizerState::Desc rsDesc;
        rsDesc.setFillMode(RasterizerState::FillMode::Solid).setCullMode(RasterizerState::CullMode::None).setScissorTest(true).setDepthClamp(false);
        mpPipelineStateCache->setRasterizerState(RasterizerState::create(rsDesc));

        // Create the depth-stencil state
        DepthStencilState::Desc dsDesc;
        dsDesc.setDepthFunc(DepthStencilState::Func::Disabled);
        mpPipelineStateCache->setDepthStencilState(DepthStencilState::create(dsDesc));

        // Create the VAO
        VertexBufferLayout::SharedPtr pBufLayout = VertexBufferLayout::create();
        pBufLayout->addElement("POSITION", offsetof(ImDrawVert, pos), ResourceFormat::RG32Float, 1, 0);
        pBufLayout->addElement("TEXCOORD", offsetof(ImDrawVert, uv), ResourceFormat::RG32Float, 1, 1);
        pBufLayout->addElement("COLOR", offsetof(ImDrawVert, col), ResourceFormat::RGBA8Unorm, 1, 2);
        mpLayout = VertexLayout::create();
        mpLayout->addBufferLayout(0, pBufLayout);
        
        mpPipelineStateCache->setPrimitiveType(PipelineState::PrimitiveType::Triangle);
    }

    Gui::UniquePtr Gui::create(uint32_t width, uint32_t height)
    {
        UniquePtr pGui = UniquePtr(new Gui);
        pGui->init();
        pGui->onWindowResize(width, height);
        pGui->mpProgramVars[0][64] = glm::vec3(1, 1, 1);
        ImGui::NewFrame();
        return pGui;
    }

    void Gui::createVao(uint32_t vertexCount, uint32_t indexCount)
    {
        static_assert(sizeof(ImDrawIdx) == sizeof(uint16_t), "ImDrawIdx expected size is a word");
        uint32_t requiredVbSize = vertexCount * sizeof(ImDrawVert);
        uint32_t requiredIbSize = indexCount * sizeof(uint16_t);

        // OPTME: Can check if only a single buffer changed and just create it
        if (mpVao)
        {
            bool vbSizeOK = mpVao->getVertexBuffer(0)->getSize() <= requiredVbSize;
            bool ibSizeOK = mpVao->getIndexBuffer()->getSize() <= requiredIbSize;
            if (vbSizeOK && ibSizeOK)
            {
                return;
            }
        }

        // Need to create a new VAO
        std::vector<Buffer::SharedPtr> pVB(1);
        pVB[0] = Buffer::create(requiredVbSize + sizeof(ImDrawVert) * 1000, Buffer::BindFlags::Vertex, Buffer::CpuAccess::Write, nullptr);
        Buffer::SharedPtr pIB = Buffer::create(requiredIbSize, Buffer::BindFlags::Index, Buffer::CpuAccess::Write, nullptr);
        mpVao = Vao::create(pVB, mpLayout, pIB, ResourceFormat::R16Uint);
    }

    void Gui::onWindowResize(uint32_t width, uint32_t height)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize.x = (float)width;
        io.DisplaySize.y = (float)height;
        mpProgramVars[0][0] = orthographicMatrix(0, float(width), float(height), 0, 0, 1);
    }

    void Gui::render(RenderContext* pContext)
    {
        pContext->setProgramVariables(mpProgramVars);
        pContext->setTopology(RenderContext::Topology::TriangleList);
        ImGui::Render();
        ImDrawData* pDrawData = ImGui::GetDrawData();

        // Update the VAO
        createVao(pDrawData->TotalVtxCount, pDrawData->TotalIdxCount);
        mpPipelineStateCache->SetVao(mpVao);
        pContext->setVao(mpVao);

        // Upload the data
        ImDrawVert* pVerts = (ImDrawVert*)mpVao->getVertexBuffer(0)->map(Buffer::MapType::WriteDiscard);
        uint16_t* pIndices = (uint16_t*)mpVao->getIndexBuffer()->map(Buffer::MapType::WriteDiscard);

        for (int n = 0; n < pDrawData->CmdListsCount; n++)
        {
            const ImDrawList* pCmdList = pDrawData->CmdLists[n];
            memcpy(pVerts, pCmdList->VtxBuffer.Data, pCmdList->VtxBuffer.Size * sizeof(ImDrawVert));
            memcpy(pIndices, pCmdList->IdxBuffer.Data, pCmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
            pVerts += pCmdList->VtxBuffer.Size;
            pIndices += pCmdList->IdxBuffer.Size;
        }
        mpVao->getVertexBuffer(0)->unmap();
        mpVao->getIndexBuffer()->unmap();
        mpPipelineStateCache->setFbo(pContext->getFbo());
        pContext->setPipelineState(mpPipelineStateCache->getPSO());

        // Setup viewport
        RenderContext::Viewport vp;
        vp.originX = 0;
        vp.originY = 0;
        vp.width = ImGui::GetIO().DisplaySize.x;
        vp.height = ImGui::GetIO().DisplaySize.y;
        vp.minDepth = 0;
        vp.maxDepth = 1;
        pContext->pushViewport(0, vp);

        // Render command lists
        uint32_t vtxOffset = 0;
        uint32_t idxOffset = 0;

        for (int n = 0; n < pDrawData->CmdListsCount; n++)
        {
            const ImDrawList* pCmdList = pDrawData->CmdLists[n];
            for (int32_t cmd = 0; cmd < pCmdList->CmdBuffer.Size; cmd++)
            {
                const ImDrawCmd* pCmd = &pCmdList->CmdBuffer[cmd];
                RenderContext::Scissor scissor((int32_t)pCmd->ClipRect.x, (int32_t)pCmd->ClipRect.y, (int32_t)pCmd->ClipRect.z, (int32_t)pCmd->ClipRect.w);
                pContext->pushScissor(0, scissor);
                pContext->drawIndexed(pCmd->ElemCount, idxOffset, vtxOffset);
                pContext->popScissor(0);
                idxOffset += pCmd->ElemCount;
            }
            vtxOffset += pCmdList->VtxBuffer.Size;
        }

        pContext->popViewport(0);

        ImGui::NewFrame();
    }

    void Gui::text(const std::string& str)
    {
        ImGui::Text(str.c_str());
        if (ImGui::Button("click"))
        {
            msgBox("Click");
        }
    }

    bool Gui::onKeyboardEvent(const KeyboardEvent& event)
    {
        return false;
    }

    bool Gui::onMouseEvent(const MouseEvent& event)
    {
        ImGuiIO& io = ImGui::GetIO();
        switch (event.type)
        {
        case MouseEvent::Type::LeftButtonDown:
            io.MouseDown[0] = true;
            break;
        case MouseEvent::Type::LeftButtonUp:
            io.MouseDown[0] = false;
            break;
        case MouseEvent::Type::RightButtonDown:
            io.MouseDown[1] = true;
            break;
        case MouseEvent::Type::RightButtonUp:
            io.MouseDown[1] = false;
            break;
        case MouseEvent::Type::MiddleButtonDown:
            io.MouseDown[2] = true;
            break;
        case MouseEvent::Type::MiddleButtonUp:
            io.MouseDown[2] = false;
            break;
        case MouseEvent::Type::Move:
            io.MousePos.x = event.pos.x *io.DisplaySize.x;
            io.MousePos.y = event.pos.y *io.DisplaySize.y;
            break;
        case MouseEvent::Type::Wheel:
            io.MouseWheel += event.wheelDelta.x;
            break;
        }

        return false;
    }
}
