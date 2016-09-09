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
#include "Core/UniformBuffer.h"
#include "core/RenderContext.h"
#include "Scene.h"
#include "Utils/OS.h"
#include "VR/OpenVR/VRSystem.h"
#include "Core/Window.h"
#include "glm/matrix.hpp"
#include "Graphics/Material/MaterialSystem.h"

namespace Falcor
{
    UniformBuffer::SharedPtr SceneRenderer::sPerMaterialCB;
    UniformBuffer::SharedPtr SceneRenderer::sPerFrameCB;
    UniformBuffer::SharedPtr SceneRenderer::sPerStaticMeshCB;
    UniformBuffer::SharedPtr SceneRenderer::sPerSkinnedMeshCB;
    size_t SceneRenderer::sBonesOffset = 0;
    size_t SceneRenderer::sCameraDataOffset = 0;
    size_t SceneRenderer::sWorldMatOffset = 0;
    size_t SceneRenderer::sMeshIdOffset = 0;
    

    static const std::string kPerMaterialCbName = "InternalPerMaterialCB";
    static const std::string kPerFrameCbName = "InternalPerFrameCB";
    static const std::string kPerStaticMeshCbName = "InternalPerStaticMeshCB";
    static const std::string kPerSkinnedMeshCbName = "InternalPerSkinnedMeshCB";

    SceneRenderer::UniquePtr SceneRenderer::create(const Scene::SharedPtr& pScene)
    {
        return UniquePtr(new SceneRenderer(pScene));
    }

    SceneRenderer::SceneRenderer(const Scene::SharedPtr& pScene) : mpScene(pScene)
    {
        setCameraControllerType(CameraControllerType::SixDof);
    }

    void SceneRenderer::createUniformBuffers(Program* pProgram)
    {
        // create uniform buffers if required
        if(sPerMaterialCB == nullptr)
        {
            auto pReflector = pProgram->getActiveVersion()->getReflector();
            sPerMaterialCB = UniformBuffer::create(pReflector->getUniformBufferDesc(kPerMaterialCbName));
            sPerFrameCB = UniformBuffer::create(pReflector->getUniformBufferDesc(kPerFrameCbName));
            sPerStaticMeshCB = UniformBuffer::create(pReflector->getUniformBufferDesc(kPerStaticMeshCbName));
            sPerSkinnedMeshCB = UniformBuffer::create(pReflector->getUniformBufferDesc(kPerSkinnedMeshCbName));

            sBonesOffset = sPerSkinnedMeshCB->getVariableOffset("gBones");
            sWorldMatOffset = sPerStaticMeshCB->getVariableOffset("gWorldMat");
            sMeshIdOffset = sPerStaticMeshCB->getVariableOffset("gMeshId");
            sCameraDataOffset = sPerFrameCB->getVariableOffset("gCam.viewMat");
        }
    }

    void SceneRenderer::bindUniformBuffers(RenderContext* pRenderContext, Program* pProgram)
    {
        createUniformBuffers(pProgram);

        const ProgramReflection* pReflection = pProgram->getActiveVersion()->getReflector().get();
        // Per skinned mesh
        uint32_t bufferLoc = pReflection->getUniformBufferBinding(kPerSkinnedMeshCbName);
        pRenderContext->setUniformBuffer(bufferLoc, sPerSkinnedMeshCB);

        // Per static mesh
        bufferLoc = pReflection->getUniformBufferBinding(kPerStaticMeshCbName);
        pRenderContext->setUniformBuffer(bufferLoc, sPerStaticMeshCB);

        // Per material
        bufferLoc = pReflection->getUniformBufferBinding(kPerMaterialCbName);
        pRenderContext->setUniformBuffer(bufferLoc, sPerMaterialCB);

        // Per frame
        bufferLoc = pReflection->getUniformBufferBinding(kPerFrameCbName);
        pRenderContext->setUniformBuffer(bufferLoc, sPerFrameCB);
    }

    void SceneRenderer::setPerFrameData(RenderContext* pContext, const CurrentWorkingData& currentData)
    {
        // Set VPMat
        //auto pCamera = mpScene->getActiveCamera();
        if (currentData.pCamera)
        {
            currentData.pCamera->setIntoUniformBuffer(sPerFrameCB.get(), sCameraDataOffset);
        }
    }

    bool SceneRenderer::setPerModelData(RenderContext* pContext,const CurrentWorkingData& currentData)
    {
        // Set bones
        if(currentData.pModel->hasBones())
        {
            sPerSkinnedMeshCB->setVariableArray(sBonesOffset, currentData.pModel->getBonesMatrices(), currentData.pModel->getBonesCount());
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
        sPerStaticMeshCB->setBlob(&worldMat, sWorldMatOffset + drawInstanceID*sizeof(glm::mat4), sizeof(glm::mat4));

        // Set mesh id
        sPerStaticMeshCB->setVariable(sMeshIdOffset, currentData.pMesh->getId());

		return true;
    }

    bool SceneRenderer::setPerMaterialData(RenderContext* pContext, const CurrentWorkingData& currentData)
    {
        currentData.pMaterial->setIntoUniformBuffer(sPerMaterialCB.get(), "gMaterial");
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
                mpLastMaterial->unloadTextures();
            }
            mpLastMaterial = pMesh->getMaterial().get();
            setPerMaterialData(pContext, currentData);

            if(mCompileMaterialWithProgram)
            {
                ProgramVersion::SharedConstPtr pPatchedProgram = MaterialSystem::patchActiveProgramVersion(currentData.pProgram, mpLastMaterial);
                pContext->setProgram(pPatchedProgram);
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
			pContext->setVao(pMesh->getVao());
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

							pContext->setProgram(currentData.pProgram->getActiveVersion());
							flushDraw(pContext, pMesh, activeInstances, currentData);
							activeInstances = 0;
						}
					}
				}
			}
			if(activeInstances != 0)
			{
				pContext->setProgram(currentData.pProgram->getActiveVersion());
				flushDraw(pContext, currentData.pMesh, activeInstances, currentData);
			}
		}
    }

    void SceneRenderer::renderModel(RenderContext* pContext, Program* pProgram, const Model* pModel, const glm::mat4& instanceMatrix, Camera* pCamera, CurrentWorkingData& currentData)
    {        
		currentData.pModel = pModel;
		if (setPerModelData(pContext, currentData))
		{
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

    void SceneRenderer::renderScene(RenderContext* pContext, Program* pProgram)
    {
        renderScene(pContext, pProgram, mpScene->getActiveCamera().get());
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

    void SceneRenderer::renderScene(RenderContext* pContext, Program* pProgram, Camera* pCamera)
    {
        bindUniformBuffers(pContext, pProgram);
		CurrentWorkingData currentData;
		currentData.pProgram = pProgram;
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
                    renderModel(pContext, pProgram, mpScene->getModel(modelID).get(), Instance.transformMatrix, pCamera, currentData);
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