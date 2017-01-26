/***************************************************************************
# Copyright (c) 2017, NVIDIA CORPORATION. All rights reserved.
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

#include "Graphics/Scene/SceneRenderer.h"
#include "Graphics/Model/ObjectInstance.h"

namespace Falcor
{
    class Picking : public SceneRenderer
    {
    public:
        using UniquePtr = std::unique_ptr<Picking>;
        using UniqueConstPtr = std::unique_ptr<const Picking>;

        /** Creates an instance of the scene picker.
            \param[in] pScene Scene to pick.
            \param[in] fboWidth Size of internal FBO used for picking.
            \param[in] fboHeight Size of internal FBO used for picking.
            \return New Picking instance for pScene.
        */
        static UniquePtr create(const Scene::SharedPtr& pScene, uint32_t fboWidth, uint32_t fboHeight);

        /** Performs a picking operation on the scene and stores the result.
            \param[in] mousePos Mouse position in the range [0,1] with (0,0) being the top left corner. Same coordinate space as in MouseEvent.
            \param[in] pContext Render context to render scene with.
            \return Whether an object was picked or not.
        */
        bool pick(RenderContext* pContext, const glm::vec2& mousePos, Camera* pCamera);

        /** Gets the picked mesh instance.
            \return Pointer to the picked mesh instance, otherwise nullptr if nothing was picked.
        */
        const Model::MeshInstance::SharedPtr& getPickedMeshInstance() const;

        /** Gets the picked model instance.
            \return Pointer to the picked model instance, otherwise nullptr if nothing was picked.
        */
        const Scene::ModelInstance::SharedPtr& getPickedModelInstance() const;

        /** Resize the internal FBO used for picking.
            \param[in] width Width of the FBO.
            \param[in] height Height of the FBO.
        */
        void resizeFBO(uint32_t width, uint32_t height);

    private:

        Picking(const Scene::SharedPtr& pScene, uint32_t fboWidth, uint32_t fboHeight);

        void renderScene(RenderContext* pContext, Camera* pCamera);
        void readPickResults(RenderContext* pContext);

        virtual bool setPerMeshInstanceData(RenderContext* pContext, const Scene::ModelInstance::SharedPtr& pModelInstance, const Model::MeshInstance::SharedPtr& pMeshInstance, uint32_t drawInstanceID, const CurrentWorkingData& currentData) override;
        virtual bool setPerMaterialData(RenderContext* pContext, const CurrentWorkingData& currentData) override;

        void calculateScissor(const glm::vec2& mousePos);

        static void updateVariableOffsets(const ProgramReflection* pReflector);

        static size_t sDrawIDOffset;

        struct Instance
        {
            Scene::ModelInstance::SharedPtr pModelInstance;
            Model::MeshInstance::SharedPtr pMeshInstance;

            Instance() {}

            Instance(Scene::ModelInstance::SharedPtr pModelInstance, Model::MeshInstance::SharedPtr pMeshInstance)
                : pModelInstance(pModelInstance), pMeshInstance(pMeshInstance) {}
        };

        std::unordered_map<uint32_t, Instance> mDrawIDToInstance;
        Instance mPickResult;

        Fbo::SharedPtr mpFBO;
        GraphicsProgram::SharedPtr mpProgram;
        GraphicsVars::SharedPtr mpProgramVars;
        GraphicsState::SharedPtr mpGraphicsState;

        GraphicsState::Scissor mScissor;
    };
}
