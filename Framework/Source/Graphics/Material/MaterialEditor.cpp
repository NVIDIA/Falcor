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
#include "MaterialEditor.h"
#include "Utils/Gui.h"
#include "Utils/OS.h"
#include "Graphics/TextureHelper.h"
#include "API/Texture.h"
#include "Graphics/Scene/Scene.h"
#include "Graphics/Scene/SceneExporter.h"
#if 0
namespace Falcor
{
    static const std::string kRemoveLayer = "Remove Layer";
    static const std::string kAddLayer    = "Add Layer";
    static const std::string kActiveLayer = "Active Layer";
    static const std::string kLayerType   = "Type";
    static const std::string kLayerNDF    = "NDF";
    static const std::string kLayerBlend  = "Blend";
    static const std::string kLayerGroup  = "Layer";
    static const std::string kAlbedo      = "Color";
    static const std::string kRoughness   = "Roughness";
    static const std::string kExtraParam  = "Extra Param";
    static const std::string kNormal      = "Normal";
    static const std::string kHeight      = "Height";
    static const std::string kAlpha       = "Alpha Test";
    static const std::string kAddTexture  = "Load Texture";
    static const std::string kClearTexture = "Clear Texture";

    Texture::SharedPtr loadTexture(bool useSrgb)
    {
        std::string filename;
        Texture::SharedPtr pTexture = nullptr;
        if(openFileDialog(nullptr, filename) == true)
        {
            pTexture = createTextureFromFile(filename, true, useSrgb);
            if(pTexture)
            {
                pTexture->setName(filename);
            }
        }
        return pTexture;
    }

    MaterialEditor::UniquePtr MaterialEditor::create(const Material::SharedPtr& pMaterial, bool useSrgb)
    {
        return UniquePtr(new MaterialEditor(pMaterial, useSrgb));
    }

    MaterialEditor::MaterialEditor(const Material::SharedPtr& pMaterial, bool useSrgb) : mpMaterial(pMaterial), mUseSrgb(useSrgb)
    {
        assert(pMaterial);
        // The height bias starts with 1 by default. Set it to zero
        float zero = 0;
        setHeightCB<0>(&zero, this);
        initUI();
    }

    MaterialEditor::~MaterialEditor() = default;

    Material* MaterialEditor::getMaterial(void* pUserData)
    {
        MaterialEditor* pEditor = (MaterialEditor*)pUserData;
        Material::SharedPtr pMaterial = pEditor->mpMaterial;
        return pMaterial.get();
    }

    MaterialLayer* MaterialEditor::getActiveLayer(void* pUserData)
    {
        MaterialEditor* pEditor = (MaterialEditor*)pUserData;
        const MaterialLayer* pLayer = getMaterial(pUserData)->getLayer(pEditor->mActiveLayer);
        return const_cast<MaterialLayer*>(pLayer);
    }

    void GUI_CALL MaterialEditor::saveMaterialCB(void* pUserData)
    {
        MaterialEditor* pEditor = (MaterialEditor*)pUserData;
        pEditor->saveMaterial();
    }

    void GUI_CALL MaterialEditor::getNameCB(void* pVal, void* pUserData)
    {
        MaterialEditor* pEditor = (MaterialEditor*)pUserData;
        *(std::string*)pVal = pEditor->mpMaterial->getName();
    }

    void GUI_CALL MaterialEditor::setNameCB(const void* pVal, void* pUserData)
    {
        getMaterial(pUserData)->setName(*(std::string*)pVal);
    }

    void GUI_CALL MaterialEditor::getIdCB(void* pVal, void* pUserData)
    {
        MaterialEditor* pEditor = (MaterialEditor*)pUserData;
        *(uint32_t*)pVal = pEditor->mpMaterial->getId();
    }

    void GUI_CALL MaterialEditor::setIdCB(const void* pVal, void* pUserData)
    {
        getMaterial(pUserData)->setID(*(uint32_t*)pVal);
    }

    void GUI_CALL MaterialEditor::getDoubleSidedCB(void* pVal, void* pUserData)
    {
        MaterialEditor* pEditor = (MaterialEditor*)pUserData;
        *(uint32_t*)pVal = pEditor->mpMaterial->isDoubleSided();
    }

    void GUI_CALL MaterialEditor::setDoubleSidedCB(const void* pVal, void* pUserData)
    {
        getMaterial(pUserData)->setDoubleSided(*(bool*)pVal);
    }

    void GUI_CALL MaterialEditor::addLayerCB(void* pUserData)
    {
        MaterialEditor* pEditor = (MaterialEditor*)pUserData;
        Material* pMaterial = pEditor->mpMaterial.get();
        if(pMaterial->getNumActiveLayers() >= MatMaxLayers)
        {
            msgBox("Exceeded the number of supported layers. Can't add anymore");
            return;
        }

        MaterialLayer Layer;
        Layer.type = MatLambert;    // Set a Type so that the material know we have a layer

        pMaterial->addLayer(Layer);
        pEditor->refreshLayerElements();
        pEditor->mActiveLayer = uint32_t(pMaterial->getNumActiveLayers() - 1);
    }

    void GUI_CALL MaterialEditor::removeLayerCB(void* pUserData)
    {
        MaterialEditor* pEditor = (MaterialEditor*)pUserData;
        Material* pMaterial = pEditor->mpMaterial.get();
        if(pMaterial->getNumActiveLayers() == 0)
        {
            msgBox("Material doesn't have any layer. Nothing to remove");
            return;
        }
        pMaterial->removeLayer(pEditor->mActiveLayer);
        pEditor->mActiveLayer = 0;
        pEditor->refreshLayerElements();
    }

    void GUI_CALL MaterialEditor::getActiveLayerCB(void* pVal, void* pUserData)
    {
        MaterialEditor* pEditor = (MaterialEditor*)pUserData;
        *(uint32_t*)pVal = pEditor->mActiveLayer;
    }

    void GUI_CALL MaterialEditor::setActiveLayerCB(const void* pVal, void* pUserData)
    {
        MaterialEditor* pEditor = (MaterialEditor*)pUserData;
        pEditor->mActiveLayer = *(uint32_t*)pVal;
    }

    void GUI_CALL MaterialEditor::getLayerTypeCB(void* pVal, void* pUserData)
    {
        *(uint32_t*)pVal = getActiveLayer(pUserData)->type;
    }

    void GUI_CALL MaterialEditor::setLayerTypeCB(const void* pVal, void* pUserData)
    {
        MaterialEditor* pEditor = (MaterialEditor*)pUserData;
        getActiveLayer(pUserData)->type = *(uint32_t*)pVal;
        pEditor->refreshLayerElements();
    }

    void GUI_CALL MaterialEditor::getLayerNdfCB(void* pVal, void* pUserData)
    {
        *(uint32_t*)pVal = getActiveLayer(pUserData)->ndf;
    }

    void GUI_CALL MaterialEditor::setLayerNdfCB(const void* pVal, void* pUserData)
    {
        getActiveLayer(pUserData)->ndf = *(uint32_t*)pVal;
    }

    void GUI_CALL MaterialEditor::getLayerBlendCB(void* pVal, void* pUserData)
    {
        *(uint32_t*)pVal = getActiveLayer(pUserData)->blending;
    }

    void GUI_CALL MaterialEditor::setLayerBlendCB(const void* pVal, void* pUserData)
    {
        getActiveLayer(pUserData)->blending = *(uint32_t*)pVal;
    }

#define layer_value_callbacks(value_, value_stmt_)                                          \
    template<uint32_t channel>                                                              \
    void GUI_CALL MaterialEditor::get##value_##CB(void* pVal, void* pUserData)              \
    {                                                                                       \
        *(float*)pVal = value_stmt_.constantColor[channel];                                 \
    }                                                                                       \
                                                                                            \
    template<uint32_t channel>                                                              \
    void GUI_CALL MaterialEditor::set##value_##CB(const void* pVal, void* pUserData)        \
    {                                                                                       \
        value_stmt_.constantColor[channel] = *(float*)pVal;                                 \
    }                                                                                       \
                                                                                            \
    void GUI_CALL MaterialEditor::load##value_##Texture(void* pUserData)                    \
    {                                                                                       \
        MaterialEditor* pEditor = (MaterialEditor*)pUserData;                               \
        auto pTexture = loadTexture(pEditor->mUseSrgb);                                     \
        if(pTexture)                                                                        \
        {                                                                                   \
            value_stmt_.texture.pTexture = pTexture.get();                                  \
        }                                                                                   \
    }                                                                                       \
                                                                                            \
    void GUI_CALL MaterialEditor::remove##value_##Texture(void* pUserData)                  \
    {                                                                                       \
        value_stmt_.texture.pTexture = nullptr;                                             \
    }

    layer_value_callbacks(Albedo, getActiveLayerData(pUserData)->albedo);
    layer_value_callbacks(Roughness, getActiveLayerData(pUserData)->roughness);
    layer_value_callbacks(ExtraParam, getActiveLayerData(pUserData)->extraParam);
    layer_value_callbacks(Normal, const_cast<MaterialValue&>(getMaterial(pUserData)->getNormalValue()));
    layer_value_callbacks(Alpha, const_cast<MaterialValue&>(getMaterial(pUserData)->getAlphaValue()));
    layer_value_callbacks(Height, const_cast<MaterialValue&>(getMaterial(pUserData)->getHeightValue()));
#undef layer_value_callbacks

    void MaterialEditor::refreshLayerElements() const
    {
        mpGui->removeGroup(kLayerGroup);

        bool hasLayers = mpMaterial->getNumActiveLayers() > 0;
        mpGui->setVarVisibility(kActiveLayer, "", hasLayers);

        if(hasLayers)
        {
            initLayerTypeElements();
            initLayerBlendElements();

            mpGui->setVarRange(kActiveLayer, "", 0, int32_t(mpMaterial->getNumActiveLayers() - 1));
            switch(mpMaterial->getLayer(mActiveLayer)->type)
            {
            case MatLambert:
                initLambertLayer();
                break;
            case MatConductor:
                initConductorLayer();
                break;
            case MatDielectric:
                initDielectricLayer();
                break;
            case MatEmissive:
                initEmissiveLayer();
                break;
            default:
                should_not_get_here();
                break;
            }
            mpGui->expandGroup(kLayerGroup);
        }
    }

    void MaterialEditor::initLayerTypeElements() const
    {
        Gui::dropdown_list layerTypes;
        layerTypes.push_back({MatLambert,    "Lambert"});
        layerTypes.push_back({MatConductor,  "Conductor"});
        layerTypes.push_back({MatDielectric, "Dielectric"});
        layerTypes.push_back({MatEmissive,   "Emissive"});
        layerTypes.push_back({MatUser,       "Custom"});

        mpGui->addDropdownWithCallback(kLayerType, layerTypes, &MaterialEditor::setLayerTypeCB, &MaterialEditor::getLayerTypeCB, (void*)this, kLayerGroup);
    }

    void MaterialEditor::initLayerBlendElements() const
    {
        Gui::dropdown_list blendTypes;
        blendTypes.push_back({BlendFresnel,  "Fresnel"});
        blendTypes.push_back({BlendConstant, "Constant Factor"});
        blendTypes.push_back({BlendAdd, "Additive"});

        mpGui->addDropdownWithCallback(kLayerBlend, blendTypes, &MaterialEditor::setLayerBlendCB, &MaterialEditor::getLayerBlendCB, (void*)this, kLayerGroup);
    }

    void MaterialEditor::initLayerNdfElements() const
    {
        Gui::dropdown_list ndfTypes;
        ndfTypes.push_back({NDFBeckmann, "Beckmann"});
        ndfTypes.push_back({NDFGGX, "GGX"});
        ndfTypes.push_back({NDFUser, "User Defined"});

        mpGui->addDropdownWithCallback(kLayerNDF, ndfTypes, &MaterialEditor::setLayerNdfCB, &MaterialEditor::getLayerNdfCB, (void*)this, kLayerGroup);
    }

#define load_textures(value_)       \
        mpGui->addButton(kAddTexture, &MaterialEditor::load##value_##Texture, (void*)this, k##value_);                                                                 \
        mpGui->addButton(kClearTexture, &MaterialEditor::remove##value_##Texture, (void*)this, k##value_);

#define add_value_var(value_, name_, channel_, minval_, maxval_, group_) \
    mpGui->addFloatVarWithCallback(name_, &MaterialEditor::set##value_##CB<channel_>, &MaterialEditor::get##value_##CB<channel_>, (void*)this, group_, 0, maxval_, 0.001f);

    void MaterialEditor::initAlbedoElements() const
    {
        load_textures(Albedo);
        add_value_var(Albedo, "r", 0, 0, FLT_MAX, "Base Color");
        add_value_var(Albedo, "g", 1, 0, FLT_MAX, "Base Color");
        add_value_var(Albedo, "b", 2, 0, FLT_MAX, "Base Color");
        mpGui->nestGroups(kAlbedo, "Base Color");
        mpGui->nestGroups(kLayerGroup, kAlbedo);
    }

    void MaterialEditor::initRoughnessElements() const
    {
        load_textures(Roughness);
        add_value_var(Roughness, "Base Roughness", 0, 0, FLT_MAX, kRoughness);
        mpGui->nestGroups(kLayerGroup, kRoughness);
    }

    void MaterialEditor::initNormalElements() const
    {
        load_textures(Normal);
    }

    void MaterialEditor::initAlphaElements() const
    {
        load_textures(Alpha);
        add_value_var(Alpha, "Alpha Threshold", 0, 0, 1, kAlpha);
    }

    void MaterialEditor::initHeightElements() const
    {
        load_textures(Height);
        add_value_var(Height, "Height Bias", 0, -FLT_MAX, FLT_MAX, kHeight);
        add_value_var(Height, "Height Scale", 1, 0, FLT_MAX, kHeight);
    }

    void MaterialEditor::initLambertLayer() const
    {
        initAlbedoElements();
    }

    void MaterialEditor::initConductorLayer() const
    {
        initAlbedoElements();
        initRoughnessElements();
        add_value_var(ExtraParam, "Real Part", 0, 0, FLT_MAX, "IoR");
        add_value_var(ExtraParam, "Imaginery Part", 1, 0, FLT_MAX, "IoR");
        mpGui->nestGroups(kLayerGroup, "IoR");
        initLayerNdfElements();
    }

    void MaterialEditor::initDielectricLayer() const
    {
        initAlbedoElements();
        initRoughnessElements();
        add_value_var(ExtraParam, "IoR", 0, 0, FLT_MAX, kLayerGroup);
    }

    void MaterialEditor::initEmissiveLayer() const
    {
        initAlbedoElements();
    }

#undef load_textures

    void MaterialEditor::initUI()
    {
        mpGui = Gui::create("Material Editor");
        mpGui->setSize(300, 300);
        mpGui->setPosition(50, 300);

        mpGui->addButton("Save Material", &MaterialEditor::saveMaterialCB, this);
        mpGui->addSeparator();
        mpGui->addTextBoxWithCallback("Name", &MaterialEditor::setNameCB, &MaterialEditor::getNameCB, this);
        mpGui->addIntVarWithCallback("ID", &MaterialEditor::setIdCB, &MaterialEditor::getIdCB, this, "", 0);
        mpGui->addCheckBoxWithCallback("Double-Sided", &MaterialEditor::setDoubleSidedCB, &MaterialEditor::getDoubleSidedCB, this);

        mpGui->addSeparator("");

        initNormalElements();
        initAlphaElements();
        initHeightElements();

        mpGui->addSeparator("");
        mpGui->addButton(kAddLayer, &MaterialEditor::addLayerCB, this);
        mpGui->addButton(kRemoveLayer, &MaterialEditor::removeLayerCB, this);
        mpGui->addSeparator("");
        mpGui->addIntVarWithCallback(kActiveLayer, &MaterialEditor::setActiveLayerCB, &MaterialEditor::getActiveLayerCB, this);

        refreshLayerElements();
    }

    void MaterialEditor::saveMaterial()
    {
        std::string filename;
        if(saveFileDialog("Scene files\0*.fscene\0\0", filename))
        {
            // Using the scene exporter
            Scene::SharedPtr pScene = Scene::create();
            pScene->addMaterial(mpMaterial);

            SceneExporter::saveScene(filename, pScene.get(), SceneExporter::ExportMaterials);
        }
    }
}
#endif