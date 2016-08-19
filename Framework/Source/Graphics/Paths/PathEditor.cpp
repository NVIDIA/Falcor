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
#include "ObjectPath.h"
#include "PathEditor.h"
#include "Graphics/Camera/Camera.h"

namespace Falcor
{
    //////////////////////////////////////////////////////////////////////////
    // Callbacks
    //////////////////////////////////////////////////////////////////////////
    void PathEditor::closeEditorCB(void* pUserData)
    {
        PathEditor* pEditor = (PathEditor*)pUserData;
        // Finish our own business before calling the user callback, since he might destroy the CPathEditor object.
        pEditor->mpGui = nullptr;

        if(pEditor->mEditCompleteCB)
        {
            pEditor->mEditCompleteCB(pEditor->mpUserData);
        }
    }

    template<uint32_t channel>
    void PathEditor::getCameraPositionCB(void* pVal, void* pUserData)
    {
        PathEditor* pEditor = (PathEditor*)pUserData;
        auto pos = pEditor->mpCamera->getPosition();
        *(float*)pVal = pos[channel];
    }

    template<uint32_t channel>
    void PathEditor::setCameraPositionCB(const void* pVal, void* pUserData)
    {
        PathEditor* pEditor = (PathEditor*)pUserData;
        auto pos = pEditor->mpCamera->getPosition();
        pos[channel] = *(float*)pVal;
        pEditor->mpCamera->setPosition(pos);
    }

    template<uint32_t channel>
    void PathEditor::getCameraTargetCB(void* pVal, void* pUserData)
    {
        PathEditor* pEditor = (PathEditor*)pUserData;
        const auto pCamera = pEditor->mpCamera;
        *(float*)pVal = pCamera->getTargetPosition()[channel];
    }

    template<uint32_t channel>
    void PathEditor::setCameraTargetCB(const void* pVal, void* pUserData)
    {
        PathEditor* pEditor = (PathEditor*)pUserData;
        auto pCamera = pEditor->mpCamera;
        auto target = pCamera->getTargetPosition();
        target[channel] = *(float*)pVal;
        pCamera->setTarget(target);
    }

    template<uint32_t channel>
    void PathEditor::getCameraUpCB(void* pVal, void* pUserData)
    {
        PathEditor* pEditor = (PathEditor*)pUserData;
        const auto pCamera = pEditor->mpCamera;
        *(float*)pVal = pCamera->getUpVector()[channel];
    }

    template<uint32_t channel>
    void PathEditor::setCameraUpCB(const void* pVal, void* pUserData)
    {
        PathEditor* pEditor = (PathEditor*)pUserData;
        auto pCamera = pEditor->mpCamera;
        auto up = pCamera->getUpVector();
        up[channel] = *(float*)pVal;
        pCamera->setUpVector(up);
    }

    void PathEditor::getActiveFrameID(void* pVar, void* pUserData)
    {
        PathEditor* pEditor = (PathEditor*)pUserData;
        *(int32_t*)pVar = pEditor->mActiveFrame;
    }

    void PathEditor::setActiveFrameID(const void* pVar, void* pUserData)
    {
        PathEditor* pEditor = (PathEditor*)pUserData;
        pEditor->mActiveFrame = *(int32_t*)pVar;

        const auto& frame = pEditor->mpPath->getKeyFrame(pEditor->mActiveFrame);
        auto pCamera = pEditor->mpCamera;
        pCamera->setUpVector(frame.up);
        pCamera->setTarget(frame.target);
        pCamera->setPosition(frame.position);
        pEditor->mFrameTime = frame.time;
    }

    void PathEditor::addFrameCB(void* pUserData)
    {
        PathEditor* pEditor = (PathEditor*)pUserData;
        pEditor->addFrame();
    }

    void PathEditor::updateFrameCB(void* pUserData)
    {
        PathEditor* pEditor = (PathEditor*)pUserData;
        pEditor->updateFrame();
    }
    
    void PathEditor::deleteFrameCB(void* pUserData)
    {
        PathEditor* pEditor = (PathEditor*)pUserData;
        pEditor->deleteFrame();
    }

    void PathEditor::getPathLoop(void* pVar, void* pUserData)
    {
        PathEditor* pEditor = (PathEditor*)pUserData;
        *(bool*)pVar = pEditor->mpPath->isRepeatOn();
    }

    void PathEditor::setPathLoop(const void* pVar, void* pUserData)
    {
        PathEditor* pEditor = (PathEditor*)pUserData;
        pEditor->mpPath->setAnimationRepeat(*(bool*)pVar);
    }

    void PathEditor::getPathName(void* pVar, void* pUserData)
    {
        PathEditor* pEditor = (PathEditor*)pUserData;
        *(std::string*)pVar = pEditor->mpPath->getName();
    }

    void PathEditor::setPathName(const void* pVar, void* pUserData)
    {
        PathEditor* pEditor = (PathEditor*)pUserData;
        pEditor->mpPath->setName(*(std::string*)pVar);
    }
    //////////////////////////////////////////////////////////////////////////
    // Callbacks end
    //////////////////////////////////////////////////////////////////////////
    PathEditor::UniquePtr PathEditor::create(const ObjectPath::SharedPtr& pPath, const Camera::SharedPtr& pCamera, pfnEditComplete editCompleteCB, void* pUserData)
    {
        return UniquePtr(new PathEditor(pPath, pCamera, editCompleteCB, pUserData));
    }

    PathEditor::PathEditor(const ObjectPath::SharedPtr& pPath, const Camera::SharedPtr& pCamera, pfnEditComplete editCompleteCB, void* pUserData) : 
        mEditCompleteCB(editCompleteCB), mpUserData(pUserData), mpPath(pPath), mpCamera(pCamera)
    {
        initUI();
        if(mpPath->getKeyFrameCount())
        {
            mFrameTime = mpPath->getKeyFrame(0).time;
        }
    }

    PathEditor::~PathEditor()
    {
    }

    void PathEditor::setCamera(const Camera::SharedPtr& pCamera)
    {
        mpCamera = pCamera;
        const auto& Frame = mpPath->getKeyFrame(mActiveFrame);
        mpCamera->setUpVector(Frame.up);
        mpCamera->setTarget(Frame.target);
        mpCamera->setPosition(Frame.position);
    }

    void PathEditor::initUI()
    {
        mpGui = Gui::create("Path Editor");
        mpGui->setSize(300, 250);

        mpGui->addButton("Close Editor", &PathEditor::closeEditorCB, this);
        mpGui->addSeparator();

        mpGui->addTextBoxWithCallback("Path Name", &PathEditor::setPathName, &PathEditor::getPathName, this);
        mpGui->addCheckBoxWithCallback("Loop Path", &PathEditor::setPathLoop, &PathEditor::getPathLoop, this);

        mpGui->addIntVarWithCallback("Active Frame", &PathEditor::setActiveFrameID, &PathEditor::getActiveFrameID, this);
        updateFrameCountUI();

        mpGui->addButton("Add Frame", &PathEditor::addFrameCB, this);
        mpGui->addSeparator();
        mpGui->addFloatVar("Frame Time", &mFrameTime, "", 0, FLT_MAX);
        mpGui->addSeparator();
        mpGui->addButton("Update Current Frame", &PathEditor::updateFrameCB, this);
        mpGui->addButton("Remove Frame", &PathEditor::deleteFrameCB, this);

        mpGui->addSeparator();

        // Camera position
        const std::string kPosStr("Camera Position");
        mpGui->addFloatVarWithCallback("x", &PathEditor::setCameraPositionCB<0>, &PathEditor::getCameraPositionCB<0>, this, kPosStr);
        mpGui->addFloatVarWithCallback("y", &PathEditor::setCameraPositionCB<1>, &PathEditor::getCameraPositionCB<1>, this, kPosStr);
        mpGui->addFloatVarWithCallback("z", &PathEditor::setCameraPositionCB<2>, &PathEditor::getCameraPositionCB<2>, this, kPosStr);

        // Target
        const std::string kTargetStr("Camera Target");
        mpGui->addFloatVarWithCallback("x", &PathEditor::setCameraTargetCB<0>, &PathEditor::getCameraTargetCB<0>, this, kTargetStr);
        mpGui->addFloatVarWithCallback("y", &PathEditor::setCameraTargetCB<1>, &PathEditor::getCameraTargetCB<1>, this, kTargetStr);
        mpGui->addFloatVarWithCallback("z", &PathEditor::setCameraTargetCB<2>, &PathEditor::getCameraTargetCB<2>, this, kTargetStr);

        // Up
        const std::string kUpStr("Camera Up");
        mpGui->addFloatVarWithCallback("x", &PathEditor::setCameraUpCB<0>, &PathEditor::getCameraUpCB<0>, this, kUpStr);
        mpGui->addFloatVarWithCallback("y", &PathEditor::setCameraUpCB<1>, &PathEditor::getCameraUpCB<1>, this, kUpStr);
        mpGui->addFloatVarWithCallback("z", &PathEditor::setCameraUpCB<2>, &PathEditor::getCameraUpCB<2>, this, kUpStr);
    }

    void PathEditor::updateFrameCountUI()
    {
        bool bVisible = (mpPath->getKeyFrameCount() != 0);
        mpGui->setVarRange("Active Frame", "", 0, mpPath->getKeyFrameCount() - 1);
        mpGui->setVarVisibility("Active Frame", "", bVisible);
        mpGui->setVarVisibility("Update Current Frame", "", bVisible);
        mpGui->setVarVisibility("Remove Frame", "", bVisible);
    }

    void PathEditor::addFrame()
    {
        const auto& pos = mpCamera->getPosition();
        const auto& target = mpCamera->getTargetPosition();
        const auto& up = mpCamera->getUpVector();
        mActiveFrame = mpPath->addKeyFrame(mFrameTime, pos, target, up);
        setActiveFrameID(&mActiveFrame, this);
        updateFrameCountUI();
    }

    void PathEditor::deleteFrame()
    {
        mpPath->removeKeyFrame(mActiveFrame);
        mActiveFrame = min(mpPath->getKeyFrameCount() - 1, (uint32_t)mActiveFrame);
        if(mpPath->getKeyFrameCount())
        {
            setActiveFrameID(&mActiveFrame, this);
        }
        updateFrameCountUI();
    }

    void PathEditor::updateFrame()
    {
        const auto& pos = mpCamera->getPosition();
        const auto& target = mpCamera->getTargetPosition();
        const auto& up = mpCamera->getUpVector();
        mpPath->setFramePosition(mActiveFrame, pos);
        mpPath->setFrameTarget(mActiveFrame, target);
        mpPath->setFrameUp(mActiveFrame, up);
        mActiveFrame = mpPath->setFrameTime(mActiveFrame, mFrameTime);
        setActiveFrameID(&mActiveFrame, this);
        updateFrameCountUI();
    }
}