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
        static UniquePtr create(const Scene::SharedPtr& pScene, ConstantBuffer::SharedPtr pAlphaMapCb) { return UniquePtr(new CsmSceneRenderer(pScene, pAlphaMapCb)); }

    protected:
        CsmSceneRenderer(const Scene::SharedPtr& pScene, ConstantBuffer::SharedPtr pAlphaMapCb) : SceneRenderer(pScene), mpAlphaMapCb(pAlphaMapCb) { setObjectCullState(false); }
        ConstantBuffer::SharedPtr mpAlphaMapCb;
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

#ifdef _ALPHA_FROM_ALBEDO_MAP
                const auto& pLayer = mpLastMaterial->getLayer(0);
                a.map = pLayer ? pLayer->albedo.texture.gpuAddress : 0;
                a.val = 0.5f;
#else
                // DISABLED_FOR_D3D12
                //TODO what is this?
                a.val = mpLastMaterial->getAlphaThreshold();
#endif
                mpAlphaMapCb->setBlob(&a, 0, sizeof(a));
                //TODO THIS
                //mpAlphaMapCb->setTexture(mpLastMaterial->getAlphaMap(), ;
            }
            return true;
        };

        void postFlushDraw(RenderContext* pContext, const CurrentWorkingData& currentData) override
        {
            if(mUnloadTexturesOnMaterialChange && mMaterialChanged)
            {
                mpLastMaterial->evictTextures();
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
        float angle = pLight->getOpeningAngle() * 2;
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
        if(mpLight->getType() != LightDirectional)
        {
            if (cascadeCount != 1)
            {
                logWarning("CSM with point-light only supports a single cascade (the user requested " + std::to_string(cascadeCount) + ")");
            }
            cascadeCount = 1;
        }
        mCsmData.cascadeCount = cascadeCount;
        createShadowPassResources(mapWidth, mapHeight);
        mDepthPass.pProg = GraphicsProgram::createFromFile(kDepthPassVSFile, "");
        mDepthPass.pProg->addDefine("_APPLY_PROJECTION");
        mDepthPass.pState = GraphicsState::create();
        mDepthPass.pState->setProgram(mDepthPass.pProg);
        mDepthPass.pGraphicsVars = GraphicsVars::create(mDepthPass.pProg->getActiveVersion()->getReflector());

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
        samplerDesc.setComparisonMode(Sampler::ComparisonMode::Disabled);
        samplerDesc.setAddressingMode(Sampler::AddressMode::Clamp, Sampler::AddressMode::Clamp, Sampler::AddressMode::Clamp);
        samplerDesc.setLodParams(-100.f, 100.f, 0.f);

        createVsmSampleState(1);
        //TODO re-add gaus blur
        //createGaussianBlurTech();
    }

    void CascadedShadowMaps::createGaussianBlurTech()
    {
        mpGaussianBlur = GaussianBlur::create(mCsmData.sampleKernelSize);
    }

    CascadedShadowMaps::UniquePtr CascadedShadowMaps::create(uint32_t mapWidth, uint32_t mapHeight, Light::SharedConstPtr pLight, Scene::SharedPtr pScene, uint32_t cascadeCount, ResourceFormat shadowMapFormat)
    {
        if(isDepthFormat(shadowMapFormat) == false)
        {
            logError(std::string("Can't create CascadedShadowMaps effect. Requested resource format ") + to_string(shadowMapFormat) + " is not a depth format", true);
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
        mCsmData.depthBias = 0.005f;
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
        {
            Fbo::Desc fboDesc;
            fboDesc.setDepthStencilTarget(depthFormat);
            mShadowPass.pFbo = FboHelper::create2D(mapWidth, mapHeight, fboDesc, mCsmData.cascadeCount);
            mDepthPass.pFbo = FboHelper::create2D(mapWidth, mapHeight, fboDesc, mCsmData.cascadeCount);
        }
        }

        if(colorFormat != ResourceFormat::Unknown)
        {
            Fbo::Desc fboDesc;
            fboDesc.setDepthStencilTarget(depthFormat).setColorTarget(0, colorFormat);
            //TODO Was max mip, had to change to 1 to fix...
            //D3D12 ERROR: ID3D12Device::CreateCommittedResource: D3D12_RESOURCE_DESC::SampleDesc::Count must be 1 when number of mip levels is not 1, or Dimension is not D3D12_RESOURCE_DIMENSION_TEXTURE2D. SampleDesc::Count is 4, MipLevels is 12, and Dimension is D3D12_RESOURCE_DIMENSION_TEXTURE2D. [ STATE_CREATION ERROR #723: CREATERESOURCE_INVALIDSAMPLEDESC]
            //SampleCount should be numCascades, putting it as this for now, requires a MS flag if fbo has > 1 sample
            //See renderTargetView::Create
            mShadowPass.pFbo = FboHelper::create2D(mapWidth, mapHeight, fboDesc, mCsmData.cascadeCount);
            mDepthPass.pFbo = FboHelper::create2D(mapWidth, mapHeight, fboDesc, mCsmData.cascadeCount);
        }

        mShadowPass.fboAspectRatio = (float)mapWidth / (float)mapHeight;

        // Create the shadows program
        mShadowPass.pProg = GraphicsProgram::createFromFile(kDepthPassVSFile, kDepthPassFsFile, kDepthPassGsFile, "", "", progDef);
        mShadowPass.pState = GraphicsState::create();
        mShadowPass.pState->setProgram(mShadowPass.pProg);
        mShadowPass.pState->setDepthStencilState(nullptr);
        mShadowPass.pState->setFbo(mShadowPass.pFbo);
        mShadowPass.pGraphicsVars = GraphicsVars::create(mShadowPass.pProg->getActiveVersion()->getReflector());
        mShadowPass.pLightCB = mShadowPass.pGraphicsVars["PerLightCB"];
        mShadowPass.pAlphaCB = mShadowPass.pGraphicsVars["AlphaMapCB"];

        mpSceneRenderer = CsmSceneRenderer::create(mpScene, mShadowPass.pAlphaCB);
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
        //TODO fix this so that settings are only offered if theyre relevant to the current filter mode
        if (pGui->beginGroup(uiGroup.c_str()))
        {
            //Filter mode
            Gui::DropdownList filterModeList;
            filterModeList.push_back({ (uint32_t)CsmFilterPoint, "Point" });
            filterModeList.push_back({ (uint32_t)CsmFilterHwPcf, "2x2 HW PCF" });
            filterModeList.push_back({ (uint32_t)CsmFilterFixedPcf, "Fixed-Size PCF" });
            filterModeList.push_back({ (uint32_t)CsmFilterVsm, "VSM" });
            filterModeList.push_back({ (uint32_t)CsmFilterEvsm2, "EVSM2" });
            filterModeList.push_back({ (uint32_t)CsmFilterEvsm4, "EVSM4" });
            filterModeList.push_back({ (uint32_t)CsmFilterStochasticPcf, "Stochastic Poisson PSF" });
            uint32_t filterIndex = static_cast<uint32_t>(mCsmData.filterMode);
            if (pGui->addDropdown("Filter Mode", filterModeList, filterIndex))
            {
                mCsmData.filterMode = filterIndex;
                createShadowPassResources(mShadowPass.pFbo->getWidth(), mShadowPass.pFbo->getHeight());
            }

            //Kernal size
            i32 newKernalSize = mCsmData.sampleKernelSize;
            if (pGui->addIntVar("Max Kernel Size", newKernalSize))
            {
                onSetKernalSize(newKernalSize);
            }

            //partition mode
            Gui::DropdownList partitionMode;
            partitionMode.push_back({ (uint32_t)PartitionMode::Linear, "Linear" });
            partitionMode.push_back({ (uint32_t)PartitionMode::Logarithmic, "Logarithmic" });
            partitionMode.push_back({ (uint32_t)PartitionMode::PSSM, "PSSM" });
            uint32_t newPartitionMode = static_cast<uint32_t>(mControls.partitionMode);
            if (pGui->addDropdown("Partition Mode", partitionMode, newPartitionMode))
            {
                mControls.partitionMode = static_cast<PartitionMode>(newPartitionMode);
            }

            pGui->addFloatVar("PSSM Lambda", mControls.pssmLambda, 0, 1.0f);

            // SDSM data
            const char* sdsmGroup = "SDSM MinMax";
            if (pGui->beginGroup(sdsmGroup))
            {
                pGui->addCheckBox("Enable", mControls.useMinMaxSdsm);
                if(pGui->addIntVar("Readback Latency", mSdsmData.readbackLatency))
                {
                    createSdsmData(nullptr);
                }
                pGui->endGroup();
            }
            
            // Controls
            const char* manualSettingsGroup = "Manual Settings";
            if (pGui->beginGroup(manualSettingsGroup))
            {
                pGui->addFloatVar("Min Distance", mControls.distanceRange.x, 0, 1);
                pGui->addFloatVar("Max Distance", mControls.distanceRange.y, 0, 1);
                pGui->addFloatVar("Depth Bias", mCsmData.depthBias, 0, FLT_MAX, 0.0001f);
                pGui->addCheckBox("Depth Clamp", mControls.depthClamp);
                pGui->addCheckBox("Stabilize Cascades", mControls.stabilizeCascades);
                pGui->addCheckBox("Concentric Cascades", mControls.concentricCascades);
                pGui->addFloatVar("Cascade Blend Threshold", mCsmData.cascadeBlendThreshold, 0, 1.0f);
                pGui->endGroup();
            }

            //VSM/ESM
            const char* vsmGroup = "VSM/EVSM";
            if (pGui->beginGroup(vsmGroup))
            {
                Gui::DropdownList vsmMaxAniso;
                vsmMaxAniso.push_back({ (uint32_t)1, "1" });
                vsmMaxAniso.push_back({ (uint32_t)2, "2" });
                vsmMaxAniso.push_back({ (uint32_t)4, "4" });
                vsmMaxAniso.push_back({ (uint32_t)8, "8" });
                vsmMaxAniso.push_back({ (uint32_t)16, "16" });

                uint32_t newMaxAniso = mShadowPass.pVSMTrilinearSampler->getMaxAnisotropy();
                pGui->addDropdown("Max Aniso", vsmMaxAniso, newMaxAniso);
                {
                    createVsmSampleState(newMaxAniso);
                }

                pGui->addFloatVar("Light Bleed Reduction", mCsmData.lightBleedingReduction, 0, 1.0f, 0.01f);
                const char* evsmExpGroup = "EVSM Exp";
                if (pGui->beginGroup(evsmExpGroup))
                {
                    pGui->addFloatVar("Positive", mCsmData.evsmExponents.x, 0.0f, 42.0f, 0.01f);
                    pGui->addFloatVar("Negative", mCsmData.evsmExponents.y, 0.0f, 42.0f, 0.01f);
                    pGui->endGroup();
                }

                pGui->endGroup();
            }

            pGui->endGroup();
        }
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
            mCsmData.cascadeStartDepth[0].x = 0;
            mCsmData.cascadeRange[0].x = 1;
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
            mCsmData.cascadeStartDepth[c].x = depthRange * cascadeStart + nearPlane;
            mCsmData.cascadeRange[c].x = (depthRange * cascadeEnd + nearPlane) - mCsmData.cascadeStartDepth[c].x;
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
        mShadowPass.pLightCB->setBlob(&mCsmData, 0, sizeof(mCsmData));
        mShadowPass.pGraphicsVars->setConstantBuffer(0, mShadowPass.pLightCB);
        mShadowPass.pAlphaCB->setVariable("evsmExp", mCsmData.evsmExponents);
        mShadowPass.pGraphicsVars->setConstantBuffer(1, mShadowPass.pAlphaCB);
        pCtx->pushGraphicsVars(mShadowPass.pGraphicsVars);
        pCtx->pushGraphicsState(mShadowPass.pState);
        mpSceneRenderer->renderScene(pCtx, mpLightCamera.get());
        pCtx->popGraphicsState();
        pCtx->popGraphicsVars();
    }

    void CascadedShadowMaps::executeDepthPass(RenderContext* pCtx, const Camera* pCamera)
    {
        // Must have an FBO attached, otherwise don't know the size of the depth map
        uint32_t width = pCtx->getGraphicsState()->getFbo()->getWidth();
        uint32_t height = pCtx->getGraphicsState()->getFbo()->getHeight();

        if((mDepthPass.pFbo == nullptr) || (mDepthPass.pFbo->getWidth() != width) || (mDepthPass.pFbo->getHeight() != height))
        {
            Fbo::Desc desc;
            desc.setDepthStencilTarget(mShadowPass.pFbo->getDepthStencilTexture()->getFormat());
            mDepthPass.pFbo = FboHelper::create2D(width, height, desc);
        }

        pCtx->clearFbo(mDepthPass.pFbo.get(), glm::vec4(), 1, 0, FboAttachmentType::Depth);

        mDepthPass.pState->setFbo(mDepthPass.pFbo);
        mpSceneRenderer->setObjectCullState(true);
        pCtx->pushGraphicsState(mDepthPass.pState);
        pCtx->pushGraphicsVars(mDepthPass.pGraphicsVars);
        mpSceneRenderer->renderScene(pCtx, const_cast<Camera*>(pCamera));
        mpSceneRenderer->setObjectCullState(false);
        pCtx->popGraphicsVars();
        pCtx->popGraphicsState();
    }

    void CascadedShadowMaps::createVsmSampleState(uint32_t maxAnisotropy)
    {
        if(mShadowPass.pVSMTrilinearSampler)
        {
            const auto pTexture = mShadowPass.pFbo->getColorTexture(0);
            if(pTexture)
            {
                pTexture->evict(mShadowPass.pVSMTrilinearSampler.get());
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
        //TODO fix parallel reudction
        //distanceRange = glm::vec2(mSdsmData.minMaxReduction->reduce(pRenderCtx, pDepthBuffer));

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
        pRenderCtx->clearFbo(mDepthPass.pFbo.get(), clearColor, 1, 0, FboAttachmentType::All);
        pRenderCtx->clearFbo(mShadowPass.pFbo.get(), clearColor, 1, 0, FboAttachmentType::Depth);

        // Calc the bounds
        glm::vec2 distanceRange(0, 0);
        //TODO re-add this when parallel reduction is fixed
        calcDistanceRange(pRenderCtx, pCamera, pDepthBuffer, distanceRange);

        GraphicsState::Viewport VP;
        VP.originX = 0;
        VP.originY = 0;
        VP.minDepth = 0;
        VP.maxDepth = 1;
        VP.height = mShadowPass.mapSize.x;
        VP.width = mShadowPass.mapSize.y;

        //Set shadow pass state
        mShadowPass.pState->setViewport(0, VP);
        if (mControls.depthClamp)
        {
            mShadowPass.pState->setRasterizerState(mShadowPass.pDepthClampRS);
        }
        else
        {
            mShadowPass.pState->setRasterizerState(nullptr);
        }

        pRenderCtx->pushGraphicsState(mShadowPass.pState);
        partitionCascades(pCamera, distanceRange);
        renderScene(pRenderCtx);

        if(mCsmData.filterMode == CsmFilterVsm || mCsmData.filterMode == CsmFilterEvsm2 || mCsmData.filterMode == CsmFilterEvsm4)
        {
            //todo
            //ILLEGAL char in shader file(1,1)???s
            //mpGaussianBlur->execute(pRenderCtx, mShadowPass.pFbo->getColorTexture(0).get(), mShadowPass.pFbo);
            mShadowPass.pFbo->getColorTexture(0)->generateMips();
        }

        pRenderCtx->popGraphicsState();
    }

    void CascadedShadowMaps::setDataIntoGraphicsVars(GraphicsVars::SharedPtr pVars, const std::string& varName)
    {
        size_t offset = pVars->getConstantBuffer("PerFrameCB")->getVariableOffset(varName + ".globalMat");
        Sampler::SharedPtr pSampler = nullptr;
        Texture::SharedPtr pTexture = nullptr;

        //TODO, set these textures and samplers by offset rather than by name
        switch (mCsmData.filterMode)
        {
        case CsmFilterPoint:
            pSampler = mShadowPass.pPointCmpSampler;
            pTexture = mShadowPass.pFbo->getDepthStencilTexture();
            pVars->setTexture("shadowMap", pTexture);
            pVars->setSampler("compareSampler", pSampler);
            break;
        case CsmFilterHwPcf:
        case CsmFilterFixedPcf:
        case CsmFilterStochasticPcf:
            pSampler = mShadowPass.pLinearCmpSampler;
            pTexture = mShadowPass.pFbo->getDepthStencilTexture();
            pVars->setTexture("shadowMap", pTexture);
            pVars->setSampler("compareSampler", pSampler);
            break;
        case CsmFilterVsm:
        case CsmFilterEvsm2:
        case CsmFilterEvsm4:
            pSampler = mShadowPass.pVSMTrilinearSampler;
            pTexture = mShadowPass.pFbo->getColorTexture(0);
            pVars->setTexture("momentsMap", pTexture);
            pVars->setSampler("exampleSampler", pSampler);
            break;
        }

        //Todo, at least depth bias and light dir aren't getitng sent to shader correctly 
        mCsmData.lightDir = glm::normalize(((DirectionalLight*)mpLight.get())->getWorldDirection());
        //I'm not sure what the deal here is, but the offsets in shader don't line up with the offsets of the struct 
        //depthBias is at offset 1012 from csm data starting at 448, so offset 564 into struct, but the Cpp size of the 
        //struct is 468. Which is why the set blob call here isn't affecting the depth bias. Also since it's a memcpy 
        //it's probably setting data incorrectly. 
        ConstantBuffer::SharedPtr pCB = pVars->getConstantBuffer("PerFrameCB");
        //pCB->setBlob(&mCsmData, offset, sizeof(mCsmData));
        //Manually setting the depth bias because of above issue. Everything at and beyond depthBias is not getting sent properly
        //This is all temporary till i can figure out the problems with blob
        pCB->setVariable(varName + ".globalMat", mCsmData.globalMat);
        pCB->setVariableArray(varName + ".cascadeScale", mCsmData.cascadeScale, CSM_MAX_CASCADES);
        pCB->setVariableArray(varName + ".cascadeOffset", mCsmData.cascadeOffset, CSM_MAX_CASCADES);
        pCB->setVariableArray(varName + ".cascadeStartDepth", mCsmData.cascadeStartDepth, CSM_MAX_CASCADES);
        pCB->setVariableArray(varName + ".cascadeRange", mCsmData.cascadeRange, CSM_MAX_CASCADES);
        pCB->setVariable(varName + ".depthBias", mCsmData.depthBias);
        pCB->setVariable(varName + ".cascadeCount", mCsmData.cascadeCount);
        pCB->setVariable(varName + ".filterMode", mCsmData.filterMode);
        pCB->setVariable(varName + ".sampleKernelSize", mCsmData.sampleKernelSize);
        pCB->setVariable(varName + ".lightDir", mCsmData.lightDir);
        pCB->setVariable(varName + ".lightBleedingReduction", mCsmData.lightBleedingReduction);
        pCB->setVariable(varName + ".cascadeBlendThreshold", mCsmData.cascadeBlendThreshold);
        pCB->setVariable(varName + ".evsmExponents", mCsmData.evsmExponents);
    }
    
    Texture::SharedPtr CascadedShadowMaps::getShadowMap() const
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

    void CascadedShadowMaps::onSetFilterMode(uint32_t newFilterMode)
    {
        mCsmData.filterMode = newFilterMode;
        createShadowPassResources(mShadowPass.pFbo->getWidth(), mShadowPass.pFbo->getHeight());
    }

    void CascadedShadowMaps::onSetVsmAnisotropy(uint32_t maxAniso)
    {
        createVsmSampleState(maxAniso);
    }

    void CascadedShadowMaps::onSetKernalSize(u32 newSize)
    {
        int32_t delta = newSize - mCsmData.sampleKernelSize;
        // Make sure we always step in 2
        int32_t offset = delta & 1;
        delta = delta + (delta < 0 ? -offset : offset);
        mCsmData.sampleKernelSize += delta;
        mCsmData.sampleKernelSize = min(11, max(1, mCsmData.sampleKernelSize));
        createGaussianBlurTech();
    }

}
