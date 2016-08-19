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
#include "Utils/Gui.h"
#include "Material.h"

namespace Falcor
{
    class Material;
    class Gui;
    
    class MaterialEditor
    {
    public:
        using UniquePtr = std::unique_ptr<MaterialEditor>;
        using UniqueConstPtr = std::unique_ptr<const MaterialEditor>;

        static UniquePtr create(const Material::SharedPtr& pMaterial, bool useSrgb) {return nullptr;}
        ~MaterialEditor() {};

        void setUiVisibility(bool visible);

    private:
        MaterialEditor(const Material::SharedPtr& pMaterial, bool useSrgb) {};

        Material::SharedPtr mpMaterial = nullptr;
        Gui::UniquePtr mpGui = nullptr;
        uint32_t mActiveLayer = 0;
        bool mUseSrgb;

        void initUI();
        void refreshLayerElements() const;
        void initLayerTypeElements() const;
        void initLayerNdfElements() const;
        void initLayerBlendElements() const;

        void initLambertLayer() const;
        void initConductorLayer() const;
        void initDielectricLayer() const;
        void initEmissiveLayer() const;

        void saveMaterial();

        static void GUI_CALL saveMaterialCB(void* pUserData);

        static void GUI_CALL getNameCB(void* pVal, void* pUserData);
        static void GUI_CALL setNameCB(const void* pVal, void* pUserData);

        static void GUI_CALL getIdCB(void* pVal, void* pUserData);
        static void GUI_CALL setIdCB(const void* pVal, void* pUserData);

        static void GUI_CALL getDoubleSidedCB(void* pVal, void* pUserData);
        static void GUI_CALL setDoubleSidedCB(const void* pVal, void* pUserData);

        static void GUI_CALL addLayerCB(void* pUserData);
        static void GUI_CALL removeLayerCB(void* pUserData);

        static void GUI_CALL getActiveLayerCB(void* pVal, void* pUserData);
        static void GUI_CALL setActiveLayerCB(const void* pVal, void* pUserData);

        static void GUI_CALL getLayerTypeCB(void* pVal, void* pUserData);
        static void GUI_CALL setLayerTypeCB(const void* pVal, void* pUserData);

        static void GUI_CALL getLayerNdfCB(void* pVal, void* pUserData);
        static void GUI_CALL setLayerNdfCB(const void* pVal, void* pUserData);

        static void GUI_CALL getLayerBlendCB(void* pVal, void* pUserData);
        static void GUI_CALL setLayerBlendCB(const void* pVal, void* pUserData);

#define value_callbacks(value_)             \
        template<uint32_t Channel>          \
        static void GUI_CALL get##value_##CB(void* pVal, void* pUserData);            \
        template<uint32_t Channel>                                                    \
        static void GUI_CALL set##value_##CB(const void* pVal, void* pUserData);      \
        static void GUI_CALL load##value_##Texture(void* pUserData);                  \
        static void GUI_CALL remove##value_##Texture(void* pUserData);                \
        void init##value_##Elements() const;

        value_callbacks(Albedo);
        value_callbacks(Roughness);
        value_callbacks(ExtraParam);
        value_callbacks(Normal);
        value_callbacks(Height);
        value_callbacks(Alpha);
#undef value_callbacks

        static Material* getMaterial(void* pUserData);
        static MaterialLayerValues* getActiveLayerData(void* pUserData);
    };
}