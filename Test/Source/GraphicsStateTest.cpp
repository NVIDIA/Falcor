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
#include "GraphicsStateTest.h"

//  
void GraphicsStateTest::addTests()
{
    addTestToList<TestAll>();

}

//
testing_func(GraphicsStateTest, TestAll)
{
    //  Create the Graphics State.
    GraphicsState::SharedPtr gps = GraphicsState::create();


    //
    Texture::SharedPtr colorTarget1 = Texture::create2D(1920, 1080, ResourceFormat::RGBA32Float, 1U, Texture::kMaxPossible, nullptr, Texture::BindFlags::RenderTarget);
    Texture::SharedPtr dsTarget1 = Texture::create2D(1920, 1080, ResourceFormat::D24UnormS8, 1U, Texture::kMaxPossible, nullptr, Texture::BindFlags::DepthStencil);
    
    //
    Texture::SharedPtr colorTarget2 = Texture::create2D(2560, 1440, ResourceFormat::RGBA32Float, 1U, Texture::kMaxPossible, nullptr, Texture::BindFlags::RenderTarget);
    Texture::SharedPtr dsTarget2 = Texture::create2D(2560, 1440, ResourceFormat::D24UnormS8, 1U, Texture::kMaxPossible, nullptr, Texture::BindFlags::DepthStencil);

    //  
    Fbo::SharedPtr fbo1 = Fbo::create();
    fbo1->attachColorTarget(colorTarget1, 0);
    fbo1->attachDepthStencilTarget(dsTarget1);

    //
    Fbo::SharedPtr fbo2 = Fbo::create();
    fbo1->attachColorTarget(colorTarget2, 0);
    fbo1->attachDepthStencilTarget(dsTarget2);

    bool fbo1set = false;
    bool fbo2set = false;

    //  
    gps->setFbo(fbo1);
    fbo1set = gps->getFbo()->getApiHandle() == fbo1->getApiHandle();

    //  
    fbo2set = gps->getFbo()->getApiHandle() == fbo2->getApiHandle();
    gps->setFbo(fbo2);

    //  
    if (!fbo1set || !fbo2set)
    {
        test_fail("Error : Setting FBO State Failed.");
    }

    fbo1set = false;
    fbo2set = false;

    //  
    gps->pushFbo(fbo1);
    gps->pushFbo(fbo2);
    fbo2set = gps->getFbo()->getApiHandle() == fbo2->getApiHandle();
    gps->popFbo();
    fbo1set = gps->getFbo()->getApiHandle() == fbo1->getApiHandle();
    gps->popFbo();


    if (!fbo1set || !fbo2set)
    {
        test_fail("Error : Push pop FBO State Failed.");
    }


    //
    glm::vec4 vf(0.5, 0.5, 0.5, 1.0);
    //
    std::vector<Buffer::SharedPtr> buffs;
    Buffer::SharedPtr newBuffs = Buffer::create(sizeof(glm::vec4), Resource::BindFlags::Constant, Buffer::CpuAccess::Write, &vf);
    buffs.push_back(newBuffs);

    VertexLayout::SharedPtr vl = VertexLayout::create();
    Vao::SharedPtr vao = Vao::create(buffs, vl, nullptr, ResourceFormat::RG32Uint, Vao::Topology::PointList);


    //
    glm::vec4 bf(0.0, 1.0, 0.3, 1.0);
    BlendState::Desc bsd = BlendState::Desc();
    bsd.setBlendFactor(glm::vec4(0.0, 1.0, 0.0, 0));

    //
    bool bsset = true;

    //  
    BlendState::SharedPtr bs = BlendState::create(bsd);
    gps->setBlendState(bs);
    bsset = bsset && gps->getBlendState()->getBlendFactor() == bf;
    bsset = bsset && gps->getBlendState()->getApiHandle() == bs->getApiHandle();

    if (!bsset)
    {
        test_fail("Error : Set Blend State Failed.");
    }





    return test_pass();
}


int main()
{
    GraphicsStateTest gpT;
    gpT.init(true);
    gpT.run();
    return 0;
}