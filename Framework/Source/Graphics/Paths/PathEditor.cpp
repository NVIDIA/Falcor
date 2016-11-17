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
    bool PathEditor::closeEditor(Gui* pGui)
    {
        if (pGui->addButton("Close Editor"))
        {
            pGui->popWindow();
            if (mEditCompleteCB)
            {
                mEditCompleteCB();
            }
            return true;
        }
        return false;
    }

    void PathEditor::editCameraProperties(Gui* pGui)
    {
        // Camera position
        vec3 p = mpCamera->getPosition();
        vec3 t = mpCamera->getTarget();
        vec3 u = mpCamera->getUpVector();
        if (pGui->addFloat3Var("Camera Position", p, -FLT_MAX, FLT_MAX))
        {
            mpCamera->setPosition(p);
        }

        if (pGui->addFloat3Var("Camera Target", t, -FLT_MAX, FLT_MAX))
        {
            mpCamera->setTarget(t);
        }

        if (pGui->addFloat3Var("Camera Up", u, -FLT_MAX, FLT_MAX))
        {
            mpCamera->setUpVector(u);
        }
    }

    void PathEditor::editActiveFrameID(Gui* pGui)
    {
        if(mpPath->getKeyFrameCount())
        {
            if (pGui->addIntVar("Active Frame", mActiveFrame, 0, mpPath->getKeyFrameCount()))
            {
                setActiveFrameID(mActiveFrame);
            }
        }
    }

    void PathEditor::setActiveFrameID(uint32_t id)
    {
        mActiveFrame = id;
        const auto& frame = mpPath->getKeyFrame(mActiveFrame);
        mpCamera->setUpVector(frame.up);
        mpCamera->setTarget(frame.target);
        mpCamera->setPosition(frame.position);
        mFrameTime = frame.time;
    }
    
    void PathEditor::editPathLoop(Gui* pGui)
    {
        bool loop = mpPath->isRepeatOn();
        if (pGui->addCheckBox("Loop Path", loop))
        {
            mpPath->setAnimationRepeat(loop);
        }
    }

    void PathEditor::editPathName(Gui* pGui)
    {
        char name[1024];
        strcpy_s(name, mpPath->getName().c_str());
        if (pGui->addTextBox("Path Name", name, arraysize(name)))
        {
            mpPath->setName(name);
        }
    }

    PathEditor::UniquePtr PathEditor::create(const ObjectPath::SharedPtr& pPath, const Camera::SharedPtr& pCamera, pfnEditComplete editCompleteCB)
    {
        return UniquePtr(new PathEditor(pPath, pCamera, editCompleteCB));
    }

    PathEditor::PathEditor(const ObjectPath::SharedPtr& pPath, const Camera::SharedPtr& pCamera, pfnEditComplete editCompleteCB) : 
        mEditCompleteCB(editCompleteCB), mpPath(pPath), mpCamera(pCamera)
    {
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

    void PathEditor::render(Gui* pGui)
    {
        pGui->pushWindow("Path Editor", 300, 250, 150, 400);
        if (closeEditor(pGui)) return;
        pGui->addSeparator();
        editPathName(pGui);
        editPathLoop(pGui);
        editActiveFrameID(pGui);

        addFrame(pGui);
        pGui->addSeparator();
        editFrameTime(pGui);
        pGui->addSeparator();
        updateFrame(pGui);
        deleteFrame(pGui);

        pGui->addSeparator();
        editCameraProperties(pGui);
        pGui->popWindow();
    }

    void PathEditor::editFrameTime(Gui* pGui)
    {
        pGui->addFloatVar("Frame Time", mFrameTime, 0, FLT_MAX);
    }

    void PathEditor::addFrame(Gui* pGui)
    {
        if(pGui->addButton("Add Frame"))
        {
            const auto& pos = mpCamera->getPosition();
            const auto& target = mpCamera->getTarget();
            const auto& up = mpCamera->getUpVector();
            mActiveFrame = mpPath->addKeyFrame(mFrameTime, pos, target, up);
            setActiveFrameID(mActiveFrame);
        }
    }

    void PathEditor::deleteFrame(Gui* pGui)
    {
        if(mpPath->getKeyFrameCount() && pGui->addButton("Remove Frame"))
        {
            mpPath->removeKeyFrame(mActiveFrame);
            mActiveFrame = min(mpPath->getKeyFrameCount() - 1, (uint32_t)mActiveFrame);
            if (mpPath->getKeyFrameCount())
            {
                setActiveFrameID(mActiveFrame);
            }
        }
    }

    void PathEditor::updateFrame(Gui* pGui)
    {
        if (mpPath->getKeyFrameCount() && pGui->addButton("Update Current Frame"))
        {
            const auto& pos = mpCamera->getPosition();
            const auto& target = mpCamera->getTarget();
            const auto& up = mpCamera->getUpVector();
            mpPath->setFramePosition(mActiveFrame, pos);
            mpPath->setFrameTarget(mActiveFrame, target);
            mpPath->setFrameUp(mActiveFrame, up);
            mActiveFrame = mpPath->setFrameTime(mActiveFrame, mFrameTime);
            setActiveFrameID(mActiveFrame);
        }
    }
}