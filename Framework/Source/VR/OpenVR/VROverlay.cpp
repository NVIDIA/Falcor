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

#include "FalcorConfig.h"

#include "VRSystem.h"
#include "openvr.h"
#include "glm/glm.hpp"
#include "glm/gtx/transform.hpp"
#include "VROverlay.h"

using namespace Falcor;

// Private namespace
namespace 
{
    // Matrix to convert OpenVR matrix to OpenGL matrix.
    glm::mat4 convertOpenVRMatrix34(vr::HmdMatrix34_t mat)
    {
        return glm::mat4( mat.m[0][0], mat.m[1][0], mat.m[2][0], 0.0f, 
                          mat.m[0][1], mat.m[1][1], mat.m[2][1], 0.0f, 
                          mat.m[0][2], mat.m[1][2], mat.m[2][2], 0.0f, 
                          mat.m[0][3], mat.m[1][3], mat.m[2][3], 1.0f );
    }
}

typedef uint64_t VROverlayHandle_t;

VROverlay::SharedPtr VROverlay::create(vr::IVROverlay* pIVROverlay)
{
    SharedPtr pOverlay = SharedPtr( new VROverlay );
    pOverlay->mpIVROverlay = pIVROverlay;
    return pOverlay;
}


void VROverlay::addOrUpdateOverlayTexture(const std::string& key, Texture::SharedPtr pTex)
{
    VROverlayHandle_t handle;
    mpIVROverlay->CreateOverlay(key.c_str(), key.c_str(), &handle);

    vr::Texture_t subTex;
    subTex.handle = (void *)(size_t)pTex->getApiHandle();
    subTex.eColorSpace = isSrgbFormat(pTex->getFormat()) ? vr::EColorSpace::ColorSpace_Gamma : vr::EColorSpace::ColorSpace_Linear;
    subTex.eType = vr::GraphicsAPIConvention::API_OpenGL; //(vr::GraphicsAPIConvention)(API_OpenGL);

    mpIVROverlay->SetOverlayTexture(handle, &subTex);
}

void VROverlay::show(const std::string& key)
{
    VROverlayHandle_t handle;
    mpIVROverlay->FindOverlay(key.c_str(), &handle);

    if (handle == vr::k_ulOverlayHandleInvalid)
    {
        mpIVROverlay->ShowOverlay(handle);
    }

}

void VROverlay::hide(const std::string& key)
{
    VROverlayHandle_t handle;
    mpIVROverlay->FindOverlay(key.c_str(), &handle);

    if (handle == vr::k_ulOverlayHandleInvalid)
    {
        mpIVROverlay->HideOverlay(handle);
    }
}
