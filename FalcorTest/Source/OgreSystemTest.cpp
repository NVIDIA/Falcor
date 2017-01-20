#include "OgreSystemTest.h"

OgreSystemTest::OgreSystemTest()
{
    REGISTER_NAME;
}

void OgreSystemTest::load()
{
    mpProgram = GraphicsProgram::createFromFile("", "SceneEditorSample.fs");
    std::string lights;
    getSceneLightString(mpScene.get(), lights);
    mpProgram->addDefine("_LIGHT_SOURCES", lights);
    mpGraphicsVars = GraphicsVars::create(mpProgram->getActiveVersion()->getReflector());
    mpGraphicsState = GraphicsState::create();
    mpGraphicsState->setProgram(mpProgram);
}

void OgreSystemTest::render()
{
    mpRenderContext->clearFbo(mpDefaultFBO.get(), glm::vec4(0.5f), 1.0f, 0, FboAttachmentType::All);
    mpGraphicsState->setFbo(mpDefaultFBO);
    mpGraphicsState->setProgram(mpProgram);
    mpRenderContext->setGraphicsState(mpGraphicsState);
    setSceneLightsIntoConstantBuffer(mpScene.get(), mpGraphicsVars["PerFrameCB"].get());
    mpRenderContext->setGraphicsVars(mpGraphicsVars);
    mpSceneRenderer->update(frameRate().getLastFrameTime());
    mpSceneRenderer->renderScene(mpRenderContext.get());
}

void OgreSystemTest::shutdown()
{}

int main()
{
    OgreSystemTest ost;
    SampleConfig config;
    ost.run(config);
    if (ost.CaughtException())
        return 1;
    else
        return 0;
}