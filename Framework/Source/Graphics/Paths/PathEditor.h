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
#include "Utils/Gui.h"
#include "Graphics/Paths/ObjectPath.h"
#include "Graphics/Camera/Camera.h"
#include "Utils/Gui.h"

namespace Falcor
{
    class PathEditor
    {
    public:
        using pfnEditComplete = void(*)(void*);
        using UniquePtr = std::unique_ptr<PathEditor>;
        using UniqueConstPtr = std::unique_ptr<const PathEditor>;

        static UniquePtr create(const ObjectPath::SharedPtr& pPath, const Camera::SharedPtr& pCamera, pfnEditComplete editCompleteCB, void* pUserData);
        ~PathEditor();

        void setCamera(const Camera::SharedPtr& pCamera);

        static void GUI_CALL closeEditorCB(void* pUserData);
        static void GUI_CALL addFrameCB(void* pUserData);
        static void GUI_CALL updateFrameCB(void* pUserData);
        static void GUI_CALL deleteFrameCB(void* pUserData);

        static void GUI_CALL getActiveFrameID(void* pVar, void* pUserData);
        static void GUI_CALL setActiveFrameID(const void* pVar, void* pUserData);

        static void GUI_CALL getPathLoop(void* pVar, void* pUserData);
        static void GUI_CALL setPathLoop(const void* pVar, void* pUserData);

        static void GUI_CALL getPathName(void* pVar, void* pUserData);
        static void GUI_CALL setPathName(const void* pVar, void* pUserData);

        template<uint32_t channel>
        static void GUI_CALL setCameraPositionCB(const void* pVal, void* pUserData);
        template<uint32_t channel>
        static void GUI_CALL getCameraPositionCB(void* pVal, void* pUserData);

        template<uint32_t channel>
        static void GUI_CALL setCameraTargetCB(const void* pVal, void* pUserData);
        template<uint32_t channel>
        static void GUI_CALL getCameraTargetCB(void* pVal, void* pUserData);

        template<uint32_t channel>
        static void GUI_CALL setCameraUpCB(const void* pVal, void* pUserData);
        template<uint32_t channel>
        static void GUI_CALL getCameraUpCB(void* pVal, void* pUserData);

    private:
        PathEditor(const ObjectPath::SharedPtr& pPath, const Camera::SharedPtr& pCamera, pfnEditComplete editCompleteCB, void* pUserData);

        void initUI();
        void addFrame();
        void updateFrame();
        void deleteFrame();

        void updateFrameCountUI();
        Gui::UniquePtr mpGui;
        ObjectPath::SharedPtr mpPath;
        Camera::SharedPtr mpCamera;
        pfnEditComplete mEditCompleteCB = nullptr;
        void* mpUserData = nullptr;

        int32_t mActiveFrame = 0;
        float mFrameTime = 0;
    };
}