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
#include <vector>
#include "Utils/Gui.h"
#include "Graphics/Camera/CameraController.h"
#include "Graphics/Scene/Scene.h"
#include "utils/CpuTimer.h"
#include "API/ConstantBuffer.h"
#include "Utils/DebugDrawer.h"

namespace Falcor
{
    class Model;
    class GraphicsState;
    class RenderContext;
    class Material;
    class Mesh;
    class Camera;

    class SceneRenderer
    {
    public:
        using UniquePtr = std::unique_ptr<SceneRenderer>;
        using UniqueConstPtr = std::unique_ptr<const SceneRenderer>;

        enum class RenderMode
        {
            Mono,
            Stereo,
            SinglePassStereo,
        };

        static UniquePtr create(const Scene::SharedPtr& pScene);

        /** Renders the full scene, does update of the camera internally
            Call update() before using this function, otherwise camera will not move and models will not be animated
        */
        void renderScene(RenderContext* pContext);

        /** Renders the full scene, overriding the internal camera
            Call update() before using this function otherwise model animation will not work
        */
        void renderScene(RenderContext* pContext, Camera* pCamera);

        /** Update the camera and model animation.
            Should be called before renderScene(), unless not animations are used and you update the camera manualy
        */
        bool update(double currentTime);

        bool onKeyEvent(const KeyboardEvent& keyEvent);
        bool onMouseEvent(const MouseEvent& mouseEvent);

        /** Enable/disable mesh culling. Culling does not always result in performance gain, especially when there are a lot of meshes to process with low rejection rate.
        */
        void setObjectCullState(bool enable) { mCullEnabled = enable; }

        /** Set the maximal number of mesh instance to dispatch in a single draw call.
        */
        void setMaxInstanceCount(uint32_t instanceCount) { mMaxInstanceCount = instanceCount; }

        /** This setting controls whether to unload textures from GPU memory before binding a new material.\n
        Useful for rendering very large models with many textures that can't fit into GPU memory at once. Setting this to true usually results in performance loss.
        */
        void setUnloadTexturesOnMaterialChange(bool unload) { mUnloadTexturesOnMaterialChange = unload; }

        enum class CameraControllerType
        {
            FirstPerson,
            SixDof
        };

        void setCameraControllerType(CameraControllerType type);

        void detachCameraController();

        const Scene* getScene() const { return mpScene.get(); }

        void setRenderMode(RenderMode mode);
        void toggleStaticMaterialCompilation(bool on) { mCompileMaterialWithProgram = on; }

    protected:

        struct CurrentWorkingData
        {
            const Camera* pCamera;
            mutable GraphicsState* pGsoCache;
            const Model* pModel;
            const Material* pMaterial;
			const DynamicLightEnvironment * pLightEnv;
            uint32_t drawID; // Zero-based mesh instance draw order/ID. Resets at the beginning of renderScene, and increments per mesh instance drawn.
        };

        SceneRenderer(const Scene::SharedPtr& pScene);
        Scene::SharedPtr mpScene;

        static const char* kPerMaterialCbName;
        static const char* kPerFrameCbName;
        static const char* kPerMeshCbName;

        static size_t sBonesOffset;
        static size_t sCameraDataOffset;
        static size_t sWorldMatOffset;
        static size_t sMeshIdOffset;
        static size_t sDrawIDOffset;
//        ComponentInstance::SharedPtr mpPerMeshComponentInstance;

        void updateVariableOffsets(const ProgramReflection* pReflector);

        virtual void setPerFrameData(RenderContext* pContext, const CurrentWorkingData& currentData);
        virtual bool setPerModelData(RenderContext* pContext, const CurrentWorkingData& currentData);
        virtual bool setPerModelInstanceData(RenderContext* pContext, const Scene::ModelInstance::SharedPtr& pModelInstance, uint32_t instanceID, const CurrentWorkingData& currentData);
        virtual bool setPerMeshData(RenderContext* pContext,  const CurrentWorkingData& currentData);
        virtual bool setPerMeshInstanceData(RenderContext* pContext, const Scene::ModelInstance::SharedPtr& pModelInstance, const Model::MeshInstance::SharedPtr& pMeshInstance, uint32_t drawInstanceID, const CurrentWorkingData& currentData);
        virtual bool setPerMaterialData(RenderContext* pContext, const CurrentWorkingData& currentData);
        virtual void postFlushDraw(RenderContext* pContext, const CurrentWorkingData& currentData);

        void renderModelInstance(RenderContext* pContext, const Scene::ModelInstance::SharedPtr& pModelInstance, Camera* pCamera, CurrentWorkingData& currentData);
        void renderMeshInstances(RenderContext* pContext, uint32_t modelID, const Scene::ModelInstance::SharedPtr& pModelInstance, Camera* pCamera, CurrentWorkingData& currentData);
        void flushDraw(RenderContext* pContext, const Mesh* pMesh, uint32_t instanceCount, CurrentWorkingData& currentData);

        void setupVR();

        CameraControllerType mCamControllerType = CameraControllerType::SixDof;
        CameraController::SharedPtr mpCameraController;

        uint32_t mMaxInstanceCount = 64;
        const Material* mpLastMaterial = nullptr;
        bool mCullEnabled = true;
        bool mUnloadTexturesOnMaterialChange = false;
        RenderMode mRenderMode = RenderMode::Mono;
        bool mCompileMaterialWithProgram = true;

        ProgramReflection const* mpSavedProgramReflection = nullptr;
        uint32_t mCameraComponentBinding = ConstantBuffer::kInvalidOffset;
        uint32_t mMeshComponentBinding = ConstantBuffer::kInvalidOffset;
		uint32_t mLightEnvBinding = ConstantBuffer::kInvalidOffset;
        uint32_t mMaterialComponentBinding = ConstantBuffer::kInvalidOffset;
        uint32_t mVertexAttributeComponentBinding = ConstantBuffer::kInvalidOffset;


        struct DrawListItem
        {
            Material*   pMaterial;
            uint32_t    modelID;
            uint32_t    modelInstanceID;
            uint32_t    meshID;
            uint32_t    meshInstanceID;
        };
        typedef std::vector<DrawListItem> DrawList;

        DrawList mDrawList;
    };
}
