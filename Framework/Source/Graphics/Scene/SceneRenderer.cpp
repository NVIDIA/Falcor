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
    

    static const char* kPerMaterialCbName = "InternalPerMaterialCB";
    static const char* kPerFrameCbName = "InternalPerFrameCB";
    static const char* kPerStaticMeshCbName = "InternalPerStaticMeshCB";
    static const char* kPerSkinnedMeshCbName = "InternalPerSkinnedMeshCB";

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
            const auto pPerMeshCbData = pReflector->getBufferDesc(kPerStaticMeshCbName, ProgramReflection::BufferReflection::Type::Constant);
            const auto pPerFrameCbData = pReflector->getBufferDesc(kPerFrameCbName, ProgramReflection::BufferReflection::Type::Constant);
            assert(pPerMeshCbData);
            assert(pPerFrameCbData);

            sWorldMatOffset = pPerMeshCbData->getVariableData("gWorldMat[0]")->location;
            sMeshIdOffset = pPerMeshCbData->getVariableData("gMeshId")->location;
            sCameraDataOffset = pPerFrameCbData->getVariableData("gCam.viewMat")->location;
        }
    }

    void SceneRenderer::setPerFrameData(RenderContext* pContext, const CurrentWorkingData& currentData)
    {
        // Set VPMat
        if (currentData.pCamera)
        {
            ConstantBuffer* pCB = pContext->getProgramVars()->getConstantBuffer(kPerFrameCbName).get();
            currentData.pCamera->setIntoConstantBuffer(pCB, sCameraDataOffset);
        }
    }

    bool SceneRenderer::setPerModelData(RenderContext* pContext,const CurrentWorkingData& currentData)
    {
        // Set bones
        if(currentData.pModel->hasBones())
        {
            ConstantBuffer* pCB = pContext->getProgramVars()->getConstantBuffer(kPerSkinnedMeshCbName).get();
            if (sBonesOffset == ConstantBuffer::kInvalidOffset)
            {
                sBonesOffset = pCB->getVariableOffset("gBones");
            }

            pCB->setVariableArray(sBonesOffset, currentData.pModel->getBonesMatrices(), currentData.pModel->getBonesCount());
        }
		return true;
    }

    bool SceneRenderer::setPerMeshData(RenderContext* pContext, const CurrentWorkingData& currentData)
    {
		return true;
    }

    bool SceneRenderer::setPerMeshInstanceData(RenderContext* pContext, const glm::mat4& translation, uint32_t meshInstanceID, uint32_t drawInstanceID, const CurrentWorkingData& currentData)
    {
        glm::mat4 worldMat = translation;
        if(currentData.pMesh->hasBones() == false)
        {
            worldMat = worldMat * currentData.pMesh->getInstanceMatrix(meshInstanceID);
        }
        ConstantBuffer* pCB = pContext->getProgramVars()->getConstantBuffer(kPerStaticMeshCbName).get();
        pCB->setBlob(&worldMat, sWorldMatOffset + drawInstanceID*sizeof(glm::mat4), sizeof(glm::mat4));

        // Set mesh id
        pCB->setVariable(sMeshIdOffset, currentData.pMesh->getId());

		return true;
    }

    bool SceneRenderer::setPerMaterialData(RenderContext* pContext, const CurrentWorkingData& currentData)
    {
        currentData.pMaterial->setIntoProgramVars(pContext->getProgramVars().get(), kPerMaterialCbName, "gMaterial");
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
            mpLastMaterial = pMesh->getMaterial().get();
            setPerMaterialData(pContext, currentData);

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

    void SceneRenderer::renderMesh(RenderContext* pContext, const Mesh* pMesh, const glm::mat4& translation, Camera* pCamera, CurrentWorkingData& currentData)
    {
		currentData.pMesh = pMesh;

		if (setPerMeshData(pContext, currentData))
		{
			// Bind VAO and set topology
			pContext->getPipelineStateCache()->setVao(pMesh->getVao());
            pContext->getPipelineStateCache()->setPrimitiveType(PipelineStateObject::PrimitiveType::Triangle);
			pContext->setTopology(pMesh->getTopology());

			uint32_t InstanceCount = pMesh->getInstanceCount();

			uint32_t activeInstances = 0;
			//auto pCamera = mpScene->getActiveCamera();

			for (uint32_t instanceID = 0; instanceID < InstanceCount; instanceID++)
			{
				BoundingBox box = pMesh->getInstanceBoundingBox(instanceID).transform(translation);

				if ((mCullEnabled == false) || (pCamera->isObjectCulled(box) == false))
				{
					if (setPerMeshInstanceData(pContext, translation, instanceID, activeInstances, currentData))
					{
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
			if(activeInstances != 0)
			{
				flushDraw(pContext, currentData.pMesh, activeInstances, currentData);
			}
		}
    }

    void SceneRenderer::renderModel(RenderContext* pContext, const Model* pModel, const glm::mat4& instanceMatrix, Camera* pCamera, CurrentWorkingData& currentData)
    {        
		currentData.pModel = pModel;
		if (setPerModelData(pContext, currentData))
		{
            Program* pProgram = currentData.pPsoCache->getProgram().get();
			// Bind the program
			if(pModel->hasBones())
			{
				pProgram->addDefine("_VERTEX_BLENDING");
			}


			mpLastMaterial = nullptr;

			// Loop over the meshes
			for (uint32_t meshID = 0; meshID < pModel->getMeshCount(); meshID++)
			{
				renderMesh(pContext, pModel->getMesh(meshID).get(), instanceMatrix, pCamera, currentData);
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
        return mpScene->updateCamera(currentTime, mpCameraController.get());

        for(uint32_t modelID = 0; modelID < mpScene->getModelCount(); modelID++)
        {
            mpScene->getModel(modelID)->animate(currentTime);
        }
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
        updateVariableOffsets(pContext->getProgramVars()->getReflection().get());
		CurrentWorkingData currentData;
		currentData.pPsoCache = pContext->getPipelineStateCache().get();
		currentData.pCamera = pCamera;
		currentData.pMaterial = nullptr;
		currentData.pMesh = nullptr;
		currentData.pModel = nullptr;
        setupVR();
        setPerFrameData(pContext, currentData);

        for (uint32_t modelID = 0; modelID < mpScene->getModelCount(); modelID++)
        {
            for (uint32_t InstanceID = 0; InstanceID < mpScene->getModelInstanceCount(modelID); InstanceID++)
            {
                auto& Instance = mpScene->getModelInstance(modelID, InstanceID);
                if (Instance.isVisible)
                {
                    renderModel(pContext, mpScene->getModel(modelID).get(), Instance.transformMatrix, pCamera, currentData);
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
                if(checkExtensionSupport("GL_NV_stereo_view_rendering") == false)
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