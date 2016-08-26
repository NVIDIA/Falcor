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
#include "CSM.h"
#include "Graphics/Scene/SceneRenderer.h"
#include "glm/gtx/transform.hpp"
#include "Utils/Math/FalcorMath.h"
#include "Graphics/FboHelper.h"

//#define _ALPHA_FROM_ALBEDO_MAP
namespace Falcor
{
    const char* kDepthPassVSFile = "Effects/ShadowPass.vs";
    const char* kDepthPassGsFile = "Effects/ShadowPass.gs";
    const char* kDepthPassFsFile = "Effects/ShadowPass.fs";
    const char* kSdsmMinMaxFile = "Effects/SDSMMinMax.fs";

    class CsmSceneRenderer : public SceneRenderer
    {
    public:
        using UniquePtr = std::unique_ptr<CsmSceneRenderer>;
        static UniquePtr create(const Scene::SharedPtr& pScene, UniformBuffer::SharedPtr pAlphaMapUbo) { return UniquePtr(new CsmSceneRenderer(pScene, pAlphaMapUbo)); }

    protected:
        CsmSceneRenderer(const Scene::SharedPtr& pScene, UniformBuffer::SharedPtr pAlphaMapUbo) : SceneRenderer(pScene), mpAlphaMapUbo(pAlphaMapUbo) { setObjectCullState(false); }
        UniformBuffer::SharedPtr mpAlphaMapUbo;
        bool mMaterialChanged = false;
        bool setPerMaterialData(RenderContext* pContext, const CurrentWorkingData& currentData) override
        {
            if(mpLastMaterial != currentData.pMaterial)
            {
                mMaterialChanged = true;
                mpLastMaterial = currentData.pMaterial;
                struct alphaData
                {
                    uint64_t map;
                    float val;
                } a;

                mpLastMaterial->bindTextures();
#ifdef _ALPHA_FROM_ALBEDO_MAP
                const auto& pLayer = mpLastMaterial->getLayer(0);
                a.map = pLayer ? pLayer->albedo.texture.gpuAddress : 0;
                a.val = 0.5f;
#else
                a.map = mpLastMaterial->getAlphaValue().texture.ptr;
                a.val = mpLastMaterial->getAlphaValue().constantColor.x;
#endif
                mpAlphaMapUbo->setBlob(&a, 0, sizeof(a));
            }
            return true;
        };

        void postFlushDraw(RenderContext* pContext, const CurrentWorkingData& currentData) override
        {
            if(mUnloadTexturesOnMaterialChange && mMaterialChanged)
            {
                mpLastMaterial->unloadTextures();
                mMaterialChanged = false;
            }
        }
    };

    void createShadowMatrix(const DirectionalLight* pLight, const glm::vec3& center, float radius, glm::mat4& shadowVP)
    {
        glm::mat4 view = glm::lookAt(center, center + pLight->getWorldDirection(), glm::vec3(0, 1, 0));
        glm::mat4 proj = orthographicMatrix(-radius, radius, -radius, radius, -radius, radius);

        shadowVP = proj * view;
    }

    void createShadowMatrix(const PointLight* pLight, const glm::vec3& center, float radius, float fboAspectRatio, glm::mat4& shadowVP)
    {
        const glm::vec3 lightPos = pLight->getWorldPosition();
        const glm::vec3 lookat = pLight->getWorldDirection() + lightPos;
        glm::vec3 up(0, 1, 0);
        if(abs(glm::dot(up, pLight->getWorldDirection())) >= 0.95f)
        {
            up = glm::vec3(1, 0, 0);
        }
        glm::mat4 view = glm::lookAt(lightPos, lookat, up);
        float distFromCenter = glm::length(lightPos - center);
        float nearZ = max(0.01f, distFromCenter - radius);
        float maxZ = min(radius * 2, distFromCenter + radius);
        float angle = pLight->getOpeningAngle() * 2; // FIXME: Why 3? In theory, 2 should be the right number
        glm::mat4 proj = perspectiveMatrix(angle, fboAspectRatio, 0.1f, 10);

        shadowVP = proj * view;
    }

    void createShadowMatrix(const Light* pLight, const glm::vec3& center, float radius, float fboAspectRatio, glm::mat4& shadowVP)
    {
        switch(pLight->getType())
        {
        case LightDirectional:
            return createShadowMatrix((DirectionalLight*)pLight, center, radius, shadowVP);
        case LightPoint:
            return createShadowMatrix((PointLight*)pLight, center, radius, fboAspectRatio, shadowVP);
        default:
            should_not_get_here();
        }
    }

    CascadedShadowMaps::~CascadedShadowMaps() = default;

    CascadedShadowMaps::CascadedShadowMaps(uint32_t mapWidth, uint32_t mapHeight, Light::SharedConstPtr pLight, Scene::SharedPtr pScene, uint32_t cascadeCount, ResourceFormat shadowMapFormat) : mpLight(pLight), mpScene(pScene)
    {
        // FIXME: CSM and point lights don't work together, got some issues between cascades
        if(mpLight->getType() != LightDirectional)
        {
            cascadeCount = 1;
        }
        mCsmData.cascadeCount = cascadeCount;
        createShadowPassResources(mapWidth, mapHeight);
        mDepthPass.pProg = Program::createFromFile(kDepthPassVSFile, "");
        mDepthPass.pProg->addDefine("_APPLY_PROJECTION");

        mpLightCamera = Camera::create();
        RasterizerState::Desc rsDesc;
        rsDesc.setDepthClamp(true);
        mShadowPass.pDepthClampRS = RasterizerState::create(rsDesc);

        Sampler::Desc samplerDesc;
        samplerDesc.setFilterMode(Sampler::Filter::Point, Sampler::Filter::Point, Sampler::Filter::Point).setAddressingMode(Sampler::AddressMode::Border, Sampler::AddressMode::Border, Sampler::AddressMode::Border).setBorderColor(glm::vec4(1.0f));
        samplerDesc.setLodParams(0.f, 0.f, 0.f);
        samplerDesc.setComparisonMode(Sampler::ComparisonMode::LessEqual);
        mShadowPass.pPointCmpSampler = Sampler::create(samplerDesc);
        samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
        mShadowPass.pLinearCmpSampler = Sampler::create(samplerDesc);
        samplerDesc.setComparisonMode(Sampler::ComparisonMode::NoComparison);
        samplerDesc.setAddressingMode(Sampler::AddressMode::Clamp, Sampler::AddressMode::Clamp, Sampler::AddressMode::Clamp);
        samplerDesc.setLodParams(-100.f, 100.f, 0.f);

        createVsmSampleState(1);
        createGaussianBlurTech();
    }

    void CascadedShadowMaps::createGaussianBlurTech()
    {
        mpGaussianBlur = GaussianBlur::create(mCsmData.sampleKernelSize);
    }

    CascadedShadowMaps::UniquePtr CascadedShadowMaps::create(uint32_t mapWidth, uint32_t mapHeight, Light::SharedConstPtr pLight, Scene::SharedPtr pScene, uint32_t cascadeCount, ResourceFormat shadowMapFormat)
    {
        if(isDepthFormat(shadowMapFormat) == false)
        {
            Logger::log(Logger::Level::Error, std::string("Can't create CascadedShadowMaps effect. Requested resource format ") + to_string(shadowMapFormat) + " is not a depth format", true);
        }

        CascadedShadowMaps* pCsm = new CascadedShadowMaps(mapWidth, mapHeight, pLight, pScene, cascadeCount, shadowMapFormat);
        return CascadedShadowMaps::UniquePtr(pCsm);
    }

    void CascadedShadowMaps::createSdsmData(const Texture* pTexture)
    {
        // Check if we actually need to create it
        if(pTexture)
        {
            // Got a texture, if we already have a reduction check if the dimensions changes
            if(mSdsmData.minMaxReduction)
            {
                if(mSdsmData.width == pTexture->getWidth() && mSdsmData.height == pTexture->getHeight())
                {
                    // No need to do anything. That's the only time we don't create the reduction
                    return;
                }
            }
            mSdsmData.width = pTexture->getWidth();
            mSdsmData.height = pTexture->getHeight();
        }

        mSdsmData.minMaxReduction = ParallelReduction::create(ParallelReduction::Type::MinMax, mSdsmData.readbackLatency, mSdsmData.width, mSdsmData.height);

    }

    void CascadedShadowMaps::createShadowPassResources(uint32_t mapWidth, uint32_t mapHeight)
    {
        mShadowPass.mapSize = glm::vec2(float(mapWidth), float(mapHeight));
        const ResourceFormat depthFormat = ResourceFormat::D32Float;
        Program::DefineList progDef;
        progDef.add("_CASCADE_COUNT", std::to_string(mCsmData.cascadeCount));

#ifdef _ALPHA_FROM_ALBEDO_MAP
        progDef.add("_ALPHA_CHANNEL", "a");
#else
        progDef.add("_ALPHA_CHANNEL", "r");
#endif
        ResourceFormat colorFormat = ResourceFormat::Unknown;
        switch(mCsmData.filterMode)
        {
        case CsmFilterVsm:
            colorFormat = ResourceFormat::RG32Float;
            progDef.add("_VSM");
            break;
        case CsmFilterEvsm2:
            colorFormat = ResourceFormat::RG32Float;
            progDef.add("_EVSM2");
            break;
        case CsmFilterEvsm4:
            colorFormat = ResourceFormat::RGBA32Float;
            progDef.add("_EVSM4");
            break;
        default:
            mShadowPass.pFbo = FboHelper::createDepthOnly(mapWidth, mapHeight, depthFormat, mCsmData.cascadeCount);
        }

        if(colorFormat != ResourceFormat::Unknown)
        {
            mShadowPass.pFbo = FboHelper::create2DWithDepth(mapWidth, mapHeight, &colorFormat, depthFormat, mCsmData.cascadeCount, 1, 0, Texture::kEntireMipChain);
        }

        mShadowPass.fboAspectRatio = (float)mapWidth / (float)mapHeight;

        // Create the program
        mShadowPass.pProg = Program::createFromFile(kDepthPassVSFile, kDepthPassFsFile, kDepthPassGsFile, "", "", progDef);
        mShadowPass.pLightUbo = UniformBuffer::create(mShadowPass.pProg->getActiveProgramVersion().get(), "PerLightCB");
        mShadowPass.pAlphaUbo = UniformBuffer::create(mShadowPass.pProg->getActiveProgramVersion().get(), "AlphaMapCB");

        mpSceneRenderer = CsmSceneRenderer::create(mpScene, mShadowPass.pAlphaUbo);
    }

    void CascadedShadowMaps::setCascadeCount(uint32_t cascadeCount)
    {
        if(mpLight->getType() != LightDirectional)
        {
            cascadeCount = 1;
        }
        mCsmData.cascadeCount = cascadeCount;
        createShadowPassResources(mShadowPass.pFbo->getWidth(), mShadowPass.pFbo->getHeight());
    }

    void CascadedShadowMaps::setUiElements(Gui* pGui, const std::string& uiGroup)
    {
        Gui::dropdown_list filterMode;
        filterMode.push_back({(uint32_t)CsmFilterPoint, "Point"});
        filterMode.push_back({(uint32_t)CsmFilterHwPcf, "2x2 HW PCF"});
        filterMode.push_back({(uint32_t)CsmFilterFixedPcf, "Fixed-Size PCF"});
        filterMode.push_back({(uint32_t)CsmFilterVsm, "VSM"});
        filterMode.push_back({(uint32_t)CsmFilterEvsm2, "EVSM2"});
        filterMode.push_back({(uint32_t)CsmFilterEvsm4, "EVSM4"});
        filterMode.push_back({(uint32_t)CsmFilterStochasticPcf, "Stochastic Poisson PSF"});
        pGui->addDropdownWithCallback("Filter Mode", filterMode, setFilterModeCB, getFilterModeCB, this, uiGroup);
        pGui->addIntVarWithCallback("Max Kernel Size", &CascadedShadowMaps::setFilterKernelSizeCB, &CascadedShadowMaps::getFilterKernelSizeCB, this, uiGroup);

        Gui::dropdown_list partitionMode;
        partitionMode.push_back({(uint32_t)PartitionMode::Linear, "Linear"});
        partitionMode.push_back({(uint32_t)PartitionMode::Logarithmic, "Logarithmic"});
        partitionMode.push_back({(uint32_t)PartitionMode::PSSM, "PSSM"});
        pGui->addDropdown("Partition Mode", partitionMode, &mControls.partitionMode, uiGroup);
        pGui->addFloatVar("PSSM Lambda", &mControls.pssmLambda, uiGroup, 0, 1.0f);

        // SDSM data
        const char* sdsmGroup = "SDSM MinMax";
        pGui->addCheckBox("Enable", &mControls.useMinMaxSdsm, sdsmGroup);
        pGui->addIntVarWithCallback("Readback Latency", setSdsmReadbackLatency, getSdsmReadbackLatency, this, sdsmGroup, 0);
        pGui->nestGroups(uiGroup, sdsmGroup);

        // Controls
        const char* manualSettingsGroup = "Manual Settings";
        pGui->addFloatVar("Min Distance", &mControls.distanceRange.x, manualSettingsGroup, 0, 1);
        pGui->addFloatVar("Max Distance", &mControls.distanceRange.y, manualSettingsGroup, 0, 1);
        pGui->addFloatVar("Depth Bias", &mCsmData.depthBias, manualSettingsGroup, 0, FLT_MAX, 0.0001f);
        pGui->addCheckBox("Depth Clamp", &mControls.depthClamp, manualSettingsGroup);
        pGui->addCheckBox("Stabilize Cascades", &mControls.stabilizeCascades, manualSettingsGroup);
        pGui->addCheckBox("Concentric Cascades", &mControls.concentricCascades, manualSettingsGroup);
        pGui->addFloatVar("Cascade Blend Threshold", &mCsmData.cascadeBlendThreshold, manualSettingsGroup, 0, 1.0f);
        pGui->nestGroups(uiGroup, manualSettingsGroup);

        const char* vsmGroup = "VSM/EVSM";
        Gui::dropdown_list vsmMaxAniso;
        vsmMaxAniso.push_back({(uint32_t)1, "1"});
        vsmMaxAniso.push_back({(uint32_t)2, "2"});
        vsmMaxAniso.push_back({(uint32_t)4, "4"});
        vsmMaxAniso.push_back({(uint32_t)8, "8"});
        vsmMaxAniso.push_back({(uint32_t)16, "16"});
        pGui->addDropdownWithCallback("Max Aniso", vsmMaxAniso, &CascadedShadowMaps::setVsmAnisotropyCB, &CascadedShadowMaps::getVsmAnisotropyCB, this, vsmGroup);
        pGui->addFloatVar("Light Bleed Reduction", &mCsmData.lightBleedingReduction, vsmGroup, 0, 1.0f, 0.01f);
        const char* evsmExpGroup = "EVSM Exp";
        pGui->addFloatVar("Positive", &mCsmData.evsmExponents.x, evsmExpGroup, 0.0f, 42.0f, 0.01f);
        pGui->addFloatVar("Negative", &mCsmData.evsmExponents.y, evsmExpGroup, 0.0f, 42.0f, 0.01f);
        pGui->nestGroups(vsmGroup, evsmExpGroup);
        pGui->nestGroups(uiGroup, vsmGroup);
    }

    void camClipSpaceToWorldSpace(const Camera* pCamera, glm::vec3 viewFrustum[8], glm::vec3& center, float& radius)
    {
        glm::vec3 clipSpace[8] =
        {
            glm::vec3(-1.0f, 1.0f, 0),
            glm::vec3(1.0f, 1.0f, 0),
            glm::vec3(1.0f, -1.0f, 0),
            glm::vec3(-1.0f, -1.0f, 0),
            glm::vec3(-1.0f, 1.0f, 1.0f),
            glm::vec3(1.0f, 1.0f, 1.0f),
            glm::vec3(1.0f, -1.0f, 1.0f),
            glm::vec3(-1.0f, -1.0f, 1.0f),
        };

        glm::mat4 invViewProj = pCamera->getInvViewProjMatrix();
        center = glm::vec3(0, 0, 0);

        for(uint32_t i = 0; i < 8; i++)
        {
            glm::vec4 crd = invViewProj * glm::vec4(clipSpace[i], 1);
            viewFrustum[i] = glm::vec3(crd) / crd.w;
            center += viewFrustum[i];
        }

        center *= 1.0f / 8.0f;

        // Calculate bounding sphere radius
        radius = 0;
        for(uint32_t i = 0; i < 8; i++)
        {
            float d = glm::length(center - viewFrustum[i]);
            radius = max(d, radius);
        }
    }

    __forceinline float calcPssmPartitionEnd(float nearPlane, float camDepthRange, const glm::vec2& distanceRange, float linearBlend, uint32_t cascade, uint32_t cascadeCount)
    {
        // Convert to camera space
        float minDepth = nearPlane + distanceRange.x * camDepthRange;
        float maxDepth = nearPlane + distanceRange.y * camDepthRange;

        float depthRange = maxDepth - minDepth;
        float depthScale = maxDepth / minDepth;

        float cascadeScale = float(cascade + 1) / float(cascadeCount);
        float logSplit = pow(depthScale, cascadeScale) * minDepth;
        float uniSplit = minDepth + depthRange * cascadeScale;

        float distance = linearBlend * logSplit + (1 - linearBlend) * uniSplit;

        // Convert back to clip-space
        distance = (distance - nearPlane) / camDepthRange;
        return distance;
    }

    void getCascadeCropParams(const glm::vec3 crd[8], const glm::mat4& lightVP, glm::vec4& scale, glm::vec4& offset)
    {
        // Transform the frustum into light clip-space and calculate min-max
        glm::vec4 maxCS(-1, -1, 0, 1);
        glm::vec4 minCS(1, 1, 1, 1);
        for(uint32_t i = 0; i < 8; i++)
        {
            glm::vec4 c = lightVP * glm::vec4(crd[i], 1.0f);
            c /= c.w;
            maxCS = max(maxCS, c);
            minCS = min(minCS, c);
        }

        glm::vec4 delta = maxCS - minCS;
        scale = glm::vec4(2, 2, 1, 1) / delta;

        offset.x = -0.5f * (maxCS.x + minCS.x) * scale.x;
        offset.y = -0.5f * (maxCS.y + minCS.y) * scale.y;
        offset.z = -minCS.z * scale.z;

        scale.w = 1;
        offset.w = 0;
    }

    void CascadedShadowMaps::partitionCascades(const Camera* pCamera, const glm::vec2& distanceRange)
    {
        struct
        {
            glm::vec3 crd[8];
            glm::vec3 center;
            float radius;
        } camFrustum;

        camClipSpaceToWorldSpace(pCamera, camFrustum.crd, camFrustum.center, camFrustum.radius);

        // Create the global shadow space
        createShadowMatrix(mpLight.get(), camFrustum.center, camFrustum.radius, mShadowPass.fboAspectRatio, mCsmData.globalMat);

        if(mCsmData.cascadeCount == 1)
        {
            mCsmData.cascadeScale[0] = glm::vec4(1);
            mCsmData.cascadeOffset[0] = glm::vec4(0);
            mCsmData.cascadeStartDepth[0] = 0;
            mCsmData.cascadeRange[0] = 1;
            return;
        }

        float nearPlane = pCamera->getNearPlane();
        float farPlane = pCamera->getFarPlane();
        float depthRange = farPlane - nearPlane;

        float cascadeEnd = 0;

        for(int32_t c = 0; c < mCsmData.cascadeCount; c++)
        {
            float cascadeStart = (c == 0) ? distanceRange.x : cascadeEnd;

            switch(mControls.partitionMode)
            {
            case PartitionMode::Linear:
                cascadeEnd = cascadeStart + (distanceRange.y - distanceRange.x) / float(mCsmData.cascadeCount);
                break;
            case PartitionMode::Logarithmic:
                cascadeEnd = calcPssmPartitionEnd(nearPlane, depthRange, distanceRange, 1.0f, c, mCsmData.cascadeCount);
                break;
            case PartitionMode::PSSM:
                cascadeEnd = calcPssmPartitionEnd(nearPlane, depthRange, distanceRange, mControls.pssmLambda, c, mCsmData.cascadeCount);
                break;
            default:
                should_not_get_here();
            }

            // Calculate the cascade distance in camera-clip space
            mCsmData.cascadeStartDepth[c] = depthRange * cascadeStart + nearPlane;
            mCsmData.cascadeRange[c] = (depthRange * cascadeEnd + nearPlane) - mCsmData.cascadeStartDepth[c];
            // Calculate the cascade frustum
            glm::vec3 cascadeFrust[8];
            for(uint32_t i = 0; i < 4; i++)
            {
                glm::vec3 edge = camFrustum.crd[i + 4] - camFrustum.crd[i];
                glm::vec3 start = edge * cascadeStart;
                glm::vec3 end = edge * cascadeEnd;
                cascadeFrust[i] = camFrustum.crd[i] + start;
                cascadeFrust[i + 4] = camFrustum.crd[i] + end;
            }

            getCascadeCropParams(cascadeFrust, mCsmData.globalMat, mCsmData.cascadeScale[c], mCsmData.cascadeOffset[c]);
        }
    }

    void CascadedShadowMaps::renderScene(RenderContext* pCtx)
    {
        mShadowPass.pLightUbo->setBlob(&mCsmData, 0, sizeof(mCsmData));
        pCtx->setUniformBuffer(0, mShadowPass.pLightUbo);
        pCtx->setUniformBuffer(1, mShadowPass.pAlphaUbo);
        mShadowPass.pAlphaUbo->setVariable("evsmExp", mCsmData.evsmExponents);
        mpSceneRenderer->renderScene(pCtx, mShadowPass.pProg.get(), mpLightCamera.get());
    }

    void CascadedShadowMaps::executeDepthPass(RenderContext* pCtx, const Camera* pCamera)
    {
        // Must have an FBO attached, otherwise don't know the size of the depth map
        uint32_t width = pCtx->getFbo()->getWidth();
        uint32_t height = pCtx->getFbo()->getHeight();

        if((mDepthPass.pFbo == nullptr) || (mDepthPass.pFbo->getWidth() != width) || (mDepthPass.pFbo->getHeight() != height))
        {
            mDepthPass.pFbo = FboHelper::createDepthOnly(width, height, mShadowPass.pFbo->getDepthStencilTexture()->getFormat());
        }

        mDepthPass.pFbo->clearDepthStencil(1, 0, true, false);
        pCtx->pushFbo(mDepthPass.pFbo);

        mpSceneRenderer->setObjectCullState(true);
        mpSceneRenderer->renderScene(pCtx, mDepthPass.pProg.get(), const_cast<Camera*>(pCamera));
        mpSceneRenderer->setObjectCullState(false);
        pCtx->popFbo();
    }

    void CascadedShadowMaps::createVsmSampleState(uint32_t maxAnisotropy)
    {
        if(mShadowPass.pVSMTrilinearSampler)
        {
            const auto pTexture = mShadowPass.pFbo->getColorTexture(0);
            if(pTexture)
            {
                pTexture->makeNonResident(mShadowPass.pVSMTrilinearSampler.get());
            }
        }
        Sampler::Desc samplerDesc;
        samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
        samplerDesc.setAddressingMode(Sampler::AddressMode::Clamp, Sampler::AddressMode::Clamp, Sampler::AddressMode::Clamp);
        samplerDesc.setMaxAnisotropy(maxAnisotropy);
        mShadowPass.pVSMTrilinearSampler = Sampler::create(samplerDesc);
    }

    void CascadedShadowMaps::reduceDepthSdsmMinMax(RenderContext* pRenderCtx, const Camera* pCamera, const Texture* pDepthBuffer, glm::vec2& distanceRange)
    {
        if(pDepthBuffer == nullptr)
        {
            // Run a shadow pass
            executeDepthPass(pRenderCtx, pCamera);
            pDepthBuffer = mDepthPass.pFbo->getDepthStencilTexture().get();
        }

        createSdsmData(pDepthBuffer);
        distanceRange = glm::vec2(mSdsmData.minMaxReduction->reduce(pRenderCtx, pDepthBuffer));

        // Convert to linear
        glm::mat4 camProj = pCamera->getProjMatrix();
        distanceRange = distanceRange*camProj[2][3] - camProj[2][2];
        distanceRange = camProj[3][2] / distanceRange;
        distanceRange = (distanceRange - pCamera->getNearPlane()) / (pCamera->getNearPlane() - pCamera->getFarPlane());
        distanceRange = glm::clamp(distanceRange, glm::vec2(0), glm::vec2(1));

        //         if(mControls.stabilizeCascades)
        //         {
        //             // Ignore minor changes that can result in swimming
        //             distanceRange = round(distanceRange * 16.0f) / 16.0f;
        //             distanceRange.y = max(distanceRange.y, 0.005f);
        //         }
    }

    void CascadedShadowMaps::calcDistanceRange(RenderContext* pRenderCtx, const Camera* pCamera, const Texture* pDepthBuffer, glm::vec2& distanceRange)
    {
        if(mControls.useMinMaxSdsm)
        {
            reduceDepthSdsmMinMax(pRenderCtx, pCamera, pDepthBuffer, distanceRange);
        }
        else
        {
            distanceRange = mControls.distanceRange;
        }
    }

    void CascadedShadowMaps::setup(RenderContext* pRenderCtx, const Camera* pCamera, const Texture* pDepthBuffer)
    {
        const glm::vec4 clearColor(1);
        mShadowPass.pFbo->clear(clearColor, 1.0f, 0, FboAttachmentType::All);

        // Calc the bounds
        glm::vec2 distanceRange;
        calcDistanceRange(pRenderCtx, pCamera, pDepthBuffer, distanceRange);

        RenderContext::Viewport VP;
        VP.originX = 0;
        VP.originY = 0;
        VP.minDepth = 0;
        VP.maxDepth = 1;
        VP.height = mShadowPass.mapSize.x;
        VP.width = mShadowPass.mapSize.y;
        pRenderCtx->pushViewport(0, VP);

        const auto& pOldRS = pRenderCtx->getRasterizerState();
        if(mControls.depthClamp)
        {
            pRenderCtx->setRasterizerState(mShadowPass.pDepthClampRS);
        }
        pRenderCtx->pushFbo(mShadowPass.pFbo);
        pRenderCtx->setDepthStencilState(nullptr, 0);

        partitionCascades(pCamera, distanceRange);
        renderScene(pRenderCtx);

        if(mCsmData.filterMode == CsmFilterVsm || mCsmData.filterMode == CsmFilterEvsm2 || mCsmData.filterMode == CsmFilterEvsm4)
        {
            mpGaussianBlur->execute(pRenderCtx, mShadowPass.pFbo->getColorTexture(0).get(), mShadowPass.pFbo);
            mShadowPass.pFbo->getColorTexture(0)->generateMips();
        }

        pRenderCtx->popViewport(0);
        pRenderCtx->setRasterizerState(pOldRS);
        pRenderCtx->popFbo();
    }

    void CascadedShadowMaps::setDataIntoUniformBuffer(UniformBuffer* pUbo, const std::string& varName)
    {
        size_t offset = pUbo->getVariableOffset(varName + ".globalMat");
        Sampler* pSampler = nullptr;
        const Texture* pTexture = nullptr;
        uint64_t* pMap = nullptr;

        switch(mCsmData.filterMode)
        {
        case CsmFilterPoint:
            pSampler = mShadowPass.pPointCmpSampler.get();
            pTexture = mShadowPass.pFbo->getDepthStencilTexture().get();
            pMap = &mCsmData.shadowMap;
            break;
        case CsmFilterHwPcf:
        case CsmFilterFixedPcf:
        case CsmFilterStochasticPcf:
            pSampler = mShadowPass.pLinearCmpSampler.get();
            pTexture = mShadowPass.pFbo->getDepthStencilTexture().get();
            pMap = &mCsmData.shadowMap;
            break;
        case CsmFilterVsm:
        case CsmFilterEvsm2:
        case CsmFilterEvsm4:
            pSampler = mShadowPass.pVSMTrilinearSampler.get();
            pTexture = mShadowPass.pFbo->getColorTexture(0).get();
            pMap = &mCsmData.momentsMap;
            break;
        }

        *pMap = pTexture->makeResident(pSampler);
        mCsmData.lightDir = glm::normalize(((DirectionalLight*)mpLight.get())->getWorldDirection());
        pUbo->setBlob(&mCsmData, offset, sizeof(mCsmData));
    }

    Texture::SharedConstPtr CascadedShadowMaps::getShadowMap() const
    {
        switch(mCsmData.filterMode)
        {
        case CsmFilterVsm:
        case CsmFilterEvsm2:
        case CsmFilterEvsm4:
            return mShadowPass.pFbo->getColorTexture(0);
        }
        return mShadowPass.pFbo->getDepthStencilTexture();
    }

    void GUI_CALL CascadedShadowMaps::getSdsmReadbackLatency(void* pData, void* pThis)
    {
        const CascadedShadowMaps* pCsm = (CascadedShadowMaps*)pThis;
        *(uint32_t*)pData = pCsm->mSdsmData.readbackLatency;
    }

    void GUI_CALL CascadedShadowMaps::setSdsmReadbackLatency(const void* pData, void* pThis)
    {
        CascadedShadowMaps* pCsm = (CascadedShadowMaps*)pThis;
        pCsm->mSdsmData.readbackLatency = *(uint32_t*)pData;
        pCsm->createSdsmData(nullptr);
    }

    void GUI_CALL CascadedShadowMaps::getFilterModeCB(void* pData, void* pThis)
    {
        CascadedShadowMaps* pCsm = (CascadedShadowMaps*)pThis;
        *(uint32_t*)pData = pCsm->mCsmData.filterMode;
    }

    void GUI_CALL CascadedShadowMaps::setFilterModeCB(const void* pData, void* pThis)
    {
        CascadedShadowMaps* pCsm = (CascadedShadowMaps*)pThis;
        pCsm->mCsmData.filterMode = *(uint32_t*)pData;
        pCsm->createShadowPassResources(pCsm->mShadowPass.pFbo->getWidth(), pCsm->mShadowPass.pFbo->getHeight());
    }

    void GUI_CALL CascadedShadowMaps::getFilterKernelSizeCB(void* pData, void* pThis)
    {
        CascadedShadowMaps* pCsm = (CascadedShadowMaps*)pThis;
        *(uint32_t*)pData = pCsm->mCsmData.sampleKernelSize;
    }

    void GUI_CALL CascadedShadowMaps::setFilterKernelSizeCB(const void* pData, void* pThis)
    {
        CascadedShadowMaps* pCsm = (CascadedShadowMaps*)pThis;
        int32_t size = *(int32_t*)pData;
        int32_t delta = size - pCsm->mCsmData.sampleKernelSize;
        // Make sure we always step in 2
        int32_t offset = delta & 1;
        delta = delta + (delta < 0 ? -offset : offset);
        pCsm->mCsmData.sampleKernelSize += delta;
        pCsm->mCsmData.sampleKernelSize = min(11, max(1, pCsm->mCsmData.sampleKernelSize));
        pCsm->createGaussianBlurTech();
    }

    void GUI_CALL CascadedShadowMaps::setVsmAnisotropyCB(const void* pData, void* pThis)
    {
        CascadedShadowMaps* pCsm = (CascadedShadowMaps*)pThis;
        pCsm->createVsmSampleState(*(uint32_t*)pData);
    }

    void GUI_CALL CascadedShadowMaps::getVsmAnisotropyCB(void* pData, void* pThis)
    {
        CascadedShadowMaps* pCsm = (CascadedShadowMaps*)pThis;
        *(uint32_t*)pData = pCsm->mShadowPass.pVSMTrilinearSampler->getMaxAnisotropy();
    }
}
