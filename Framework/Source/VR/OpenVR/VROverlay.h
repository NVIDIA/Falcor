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
#include "glm/mat4x4.hpp"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "Core/FBO.h"
#include "Core/RenderContext.h"
#include "Graphics/Program.h"
#include "Core/Texture.h"

// Forward declare OpenVR system class types to remove "openvr.h" dependencies from Falcor headers
namespace vr { 
    class IVRSystem;              // A generic system with basic API to talk with a VR system
    class IVRCompositor;          // A class that composite rendered results with appropriate distortion into the HMD
    class IVRRenderModels;        // A class giving access to renderable geometry for things like controllers
    class IVRChaperone;
    class IVRRenderModels;
    class IVROverlay;
    struct TrackedDevicePose_t;  
}

namespace Falcor
{
    // Forward declare the VRSystem class to avoid header cycles.
    class VRSystem;

    /** High-level OpenVR overlay abstraction
    */
    class VROverlay
    {
    public:
        // Declare standard Falcor shared pointer types.
        using SharedPtr = std::shared_ptr<VROverlay>;
        using SharedConstPtr = std::shared_ptr<const VROverlay>;

        // Constructor - The OpenVR Overlay class does not completely work with Falcor yet.
        static SharedPtr create(vr::IVROverlay* pIVROverlay);

        // create new overlay - The OpenVR Overlay class does not completely work with Falcor yet.
        void addOrUpdateOverlayTexture(const std::string& key, Texture::SharedPtr pTex);

        // show overlay - The OpenVR Overlay class does not completely work with Falcor yet.
        void show(const std::string& key);

        // hide overlay - The OpenVR Overlay class does not completely work with Falcor yet.
        void hide(const std::string& key);

    private:

        vr::IVROverlay* mpIVROverlay;

    };
}
