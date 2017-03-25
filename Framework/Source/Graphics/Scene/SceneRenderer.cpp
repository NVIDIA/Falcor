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
            const auto pPerMeshCbData = pReflector->getBufferDesc(kPerMeshCbName, ProgramReflection::BufferReflection::Type::Constant);

            if (pPerMeshCbData != nullptr)
            {
// SPIRE:
//                sWorldMatOffset = pPerMeshCbData->getVariableData("gWorldMat[0]")->location;
//                sMeshIdOffset = pPerMeshCbData->getVariableData("gMeshId")->location;
//                sDrawIDOffset = pPerMeshCbData->getVariableData("gDrawId[0]")->location;
            }
        }

        if (sCameraDataOffset == ConstantBuffer::kInvalidOffset)
        {
            const auto pPerFrameCbData = pReflector->getBufferDesc(kPerFrameCbName, ProgramReflection::BufferReflection::Type::Constant);

            if (pPerFrameCbData != nullptr)
            {
// SPIRE:
//                sCameraDataOffset = pPerFrameCbData->getVariableData("gCam.viewMat")->location;
                sCameraDataOffset = pPerFrameCbData->getVariableData("viewMat")->location;
            }
        }
    }

    void SceneRenderer::setPerFrameData(RenderContext* pContext, const CurrentWorkingData& currentData)
    {
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
            auto spireContext = ShaderRepository::Instance().GetContext();

            SpireModule* cameraComponentClass = currentData.pCamera->getSpireComponentClass(spireContext);

            // TODO: need to find the index of the correct parameter, if any...
//            currentData.pGsoCache->getProgram()->setComponent(2, cameraComponentClass);

            ComponentInstance::SharedPtr cameraComponent = currentData.pCamera->getSpireComponentInstance(spireContext);

            // Need to set this at the right place...
            pContext->getGraphicsVars()->setComponent(2, cameraComponent);

#endif
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

         auto spireContext = ShaderRepository::Instance().GetContext();

        SpireModule* componentClass = spFindModule(spireContext, "InternalPerMeshCB_T");

        // TODO: cache and re-use reflection data...
        ProgramReflection::BufferTypeReflection::SharedPtr componentClassReflection =
            ProgramReflection::BufferTypeReflection::create(componentClass);

        // We create a transient component instance here.
        // TODO: find a way to reclaim this space more cleanly.
        ComponentInstance::SharedPtr componentInstance = ComponentInstance::create(componentClassReflection);

        componentInstance->setVariable("gWorldMat", worldMat);
        componentInstance->setVariable("gMeshId", pMesh->getId());

        // Need to set this at the right place...
        int componentIndex = 3;
//        currentData.pGsoCache->getProgram()->setComponent(componentIndex, componentClass);
        pContext->getGraphicsVars()->setComponent(componentIndex, componentInstance);
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

        auto material = currentData.pMaterial;
        
        SpireModule* componentClass = material->getSpireComponentClass();
        ComponentInstance::SharedPtr componentInstance = material->getSpireComponentInstance();

        int componentIndex = 4;
//        currentData.pGsoCache->getProgram()->setComponent(componentIndex, componentClass);
        pContext->getGraphicsVars()->setComponent(componentIndex, componentInstance);
#endif

        return true;
    }

    void SceneRenderer::flushDraw(RenderContext* pContext, const Mesh* pMesh, uint32_t instanceCount, CurrentWorkingData& currentData)
    {
        currentData.pMaterial = pMesh->getMaterial().get();
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
			// Bind spire vertex module
			//TODO: need to set this at right place
			int componentIndex = 1;
			pContext->getGraphicsVars()->setComponent(componentIndex, pMesh->getVertexComponent());


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
        currentData.pMaterial = nullptr;
        currentData.pModel = nullptr;
        currentData.drawID = 0;

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
