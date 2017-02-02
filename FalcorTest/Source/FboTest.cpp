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
#include "FboTest.h"
#include <stdio.h>

void FboTest::addTests()
{
    addTestToList<TestDefault>();
    addTestToList<TestCreate>();
    addTestToList<TestCaptureToFile>();
    addTestToList<TestDepthStencilAttach>();
    addTestToList<TestColorAttach>();
    addTestToList<TestZeroAttachment>();
    addTestToList<TestGetWidthHeight>();
    addTestToList<TestCreate2D>();
    addTestToList<TestCreateCubemap>();
}

TESTING_FUNC(FboTest, TestDefault)
{
    Fbo::SharedPtr fbo = Fbo::getDefault();
    if (fbo->getApiHandle() == -1)
    {
        return TestBase::TestData(TestBase::TestResult::Pass, mName);
    }
    else
    {
        return TestBase::TestData(TestBase::TestResult::Fail, mName, 
            "Error creating default fbo, api handle is initialized and should not be.");
    }
}

TESTING_FUNC(FboTest, TestCreate)
{
    Fbo::SharedPtr fbo = Fbo::create();
    //This is exactly the same as getDefault. It passes true for initapihandle but that param is unused in Fbo::Fbo(bool)
    if (fbo->getApiHandle() == -1)
    {
        return TestBase::TestData(TestBase::TestResult::Fail, mName,
            "Error creating default fbo, api handle isn't initialized and should be.");
    }
    else
    {
        return TestBase::TestData(TestBase::TestResult::Pass, mName);
    }
}

TESTING_FUNC(FboTest, TestCaptureToFile)
{
    //this will fail, capture to file currently does nothing. This might need changing depending on the implementation
    Fbo::SharedPtr fbo = Fbo::create();
    std::string executableDir = getExecutableDirectory();
    std::ifstream inFile;
    for (uint32_t i = 0; i < fbo->getMaxColorTargetCount(); ++i)
    {
        std::string pngFile;
        findAvailableFilename("FboTest", executableDir, "png", pngFile);
        fbo->captureToFile(i, pngFile, Bitmap::FileFormat::PngFile);
        inFile.open(pngFile, std::ios::in);
        if (inFile.is_open() && !inFile.fail())
        {
            inFile.close();
            remove(pngFile.c_str());
        }
        else
        {
            return TestBase::TestData(TestBase::TestResult::Fail, mName, 
                "(The tested function is not implemented) Error opening created png: " + pngFile);
        }
    }

    return TestBase::TestData(TestBase::TestResult::Pass, mName);
}

TESTING_FUNC(FboTest, TestDepthStencilAttach)
{
    //Also, attachDepthStencilTarget, the fx this is meant to test, calls applyDepthAttachment, which is unimplemented in d3d12
    const uint32_t texWidth = 800u;
    const uint32_t texHeight = 600u;
    Fbo::SharedPtr fbo = Fbo::create();
    Texture::SharedPtr tex = Texture::create2D(texWidth, texHeight, ResourceFormat::D32Float, 1u, 
        4294967295u, (const void*)nullptr, Resource::BindFlags::DepthStencil);
    fbo->attachDepthStencilTarget(tex);

    if (fbo->getDepthStencilTexture()->getWidth() != texWidth || 
        fbo->getDepthStencilTexture()->getHeight() != texHeight ||
        fbo->getDesc().getDepthStencilFormat() != ResourceFormat::D32Float ||
        fbo->getDesc().isDepthStencilUav() /*should be false without uav flag on texture*/)
    {
        return TestBase::TestData(TestBase::TestResult::Fail, mName,
            "Fbo properties are incorrect after attaching depth stencil texture");
    }

    //I have no idea why this causes a crash in texture. Obscure error message incorrect paramaters, can't be 
    //unordered access and depth stencil? attachDepthStencilTarget() seems to suggest otherwise

    //Texture::BindFlags flags = Texture::BindFlags::DepthStencil | Texture::BindFlags::UnorderedAccess;
    //tex = Texture::create2D(texWidth, texHeight, ResourceFormat::D16Unorm, 1u,
    //    4294967295u, (const void*)nullptr, flags);
    //fbo->attachDepthStencilTarget(tex, 0u, 0u, 0u);
    //if (fbo->getDesc().getDepthStencilFormat() != ResourceFormat::D16Unorm ||
    //    !fbo->getDesc().isDepthStencilUav() /*should be true with unordered access flag*/)
    //{
    //    return TestBase::TestData(TestBase::TestResult::Fail, mName,
    //        "Fbo properties are incorrect after attaching depth stencil texture with Unordered Access bind flag");
    //}

    fbo->attachDepthStencilTarget(nullptr, 0u, 0u, 0u);
    if (fbo->getDesc().getDepthStencilFormat() != ResourceFormat::Unknown ||
        fbo->getDesc().isDepthStencilUav() /*should be false from nullptr texture*/)
    {
        return TestBase::TestData(TestBase::TestResult::Fail, mName,
            "Fbo properties are incorrect after attaching depth stencil texture with nullptr");
    }

    return TestBase::TestData(TestBase::TestResult::Pass, mName);
}

TESTING_FUNC(FboTest, TestColorAttach)
{
    //Also, attachColorTarget, the fx this is meant to test, calls applyColorAttachment, which is unimplemented in d3d12
    Fbo::SharedPtr fbo = Fbo::create();
    for (uint32_t i = 0; i < Fbo::getMaxColorTargetCount(); ++i)
    {
        Resource::BindFlags flags = Resource::BindFlags::RenderTarget;
        if (i % 2 == 0)
        {
            flags |= Resource::BindFlags::UnorderedAccess;
        }

        bool loadAsSrgb = false;
        if (i % 3 == 0)
        {
            loadAsSrgb = true;
        }

        Texture::SharedPtr tex = createTextureFromFile("TestTex.png", false, loadAsSrgb, flags);
        fbo->attachColorTarget(tex, i);

        //Check uav
        if ((flags & Resource::BindFlags::UnorderedAccess) != Resource::BindFlags::None)
        {
            if (!fbo->getDesc().isColorTargetUav(i))
            {
                return TestBase::TestData(TestBase::TestResult::Fail, mName,
                    "Fbo color target missing uav flag despite texture being created with unordered access flag");
            }
        }
        else
        {
            if (fbo->getDesc().isColorTargetUav(i))
            {
                return TestBase::TestData(TestBase::TestResult::Fail, mName,
                    "Fbo color target has uav flag despite texture being created without unordered access flag");
            }
        }

        if (tex->getFormat() != fbo->getDesc().getColorTargetFormat(i))
        {
            return TestBase::TestData(TestBase::TestResult::Fail, mName,
                "Fbo color target format does not match the format of the attached texture");
        }
    }

    return TestBase::TestData(TestBase::TestResult::Pass, mName);
}

TESTING_FUNC(FboTest, TestZeroAttachment)
{
    return TestBase::TestData(TestBase::TestResult::Fail, mName, "This function is only implemented in GL");
}

TESTING_FUNC(FboTest, TestGetWidthHeight)
{
    //This fails, the logic in here tests as if verifyAttachment is being called but it's not being called
    Fbo::SharedPtr fbo = Fbo::create();
    const uint32_t numResolutions = 6;
    uint32_t widths[numResolutions] = { 1920u, 1440u, 1280u, 1600u, 1280u, 800u };
    uint32_t heights[numResolutions] = { 1080u, 900u, 800u, 1200u, 960u, 700u };
    for (size_t i = 0; i < numResolutions; i++)
    {
        Texture::SharedPtr tex = Texture::create2D(widths[i], heights[i], ResourceFormat::RGBA32Float, 1u, 
            4294967295u, (const void*)nullptr, Resource::BindFlags::RenderTarget);
        uint32_t curWidth = fbo->getWidth();
        uint32_t curHeight = fbo->getHeight();
        for (uint32_t j = 0; j < fbo->getMaxColorTargetCount(); ++j)
        {
            fbo->attachColorTarget(tex, j);
            curWidth = min(curWidth, widths[i]);
            curHeight = min(curHeight, heights[i]);
            if (fbo->getWidth() != curWidth || fbo->getHeight() != curHeight)
            {
                return TestBase::TestData(TestBase::TestResult::Fail, mName,
                    "Fbo dimensions do not match dimensions used to set");
            }
        }
    }

    return TestBase::TestData(TestBase::TestResult::Pass, mName);
}

TESTING_FUNC(FboTest, TestCreate2D)
{
    if (isStressCreateCorrect(true))
    {
        return TestBase::TestData(TestBase::TestResult::Pass, mName);
    }
    else
    {
        return TestBase::TestData(TestBase::TestResult::Fail, mName,
            "Fbo properties do not match properties used to create it");
    }
}

TESTING_FUNC(FboTest, TestCreateCubemap)
{
    if (isStressCreateCorrect(false))
    {
        return TestBase::TestData(TestBase::TestResult::Pass, mName);
    }
    else
    {
        return TestBase::TestData(TestBase::TestResult::Fail, mName,
            "Fbo properties do not match properties used to create it");
    }
}

bool FboTest::isStressCreateCorrect(bool randomSampleCount)
{
    const uint32_t numSampleCounts = randomSampleCount ? 10 : 1;
    const uint32_t numResolutions = 6;
    uint32_t widths[numResolutions] = { 1920u, 1440u, 1280u, 1600u, 1280u, 800u };
    uint32_t heights[numResolutions] = { 1080u, 900u, 800u, 1200u, 960u, 700u };
    const uint32_t numTestFormats = 3;
    ResourceFormat resources[numTestFormats] = { ResourceFormat::RGBA8UnormSrgb, ResourceFormat::RGBA32Float, ResourceFormat::RGBA32Uint };
    Fbo::Desc desc;

    //formats
    for (uint32_t i = 0; i < numTestFormats; ++i)
    {
        ResourceFormat format = static_cast<ResourceFormat>(i);
        //bool for isUav
        for (uint32_t j = 0; j < 2; ++j)
        {
            //generate a few random numbers for sample counts 
            for (uint32_t k = 0; k < numSampleCounts; ++k)
            {
                desc.setDepthStencilTarget(format, j != 0);
                randomSampleCount ? desc.setSampleCount(static_cast<uint32_t>(rand())) : desc.setSampleCount(1u);
                //set properties for all color targets
                for (uint32_t x = 0; x < Fbo::getMaxColorTargetCount(); ++x)
                {
                    desc.setColorTarget(x, format, j != 0);
                }
                //create fbos with created desc for each resolution
                for (uint32_t y = 0; y < numResolutions; ++y)
                {
                    for (uint32_t z = 0; z < numResolutions; ++z)
                    {
                        Fbo::SharedPtr fbo = FboHelper::create2D(widths[y], heights[z], desc);
                        if (!isFboCorrect(fbo, widths[y], heights[z], desc))
                        {
                            return false;
                        }
                    }
                }
            }
        }
    }

    return true;
}

bool FboTest::isFboCorrect(Fbo::SharedPtr fbo, const uint32_t& w, const uint32_t& h, const Fbo::Desc& desc)
{
    if (fbo->getWidth() != w || fbo->getHeight() != h || fbo->getDesc().getSampleCount() != desc.getSampleCount() ||
        fbo->getDesc().isDepthStencilUav() != desc.isDepthStencilUav() || 
        fbo->getDepthStencilTexture()->getFormat() != desc.getDepthStencilFormat())
    {
        return false;
    }

    for (uint32_t i = 0; i < Fbo::getMaxColorTargetCount(); ++i)
    {
        if (fbo->getColorTexture(i)->getFormat() != desc.getColorTargetFormat(i) ||
            fbo->getDesc().isColorTargetUav(i) != desc.isColorTargetUav(i))
        {
            return false;
        }
    }

    return true;
}

int main()
{
    FboTest fbot;
    fbot.init(true);
    fbot.run();
    return 0;
}