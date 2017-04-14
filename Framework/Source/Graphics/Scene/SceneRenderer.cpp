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
#include "SceneRenderer.h"
#include "Graphics/Program.h"
#include "Utils/Gui.h"
#include "API/ConstantBuffer.h"
#include "API/RenderContext.h"
#include "Scene.h"
#include "Utils/OS.h"
#include "VR/OpenVR/VRSystem.h"
#include "API/Device.h"
#include "glm/matrix.hpp"
#include "Graphics/Material/MaterialSystem.h"
#include "Graphics/DynamicLightEnvironment.h"

namespace Falcor
{
    size_t SceneRenderer::sBonesOffset = ConstantBuffer::kInvalidOffset;
    size_t SceneRenderer::sCameraDataOffset = ConstantBuffer::kInvalidOffset;
    size_t SceneRenderer::sWorldMatOffset = ConstantBuffer::kInvalidOffset;
    size_t SceneRenderer::sMeshIdOffset = ConstantBuffer::kInvalidOffset;
    size_t SceneRenderer::sDrawIDOffset = ConstantBuffer::kInvalidOffset;

    const char* SceneRenderer::kPerMaterialCbName = "InternalPerMaterialCB";
    const char* SceneRenderer::kPerFrameCbName = "InternalPerFrameCB";
    const char* SceneRenderer::kPerMeshCbName = "InternalPerMeshCB";

    SceneRenderer::UniquePtr SceneRenderer::create(const Scene::SharedPtr& pScene)
    {
        return UniquePtr(new SceneRenderer(pScene));
    }

    SceneRenderer::SceneRenderer(const Scene::SharedPtr& pScene) : mpScene(pScene)
    {
        setCameraControllerType(CameraControllerType::SixDof);
    }

    void SceneRenderer::updateVariableOffsets(const ProgramReflection* pReflector)
    {
        if(sWorldMatOffset == ConstantBuffer::kInvalidOffset)
        {
            auto pPerMeshCbData = ShaderRepository::Instance().findComponentClass("InternalPerMeshCB_T");

            if (pPerMeshCbData != nullptr)
            {
                sWorldMatOffset = pPerMeshCbData->getVariableData("gWorldMat", true)->location;
                sMeshIdOffset = pPerMeshCbData->getVariableData("gMeshId")->location;
                sDrawIDOffset = pPerMeshCbData->getVariableData("gDrawId", true)->location;
            }
        }

        if (sCameraDataOffset == ConstantBuffer::kInvalidOffset)
        {
//            const auto pPerFrameCbData = pReflector->getBufferDesc(kPerFrameCbName, ProgramReflection::BufferReflection::Type::Constant);
            auto pPerFrameCbData = pReflector->getComponent(kPerFrameCbName);

            if (pPerFrameCbData != nullptr)
            {
// SPIRE:
//                sCameraDataOffset = pPerFrameCbData->getVariableData("gCam.viewMat")->location;
                sCameraDataOffset = pPerFrameCbData->getVariableData("viewMat")->location;
            }
        }

        if(mpSavedProgramReflection != pReflector)
        {
            mpSavedProgramReflection = pReflector;

            // Extract component offsets for the components we will set
            //
            // TODO: These should perform lookup based on teh declared class/interface of the
            // parameter, and not on the name, but that is a tweak we can make later...

            mCameraComponentBinding = pReflector->getComponentBinding("InternalPerFrameCB");
            mMeshComponentBinding = pReflector->getComponentBinding("InternalPerMeshCB");
			mLightEnvBinding = pReflector->getComponentBinding("L");
            mMaterialComponentBinding = pReflector->getComponentBinding("M");
            mVertexAttributeComponentBinding = pReflector->getComponentBinding("VertexAttribs");
        }
    }

    void SceneRenderer::setPerFrameData(RenderContext* pContext, const CurrentWorkingData& currentData)
    {
		mpScene->updateLights();
        // Set VPMat
        if (currentData.pCamera)
        {
#if 0
            ConstantBuffer* pCB = pContext->getGraphicsVars()->getConstantBuffer(kPerFrameCbName).get();
            if (pCB)
            {
                currentData.pCamera->setIntoConstantBuffer(pCB, sCameraDataOffset);
            }
#else
            if(mCameraComponentBinding != ConstantBuffer::kInvalidOffset)
            {
                ComponentInstance::SharedPtr cameraComponent = currentData.pCamera->getSpireComponentInstance();

                // Need to set this at the right place...
                pContext->getGraphicsVars()->setComponent(mCameraComponentBinding, cameraComponent);
            }
#endif
        }
		if (mLightEnvBinding != ConstantBuffer::kInvalidOffset)
		{
			pContext->getGraphicsVars()->setComponent(mLightEnvBinding, currentData.pLightEnv->getSpireComponentInstance());
		}
    }

    bool SceneRenderer::setPerModelData(RenderContext* pContext,const CurrentWorkingData& currentData)
    {
        // Set bones
#if 0
        if(currentData.pModel->hasBones())
        {
            ConstantBuffer* pCB = pContext->getGraphicsVars()->getConstantBuffer(kPerMeshCbName).get();
            if(pCB)
            {
                if (sBonesOffset == ConstantBuffer::kInvalidOffset)
                {
                    sBonesOffset = pCB->getVariableOffset("gWorldMat[0]");
                }

                pCB->setVariableArray(sBonesOffset, currentData.pModel->getBonesMatrices(), currentData.pModel->getBonesCount());
            }
        }
#else
        // The spire way...
#endif
        return true;
    }

    bool SceneRenderer::setPerModelInstanceData(RenderContext* pContext, const Scene::ModelInstance::SharedPtr& pModelInstance, uint32_t instanceID, const CurrentWorkingData& currentData)
    {
        return true;
    }

    bool SceneRenderer::setPerMeshData(RenderContext* pContext, const CurrentWorkingData& currentData)
    {
        return true;
    }

    bool SceneRenderer::setPerMeshInstanceData(RenderContext* pContext, const Scene::ModelInstance::SharedPtr& pModelInstance, const Model::MeshInstance::SharedPtr& pMeshInstance, uint32_t drawInstanceID, const CurrentWorkingData& currentData)
    {
#if 0
        ConstantBuffer* pCB = pContext->getGraphicsVars()->getConstantBuffer(kPerMeshCbName).get();
        if(pCB)
        {
            const Mesh* pMesh = pMeshInstance->getObject().get();

            glm::mat4 worldMat;
            if (pMesh->hasBones() == false)
            {
                worldMat = pModelInstance->getTransformMatrix() * pMeshInstance->getTransformMatrix();
            }

            pCB->setBlob(&worldMat, sWorldMatOffset + drawInstanceID * sizeof(glm::mat4), sizeof(glm::mat4));

            // Set mesh id
            pCB->setVariable(sMeshIdOffset, pMesh->getId());
        }
#else
        // Do it the Spire way...

        const Mesh* pMesh = pMeshInstance->getObject().get();

        glm::mat4 worldMat;
        if (pMesh->hasBones() == false)
        {
            worldMat = pModelInstance->getTransformMatrix() * pMeshInstance->getTransformMatrix();
        }

        if(mMeshComponentBinding != ConstantBuffer::kInvalidOffset)
        {
#if 0

            // We are re-using a single component instance for "transient" data, which means that
            // we will end up churning through descriptor sets a bit here...
            if(!mpPerMeshComponentInstance)
            {
                auto componentClass = ShaderRepository::Instance().findComponentClass("InternalPerMeshCB_T");
                mpPerMeshComponentInstance = ComponentInstance::create(componentClass);
            }

            mpPerMeshComponentInstance->setVariable(
                sWorldMatOffset + drawInstanceID * sizeof(glm::mat4),
                worldMat);

            mpPerMeshComponentInstance->setVariable(sMeshIdOffset, pMesh->getId());

            // Need to set this at the right place...
            pContext->getGraphicsVars()->setComponent(mMeshComponentBinding, mpPerMeshComponentInstance);
#else
            auto pPerMeshComponentInstance = pMesh->getTransformComponent(worldMat, drawInstanceID);
            pContext->getGraphicsVars()->setComponent(mMeshComponentBinding, pPerMeshComponentInstance);
#endif
        }
#endif

        return true;
    }

    bool SceneRenderer::setPerMaterialData(RenderContext* pContext, const CurrentWorkingData& currentData)
    {
        ProgramVars* pGraphicsVars = pContext->getGraphicsVars().get();

#if 0
        ConstantBuffer* pCB = pGraphicsVars->getConstantBuffer(kPerMaterialCbName).get();
        if (pCB)
        {
            currentData.pMaterial->setIntoProgramVars(pGraphicsVars, pCB, "gMaterial");
        }
#else
        // Do it the Spire way
        if(mMaterialComponentBinding != ConstantBuffer::kInvalidOffset)
        {
            auto material = currentData.pMaterial;        
            ComponentInstance::SharedPtr componentInstance = material->getSpireComponentInstance();

            pContext->getGraphicsVars()->setComponent(mMaterialComponentBinding, componentInstance);
        }
#endif

        return true;
    }

    void SceneRenderer::flushDraw(RenderContext* pContext, const Mesh* pMesh, uint32_t instanceCount, CurrentWorkingData& currentData)
    {
        currentData.pMaterial = pMesh->getMaterial().get();

        // SPIRE: is it okay to require this?
        assert(currentData.pMaterial);

        // Bind material
        if(mpLastMaterial != pMesh->getMaterial().get())
        {
            if(mUnloadTexturesOnMaterialChange && mpLastMaterial)
            {
                mpLastMaterial->evictTextures();
            }
            setPerMaterialData(pContext, currentData);
            mpLastMaterial = pMesh->getMaterial().get();

            if(mCompileMaterialWithProgram)
            {
                // DISABLED_FOR_D3D12
//                 ProgramVersion::SharedConstPtr pPatchedProgram = MaterialSystem::patchActiveProgramVersion(currentData.pProgram, mpLastMaterial);
//                 pContext->setProgram(pPatchedProgram);
            }
        }

        // Draw
        pContext->drawIndexedInstanced(pMesh->getIndexCount(), instanceCount, 0, 0, 0);
        postFlushDraw(pContext, currentData);
    }

    void SceneRenderer::postFlushDraw(RenderContext* pContext, const CurrentWorkingData& currentData)
    {

    }

    void SceneRenderer::renderMeshInstances(RenderContext* pContext, uint32_t meshID, const Scene::ModelInstance::SharedPtr& pModelInstance, Camera* pCamera, CurrentWorkingData& currentData)
    {
        const Model* pModel = currentData.pModel;
        const Mesh* pMesh = pModel->getMesh(meshID).get();

        if (setPerMeshData(pContext, currentData))
        {
            // Bind VAO and set topology
            pContext->getGraphicsState()->setVao(pMesh->getVao());


            if(mVertexAttributeComponentBinding != ConstantBuffer::kInvalidOffset)
            {
                // Bind spire vertex module
                //TODO: need to set this at right place
                pContext->getGraphicsVars()->setComponent(mVertexAttributeComponentBinding, pMesh->getVertexComponent());
            }


            uint32_t activeInstances = 0;

            const uint32_t instanceCount = pModel->getMeshInstanceCount(meshID);
            for (uint32_t instanceID = 0; instanceID < instanceCount; instanceID++)
            {
                auto& meshInstance = pModel->getMeshInstance(meshID, instanceID);
                BoundingBox box = meshInstance->getBoundingBox().transform(pModelInstance->getTransformMatrix());

                if ((mCullEnabled == false) || (pCamera->isObjectCulled(box) == false))
                {
                    if (meshInstance->isVisible())
                    {
                        if (setPerMeshInstanceData(pContext, pModelInstance, meshInstance, activeInstances, currentData))
                        {
                            currentData.drawID++;
                            activeInstances++;

                            if (activeInstances == mMaxInstanceCount)
                            {
                                // DISABLED_FOR_D3D12
                                //pContext->setProgram(currentData.pProgram->getActiveProgramVersion());
                                flushDraw(pContext, pMesh, activeInstances, currentData);
                                activeInstances = 0;
                            }
                        }
                    }
                }
            }
            if(activeInstances != 0)
            {
                flushDraw(pContext, pMesh, activeInstances, currentData);
            }
        }
    }

    void SceneRenderer::renderModelInstance(RenderContext* pContext, const Scene::ModelInstance::SharedPtr& pModelInstance, Camera* pCamera, CurrentWorkingData& currentData)
    {
        const Model* pModel = pModelInstance->getObject().get();

        if (setPerModelData(pContext, currentData))
        {
            Program* pProgram = currentData.pGsoCache->getProgram().get();
            // Bind the program
            if(pModel->hasBones())
            {
                pProgram->addDefine("_VERTEX_BLENDING");
            }

            mpLastMaterial = nullptr;

            // Loop over the meshes
            for (uint32_t meshID = 0; meshID < pModel->getMeshCount(); meshID++)
            {
                renderMeshInstances(pContext, meshID, pModelInstance, pCamera, currentData);
            }

            // Restore the program state
            if(pModel->hasBones())
            {
                pProgram->removeDefine("_VERTEX_BLENDING");
            }
        }

    }

    bool SceneRenderer::update(double currentTime)
    {
        return mpScene->update(currentTime, mpCameraController.get());
    }

    void SceneRenderer::renderScene(RenderContext* pContext)
    {
        renderScene(pContext, mpScene->getActiveCamera().get());
    }

    void SceneRenderer::setupVR()
    {
        if(mRenderMode == RenderMode::SinglePassStereo || mRenderMode == RenderMode::Stereo)
        {
            VRSystem* pVR = VRSystem::instance();

            if(pVR && pVR->isReady())
            {
                // Get the VR data
                pVR->pollEvents();
                pVR->refreshTracking();
            }
        }
    }

    void SceneRenderer::renderScene(RenderContext* pContext, Camera* pCamera)
    {
        updateVariableOffsets(pContext->getGraphicsVars()->getReflection().get());

        CurrentWorkingData currentData;
        currentData.pGsoCache = pContext->getGraphicsState().get();
        currentData.pCamera = pCamera;
		currentData.pLightEnv = &mpScene->lightEnvironment;
        currentData.pMaterial = nullptr;
        currentData.pModel = nullptr;
        currentData.drawID = 0;

        mpLastMaterial = nullptr;

#if 0
        // Render things a new way, that involves creating a sorted "draw list"

        // Step 1: collect meshes to draw into a draw list
        mDrawList.clear();

        uint32_t modelCount = mpScene->getModelCount();
        for (uint32_t modelID = 0; modelID < modelCount; modelID++)
        {
            uint32_t modelInstanceCount = mpScene->getModelInstanceCount(modelID);
            for (uint32_t modelInstanceID = 0; modelInstanceID < modelInstanceCount; modelInstanceID++)
            {
                auto& pModelInstance = mpScene->getModelInstance(modelID, modelInstanceID);
                if (pModelInstance->isVisible())
                {
                    const Model* pModel = pModelInstance->getObject().get();

                    // Loop over the meshes
                    uint32_t meshCount = pModel->getMeshCount();
                    for (uint32_t meshID = 0; meshID < meshCount; meshID++)
                    {
                        const Mesh* pMesh = pModel->getMesh(meshID).get();

                        const uint32_t instanceCount = pModel->getMeshInstanceCount(meshID);
                        for (uint32_t instanceID = 0; instanceID < instanceCount; instanceID++)
                        {
                            auto& meshInstance = pModel->getMeshInstance(meshID, instanceID);
                            BoundingBox box = meshInstance->getBoundingBox().transform(pModelInstance->getTransformMatrix());

                            if ((mCullEnabled == false) || (pCamera->isObjectCulled(box) == false))
                            {
                                if (meshInstance->isVisible())
                                {
                                    DrawListItem item;

                                    item.modelID = modelID;
                                    item.modelInstanceID = modelInstanceID;
                                    item.meshID = meshID;
                                    item.meshInstanceID = instanceID;

                                    item.pMaterial = pMesh->getMaterial().get();

                                    mDrawList.push_back(item);
                                }
                            }
                        }
                    }
                }
            }
        }

        // Step 2: sort the draw list, based on material
        std::sort(mDrawList.begin(), mDrawList.end(), [=](DrawListItem const& left, DrawListItem const& right)
        {
            return left.pMaterial->getSpireComponentClass()->getSpireComponentClass() < right.pMaterial->getSpireComponentClass()->getSpireComponentClass();
        });

        // Step 3: actually render the meshes we collected


        setupVR();
        setPerFrameData(pContext, currentData);

        mpLastMaterial = nullptr;
        for(auto& item : mDrawList)
        {
            auto modelID = item.modelID;
            auto meshID = item.meshID;
            auto modelInstanceID = item.modelInstanceID;
            auto meshInstanceID = item.meshInstanceID;

            auto pModel = mpScene->getModel(modelID).get();
            if(pModel != currentData.pModel)
            {
                currentData.pModel = pModel;
            }

            auto& pModelInstance = mpScene->getModelInstance(modelID, modelInstanceID);
            if (!setPerModelInstanceData(pContext, pModelInstance, modelInstanceID, currentData))
                continue;

            if (!setPerModelData(pContext, currentData))
                continue;

            Program* pProgram = currentData.pGsoCache->getProgram().get();
            // Bind the program

            // TODO(tfoley): handle `_VERTEX_BLENDING` state...

            const Mesh* pMesh = pModel->getMesh(meshID).get();
            if (!setPerMeshData(pContext, currentData))
                continue;

            // Bind VAO and set topology
            pContext->getGraphicsState()->setVao(pMesh->getVao());

            if(mVertexAttributeComponentBinding != ConstantBuffer::kInvalidOffset)
            {
                // Bind spire vertex module
                //TODO: need to set this at right place
                pContext->getGraphicsVars()->setComponent(mVertexAttributeComponentBinding, pMesh->getVertexComponent());
            }


            uint32_t activeInstances = 0;

            auto& meshInstance = pModel->getMeshInstance(meshID, meshInstanceID);
            if (!setPerMeshInstanceData(pContext, pModelInstance, meshInstance, activeInstances, currentData))
                continue;

            currentData.drawID++;
            activeInstances++;

//                            if (activeInstances == mMaxInstanceCount)
            {
                // DISABLED_FOR_D3D12
                //pContext->setProgram(currentData.pProgram->getActiveProgramVersion());
                flushDraw(pContext, pMesh, activeInstances, currentData);
                activeInstances = 0;
            }
        }

#else
        // The old way: no sorting

        setupVR();
        setPerFrameData(pContext, currentData);

        for (uint32_t modelID = 0; modelID < mpScene->getModelCount(); modelID++)
        {
            currentData.pModel = mpScene->getModel(modelID).get();

            for (uint32_t instanceID = 0; instanceID < mpScene->getModelInstanceCount(modelID); instanceID++)
            {
                auto& pInstance = mpScene->getModelInstance(modelID, instanceID);
                if (pInstance->isVisible())
                {
                    if (setPerModelInstanceData(pContext, pInstance, instanceID, currentData))
                    {
                        renderModelInstance(pContext, pInstance, pCamera, currentData);
                    }
                }
            }
        }
#endif
    }

    void SceneRenderer::setCameraControllerType(CameraControllerType type)
    {
        switch(type)
        {
        case CameraControllerType::FirstPerson:
            mpCameraController = CameraController::SharedPtr(new FirstPersonCameraController);
            break;
        case CameraControllerType::SixDof:
            mpCameraController = CameraController::SharedPtr(new SixDoFCameraController);
            break;
        default:
            should_not_get_here();
        }
        mCamControllerType = type;
    }

    void SceneRenderer::detachCameraController()
    {
        mpCameraController->attachCamera(nullptr);
    }

    bool SceneRenderer::onMouseEvent(const MouseEvent& mouseEvent)
    {
        return mpCameraController->onMouseEvent(mouseEvent);
    }

    bool SceneRenderer::onKeyEvent(const KeyboardEvent& keyEvent)
    {
        return mpCameraController->onKeyEvent(keyEvent);
    }

    void SceneRenderer::setRenderMode(RenderMode mode)
    {
        if(mode == RenderMode::SinglePassStereo || mode == RenderMode::Stereo)
        {
            // Stereo should have been initialized before
            if(VRSystem::instance() == nullptr)
            {
                msgBox("Can't set SceneRenderer render mode to stereo. VRSystem() wasn't initialized or stereo is not available");
                return;
            }
            if(mode == RenderMode::SinglePassStereo)
            {
                // Make sure single pass stereo is supported
                if(Device::isExtensionSupported("GL_NV_stereo_view_rendering") == false)
                {
                    msgBox("Can't set SceneRenderer render mode to single pass stereo. Extension is not supported on this machine");
                    return;
                }
            }
            mpCameraController = CameraController::SharedPtr(new HmdCameraController);
        }
        else
        {
            setCameraControllerType(mCamControllerType);
        }

        mRenderMode = mode;
    }
}
