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
#include "BinaryModelImporter.h"
#include "BinaryModelSpec.h"
#include "../Model.h"
#include "../Mesh.h"
#include "Utils/OS.h"
#include "API/VertexLayout.h"
#include "Data/VertexAttrib.h"
#include "API/Buffer.h"
#include "glm/common.hpp"
#include "BinaryImage.hpp"
#include "API/Formats.h"
#include "API/Texture.h"
#include "Graphics/Material/Material.h"
#include "glm/geometric.hpp"

namespace Falcor
{
    struct TextureData
    {
        uint32_t width  = 0;
        uint32_t height = 0;
        ResourceFormat format = ResourceFormat::Unknown;
        std::vector<uint8_t> data;
        std::string name;
    };

    bool isSpecialFloat(float f)
    {
        uint32_t d = *(uint32_t*)&f;
        // Check the exponent
        d = (d >> 23) & 0xff;
        return d == 0xff;
    }

    template<typename posType>
    void generateSubmeshTangentData(
        const std::vector<uint32_t>& indices,
        const posType* vertexPosData,
        const glm::vec3* vertexNormalData,
        const glm::vec2* texCrdData,
        uint32_t texCrdCount,
        glm::vec3* tangentData,
        glm::vec3* bitangentData)
    {
        // calculate the tangent and bitangent for every face
        size_t primCount = indices.size() / 3;
        for(size_t primID = 0; primID < primCount; primID++)
        {
            struct Data
            {
                posType position;
                glm::vec3 normal;
                glm::vec2 uv;
            };
            Data V[3];

            // Get the data
            for(uint32_t i = 0; i < 3; i++)
            {
                uint32_t index = indices[primID * 3 + i];
                V[i].position = vertexPosData[index];
                V[i].normal = vertexNormalData[index];

                if(texCrdData)
                {
                    V[i].uv = texCrdData[index * texCrdCount];
                }
                else
                {
                    V[i].uv = glm::vec2(0.f, 0.f);
                }
            }

            // Position delta
            posType posDelta[2];
            posDelta[0] = V[1].position - V[0].position;
            posDelta[1] = V[2].position - V[0].position;

            // Texture offset
            glm::vec2 s = V[1].uv - V[0].uv;
            glm::vec2 t = V[2].uv - V[0].uv;
            s.y = -s.y;
            t.y = -t.y;

            float dirCorrection = (t.x * s.y - t.y * s.x) < 0.0f ? -1.0f : 1.0f;
            // when t1, t2, t3 in same position in UV space, just use default UV direction.
            if((s == glm::vec2(0, 0)) && (t == glm::vec2(0, 0)))
            {
                s.x = 0.0;
                s.y = 1.0;
                t.x = 1.0;
                t.y = 0.0;
            }

            // tangent points in the direction where to positive X axis of the texture coord's would point in model space
            // bitangent's points along the positive Y axis of the texture coord's, respectively
            glm::vec3 tangent;
            glm::vec3 bitangent;
            tangent.x = (posDelta[1].x * s.y - posDelta[0].x * t.y) * dirCorrection;
            tangent.y = (posDelta[1].y * s.y - posDelta[0].y * t.y) * dirCorrection;
            tangent.z = (posDelta[1].z * s.y - posDelta[0].z * t.y) * dirCorrection;

            bitangent.x = (posDelta[1].x * s.x - posDelta[0].x * t.x) * dirCorrection;
            bitangent.y = (posDelta[1].y * s.x - posDelta[0].y * t.x) * dirCorrection;
            bitangent.z = (posDelta[1].z * s.x - posDelta[0].z * t.x) * dirCorrection;

            // store for every vertex of that face
            for(uint32_t i = 0; i < 3; i++)
            {
                // project tangent and bitangent into the plane formed by the vertex' normal
                glm::vec3 localTangent = tangent - V[i].normal * (glm::dot(tangent, V[i].normal));
                localTangent = glm::normalize(localTangent);
                glm::vec3 localBitangent = bitangent - V[i].normal * (glm::dot(bitangent, V[i].normal));
                localBitangent = glm::normalize(localBitangent);

                // reconstruct tangent/bitangent according to normal and bitangent/tangent when it's infinite or NaN.
                bool isInvalidTangent = isSpecialFloat(localTangent.x) || isSpecialFloat(localTangent.y) || isSpecialFloat(localTangent.z);
                bool isInvalidBitangent = isSpecialFloat(localBitangent.x) || isSpecialFloat(localBitangent.y) || isSpecialFloat(localBitangent.z);

                if(isInvalidTangent != isInvalidBitangent)
                {
                    if(isInvalidTangent)
                    {
                        localTangent = glm::cross(V[i].normal, localBitangent);
                        localTangent = glm::normalize(localTangent);
                    }
                    else
                    {
                        localBitangent = glm::cross(localTangent, V[i].normal);
                        localBitangent = glm::normalize(localBitangent);
                    }
                }

                // and write it into the mesh
                uint32_t index = indices[primID * 3 + i];
                tangentData[index] = localTangent;
                bitangentData[index] = localBitangent;
            }
        }
    }

    static BasicMaterial::MapType getFalcorMapType(TextureType map)
    {
        switch(map)
        {
        case TextureType_Diffuse:
            return BasicMaterial::MapType::DiffuseMap;
        case TextureType_Alpha:
            return BasicMaterial::MapType::AlphaMap;
        case TextureType_Normal:
            return BasicMaterial::MapType::NormalMap;
        case TextureType_Specular:
            return BasicMaterial::MapType::SpecularMap;
        case TextureType_Displacement:
            return BasicMaterial::MapType::HeightMap;
        default:
            return BasicMaterial::MapType::Count;
        }
    }

    static const std::string getSemanticName(AttribType type)
    {
        switch(type)
        {
        case AttribType_Position:
            return VERTEX_POSITION_NAME;
        case AttribType_Normal:
            return VERTEX_NORMAL_NAME;
        case AttribType_Color:
            return VERTEX_DIFFUSE_COLOR_NAME;
        case AttribType_TexCoord:
            return VERTEX_TEXCOORD_NAME;
        case AttribType_Tangent:
            return VERTEX_TANGENT_NAME;
        case AttribType_Bitangent:
            return VERTEX_BITANGENT_NAME;
        default:
            should_not_get_here();
            return "";
        }
    }

    static ResourceFormat getFalcorFormat(AttribFormat format, uint32_t components)
    {
        switch(format)
        {
        case AttribFormat_U8:
            switch(components)
            {
            case 1:
                return ResourceFormat::R8Unorm;
            case 2:
                return ResourceFormat::RG8Unorm;
            case 3:
                return ResourceFormat::RGBX8Unorm;
            case 4:
                return ResourceFormat::RGBA8Unorm;
            }
            break;
        case AttribFormat_S32:
            switch(components)
            {
            case 1:
                return ResourceFormat::R32Int;
            case 2:
                return ResourceFormat::RG32Int;
            case 3:
                return ResourceFormat::RGB32Int;
            case 4:
                return ResourceFormat::RGBA32Int;
            }
            break;
        case AttribFormat_F32:
            switch(components)
            {
            case 1:
                return ResourceFormat::R32Float;
            case 2:
                return ResourceFormat::RG32Float;
            case 3:
                return ResourceFormat::RGB32Float;
            case 4:
                return ResourceFormat::RGBA32Float;
            }
            break;
        }
        should_not_get_here();
        return ResourceFormat::Unknown;
    }

    static uint32_t getShaderLocation(AttribType type)
    {
        switch(type)
        {
        case AttribType_Position:
            return VERTEX_POSITION_LOC;
        case AttribType_Normal:
            return VERTEX_NORMAL_LOC;
        case AttribType_Color:
            return VERTEX_DIFFUSE_COLOR_LOC;
        case AttribType_TexCoord:
            return VERTEX_TEXCOORD_LOC;
        case AttribType_Tangent:
            return VERTEX_TANGENT_LOC;
        case AttribType_Bitangent:
            return VERTEX_BITANGENT_LOC;
        default:
            should_not_get_here();
            return 0;
        }
    }

    static uint32_t getFormatByteSize(AttribFormat format)
    {
        switch(format)
        {
        case AttribFormat_U8:
            return 1;
        case AttribFormat_S32:
        case AttribFormat_F32:
            return 4;
        default:
            should_not_get_here();
            return 0;
        }
    }

    static ResourceFormat getTextureFormat(FW::ImageFormat::ID formatId)
    {
        // Note: If you make changes here, make sure to update GetFormatFromMapType()
        switch(formatId)
        {
        case FW::ImageFormat::R8_G8_B8:
            return ResourceFormat::RGBX8Unorm;
        case FW::ImageFormat::R8_G8_B8_A8:
            return ResourceFormat::RGBA8Unorm;
        case FW::ImageFormat::A8:
            return ResourceFormat::Alpha8Unorm;
        case FW::ImageFormat::XBGR_8888:
            return ResourceFormat::RGBX8Unorm;
        case FW::ImageFormat::ABGR_8888:
            return ResourceFormat::RGBA8Unorm;
        case FW::ImageFormat::RGB_565:
            return ResourceFormat::R5G6B5Unorm;
        case FW::ImageFormat::RGBA_5551:
            return ResourceFormat::RGB5A1Unorm;
        case FW::ImageFormat::RGB_Vec3f:
            return ResourceFormat::RGB32Float;
        case FW::ImageFormat::RGBA_Vec4f:
            return ResourceFormat::RGBA32Float;
        case FW::ImageFormat::A_F32:
            return ResourceFormat::Alpha32Float;

        case FW::ImageFormat::BGRA_8888:
            return ResourceFormat::BGRA8Unorm;
        case FW::ImageFormat::BGR_888:
            return ResourceFormat::BGRA8Unorm;
        case FW::ImageFormat::RG_88:
            return ResourceFormat::RG8Unorm;
        case FW::ImageFormat::R8:
            return ResourceFormat::R8Unorm;

        case FW::ImageFormat::S3TC_DXT1:
            return ResourceFormat::BC1Unorm;
        case FW::ImageFormat::S3TC_DXT3:
            return ResourceFormat::BC2Unorm;
        case FW::ImageFormat::S3TC_DXT5:
            return ResourceFormat::BC3Unorm;
        case FW::ImageFormat::RGTC_R:
            return ResourceFormat::BC4Unorm;
        case FW::ImageFormat::RGTC_RG:
            return ResourceFormat::BC5Unorm;

        default:
            should_not_get_here();
            return ResourceFormat::Unknown;
        }
    }

    std::string readString(BinaryFileStream& stream)
    {
        int32_t length;
        stream >> length;
        std::vector<char> charVec(length + 1);
        stream.read(&charVec[0], length);
        charVec[length] = 0;
        return std::string(charVec.data());
    }

    bool loadBinaryTextureData(BinaryFileStream& stream, const std::string& modelName, TextureData& data)
    {
        // ImageHeader.
        char tag[9];
        stream.read(tag, 8);
        tag[8] = '\0';
        if(std::string(tag) != "BinImage")
        {
            std::string msg = "Error when loading model " + modelName + ".\nBinary image header corrupted.";
            logError(msg);
            return false;
        }

        int32_t version;
        stream >> version;

        if(version < 1 || version > 2)        
        {
            std::string msg = "Error when loading model " + modelName + ".\nUnsupported binary image version.";
            logError(msg);
            return false;
        }

        int32_t bpp, numChannels;
        stream >> data.width >> data.height >> bpp >> numChannels;
        if(data.width < 0 || data.height < 0 || bpp < 0 || numChannels < 0)
        {
            std::string msg = "Error when loading model " + modelName + ".\nCorrupt binary image version.";
            logError(msg);
            return false;
        }

        int32_t dataSize = -1;
        int32_t formatId = FW::ImageFormat::ID_Max;
        FW::ImageFormat format;
        if(version >= 2)
        {
            stream >> formatId >> dataSize;
            if(formatId < 0 || formatId >= FW::ImageFormat::ID_Generic || dataSize < 0)
            {
                std::string msg = "Error when loading model " + modelName + ".\nCorrupt binary image data (unsupported image format).";
                logError(msg);
                return false;
            }
            format = FW::ImageFormat(FW::ImageFormat::ID(formatId));
        }

        // Array of ImageChannel.
        for(int i = 0; i < numChannels; i++)
        {
            int32_t ctype, cformat;
            FW::ImageFormat::Channel c;
            stream >> ctype >> cformat >> c.wordOfs >> c.wordSize >> c.fieldOfs >> c.fieldSize;
            if(ctype < 0 || cformat < 0 || cformat >= FW::ImageFormat::ChannelFormat_Max ||
                c.wordOfs < 0 || (c.wordSize != 1 && c.wordSize != 2 && c.wordSize != 4) ||
                c.fieldOfs < 0 || c.fieldSize <= 0 || c.fieldOfs + c.fieldSize > c.wordSize * 8 ||
                (cformat == FW::ImageFormat::ChannelFormat_Float && c.fieldSize != 32))
            {
                std::string msg = "Error when loading model " + modelName + ".\nCorrupt binary image data (unsupported floating point format).";
                logError(msg);
                return false;
            }

            c.Type = (FW::ImageFormat::ChannelType)ctype;
            c.format = (FW::ImageFormat::ChannelFormat)cformat;
            format.addChannel(c);
        }

        if(bpp != format.getBPP())
        {
            std::string msg = "Error when loading model " + modelName + ".\nCorrupt binary image data (bits/pixel do not match with format).";
            logError(msg);
            return false;
        }

        // Format
        if(formatId == FW::ImageFormat::ID_Max)
            formatId = format.getID();
        data.format = getTextureFormat(FW::ImageFormat::ID(formatId));

        // Image data.
        const int32_t texelCount = data.width * data.height;
        if(dataSize == -1)
        {
            dataSize = bpp * texelCount;
        }
        size_t storageSize = dataSize;
        if(bpp == 3)
            storageSize = 4 * texelCount;

        data.data.resize(storageSize);
        stream.read(data.data.data(), dataSize);

        // Convert 3-channel 8-bits RGB formats to 4-channel RGBX by adding padding
        if(bpp == 3)
        {
            for(int32_t i=texelCount-1;i>=0;--i)
            {
                data.data[i * 4 + 0] = data.data[i * 3 + 0];
                data.data[i * 4 + 1] = data.data[i * 3 + 1];
                data.data[i * 4 + 2] = data.data[i * 3 + 2];
                data.data[i * 4 + 3] = 0xff;
            }
        }

        return true;
    }

    bool importTextures(std::vector<TextureData>& textures, uint32_t textureCount, BinaryFileStream& stream, const std::string& modelName)
    {
        textures.assign(textureCount, TextureData());

        for(uint32_t i = 0; i < textureCount; i++)
        {
            textures[i].name = readString(stream);
            if(loadBinaryTextureData(stream, modelName, textures[i]) == false)
            {
                return false;
            }
        }

        return true;
    }

    BinaryModelImporter::BinaryModelImporter(const std::string& fullpath) : mModelName(fullpath), mStream(fullpath.c_str(), BinaryFileStream::Mode::Read)
    {
    }

    Model::SharedPtr BinaryModelImporter::createFromFile(const std::string& filename, uint32_t flags)
    {
        std::string fullpath;
        if(findFileInDataDirectories(filename, fullpath) == false)
        {
            logError(std::string("Can't find model file ") + filename);
            return nullptr;
        }

        BinaryModelImporter loader(fullpath);
        Model::SharedPtr pModel = loader.createModel(flags);
        return pModel;
    }

    static bool checkVersion(const std::string& formatID, uint32_t version, const std::string& modelName)
    {
        if(std::string(formatID) == "BinScene")
        {
            if(version < 6 || version > 8)
            {
                std::string Msg = "Error when loading model " + modelName + ".\nUnsupported binary scene version " + std::to_string(version);
                logError(Msg);
                return false;
            }
        }
        else if(std::string(formatID) == "BinMesh ")
        {
            if(version < 1 || version > 5)
            {
                std::string Msg = "Error when loading model " + modelName + ".\nUnsupported binary scene version " + std::to_string(version);
                logError(Msg);
                return false;
            }
        }
        else
        {
            std::string Msg = "Error when loading model " + modelName + ".\nNot a binary scene file!";
            logError(Msg);
            return false;
        }
        return true;
    }

    ResourceFormat convertFormatToSrgb(ResourceFormat format)
    {
        switch(format)
        {
        case ResourceFormat::RGBX8Unorm:
            return ResourceFormat::RGBX8UnormSrgb;
        case ResourceFormat::RGBA8Unorm:
            return ResourceFormat::RGBA8UnormSrgb;
        case ResourceFormat::BGRA8Unorm:
            return ResourceFormat::BGRA8UnormSrgb;
        case ResourceFormat::BGRX8Unorm:
            return ResourceFormat::BGRX8UnormSrgb;
        case ResourceFormat::BC1Unorm:
            return ResourceFormat::BC1UnormSrgb;
        case ResourceFormat::BC2Unorm:
            return ResourceFormat::BC2UnormSrgb;
        case ResourceFormat::BC3Unorm:
            return ResourceFormat::BC3UnormSrgb;
        default:
            logWarning("BinaryModelImporter::ConvertFormatToSrgb() warning. Provided format doesn't have a matching sRGB format");
            return format;
        }
    }
    
    ResourceFormat getFormatFromMapType(bool requestSrgb, ResourceFormat originalFormat, BasicMaterial::MapType mapType)
    {
        if(requestSrgb == false)
        {
            return originalFormat;
        }

        switch(mapType)
        {
        case Falcor::BasicMaterial::DiffuseMap:
        case Falcor::BasicMaterial::SpecularMap:
        case Falcor::BasicMaterial::EmissiveMap:
            return convertFormatToSrgb(originalFormat);
        case Falcor::BasicMaterial::NormalMap:
        case Falcor::BasicMaterial::AlphaMap:
        case Falcor::BasicMaterial::HeightMap:
            return originalFormat;
        default:
            should_not_get_here();
            return originalFormat;
        }
    }
    
    Model::SharedPtr BinaryModelImporter::createModel(uint32_t flags)
    {
        // Format ID and version.
        char formatID[9];
        mStream.read(formatID, 8);
        formatID[8] = '\0';

        uint32_t version;
        mStream >> version;

        // Check if the version matches
        if(checkVersion(formatID, version, mModelName) == false)
        {
            return nullptr;
        }

        int numTextureSlots;
        int numAttributesType = AttribType_AORadius + 1;

        switch(version)
        {
        case 1:     numTextureSlots = 0; break;
        case 2:     numTextureSlots = TextureType_Alpha + 1; break;
        case 3:     numTextureSlots = TextureType_Displacement + 1; break;
        case 4:     numTextureSlots = TextureType_Environment + 1; break;
        case 5:     numTextureSlots = TextureType_Specular + 1; break;
        case 6:     numTextureSlots = TextureType_Specular + 1; break;
        case 7:     numTextureSlots = TextureType_Glossiness + 1; break;
        case 8:     numTextureSlots = TextureType_Glossiness + 1; numAttributesType = AttribType_Max; break;
        default:
            should_not_get_here();
            return nullptr;
        }


        // File header
        int32_t numTextures = 0;
        int32_t numMeshes = 0;
        int32_t numInstances = 0;
        int32_t numAttribs_v5 = 0;
        int32_t numVertices_v5 = 0;
        int32_t numSubmeshes_v5 = 0;

        if(version >= 6)
        {
            mStream >> numTextures >> numMeshes >> numInstances;
        }
        else
        {
            numMeshes = 1;
            numInstances = 1;
            mStream >> numAttribs_v5 >> numVertices_v5 >> numSubmeshes_v5;
            if(version >= 2)
            {
                mStream >> numTextures;
            }
        }

        if(numTextures < 0 || numMeshes < 0 || numInstances < 0)
        {
            std::string msg = "Error when loading model " + mModelName + ".\nFile is corrupted.";
            logError(msg);
            return nullptr;
        }

        // create objects
        auto pModel = Model::create();
        bool shouldGenerateTangents = (flags & Model::GenerateTangentSpace) != 0;

        std::vector<TextureData> texData;

        if(version >= 6)
        {
            importTextures(texData, numTextures, mStream, mModelName);
        }

        // This file format has a concept of sub-meshes, which Falcor model doesn't have - Falcor creates a new mesh for each sub-mesh
        // When creating instances of meshes, it means we need to translate the original mesh index to all it's submeshes Falcor IDs. This is what the next 2 variables are for.
        std::vector<std::vector<uint32_t>> meshToSubmeshesID(numMeshes);

        // This importer loads mesh/submesh data before instance data, so the meshes are cached here.
        std::vector<Mesh::SharedPtr> falcorMeshCache;
        
        struct TexSignature
        {
            const uint8_t* pData;
            ResourceFormat format;
            bool operator<(const TexSignature& other) const 
            { 
                if(pData < other.pData) return true;
                if(pData == other.pData) return format < other.format;
                return false;
            }
            bool operator==(const TexSignature& other) const { return pData == other.pData || format == other.format; }
        };
        std::map<TexSignature, Texture::SharedPtr> textures;
        bool loadTexAsSrgb = (flags & Model::AssumeLinearSpaceTextures) ? false : true;

        // Load the meshes
        for(int meshIdx = 0; meshIdx < numMeshes; meshIdx++)
        {
            // Mesh header
            int32_t numAttribs = 0;
            int32_t numVertices = 0;
            int32_t numSubmeshes = 0;

            if(version >= 6)
            {
                mStream >> numAttribs >> numVertices >> numSubmeshes;
            }
            else
            {
                numAttribs = numAttribs_v5;
                numVertices = numVertices_v5;
                numSubmeshes = numSubmeshes_v5;
            }

            if(numAttribs < 0 || numVertices < 0 || numSubmeshes < 0)
            {
                std::string Msg = "Error when loading model " + mModelName + ".\nCorrupted data.!";
                logError(Msg);
                return nullptr;
            }

            Vao::BufferVec pVBs;
            VertexLayout::SharedPtr pLayout = VertexLayout::create();
            std::vector<std::vector<uint8_t> > buffers;
            pVBs.resize(numAttribs);
            buffers.resize(numAttribs);

            const uint32_t kInvalidBufferIndex = (uint32_t)-1;
            uint32_t positionBufferIndex = kInvalidBufferIndex;
            uint32_t normalBufferIndex = kInvalidBufferIndex;
            uint32_t tangentBufferIndex = kInvalidBufferIndex;
            uint32_t bitangentBufferIndex = kInvalidBufferIndex;
            uint32_t texCoordBufferIndex = kInvalidBufferIndex;

            for(int i = 0; i < numAttribs; i++)
            {
                VertexBufferLayout::SharedPtr pBufferLayout = VertexBufferLayout::create();
                pLayout->addBufferLayout(i, pBufferLayout);
                int32_t type, format, length;
                mStream >> type >> format >> length;
                if(type < 0 || type >= numAttributesType || format < 0 || format >= AttribFormat::AttribFormat_Max || length < 1 || length > 4)
                {
                    std::string msg = "Error when loading model " + mModelName + ".\nCorrupted data.!";
                    logError(msg);
                    return nullptr;
                }
                else
                {
                    const std::string falcorName = getSemanticName(AttribType(type));
                    ResourceFormat falcorFormat = getFalcorFormat(AttribFormat(format), length);
                    uint32_t shaderLocation = getShaderLocation(AttribType(type));

                    switch (shaderLocation)
                    {
                    case VERTEX_POSITION_LOC:
                        positionBufferIndex = i;
                        assert(falcorFormat == ResourceFormat::RGB32Float || falcorFormat == ResourceFormat::RGBA32Float);
                        break;
                    case VERTEX_NORMAL_LOC:
                        normalBufferIndex = i;
                        assert(falcorFormat == ResourceFormat::RGB32Float);
                        break;
                    case VERTEX_TANGENT_LOC:
                        tangentBufferIndex = i;
                        assert(falcorFormat == ResourceFormat::RGB32Float);
                        break;
                    case VERTEX_BITANGENT_LOC:
                        bitangentBufferIndex = i;
                        assert(falcorFormat == ResourceFormat::RGB32Float);
                        break;
                    case VERTEX_TEXCOORD_LOC:
                        texCoordBufferIndex = i;
                        break;
                    }

                    pBufferLayout->addElement(falcorName, 0, falcorFormat, 1, shaderLocation);
                    buffers[i].resize(pBufferLayout->getStride() * numVertices);
                }
            }

            
            // Check if we need to generate tangents  
            bool genTangentForMesh = false;
            if(shouldGenerateTangents && (tangentBufferIndex == kInvalidBufferIndex) && (bitangentBufferIndex == kInvalidBufferIndex))
            {
                if(normalBufferIndex == kInvalidBufferIndex)
                {
                    logWarning("Can't generate tangent space for mesh " + std::to_string(meshIdx) + " when loading model " + mModelName + ".\nMesh doesn't contain normals or texture coordinates\n");
                    genTangentForMesh = false;
                }
                else
                {
                    if(texCoordBufferIndex == kInvalidBufferIndex)
                    {
                        logWarning("No uv mapping is provided to generate tangent space for mesh " + std::to_string(meshIdx) + " when loading model " + mModelName + ".\nMesh doesn't contain normals or texture coordinates\n");
                    }
                    // Set the offsets
                    genTangentForMesh = true;
                    tangentBufferIndex = (uint32_t)pVBs.size();
                    bitangentBufferIndex = (uint32_t)pVBs.size() + 1;
                    pVBs.resize(bitangentBufferIndex + 1);
                    buffers.resize(bitangentBufferIndex + 1);
                   
                    auto pTangentLayout = VertexBufferLayout::create();
                    pLayout->addBufferLayout(tangentBufferIndex, pTangentLayout);
                    pTangentLayout->addElement(VERTEX_TANGENT_NAME, 0, ResourceFormat::RGB32Float, 1, VERTEX_TANGENT_LOC);
                    buffers[tangentBufferIndex].resize(sizeof(glm::vec3) * numVertices);

                    auto pBitangentLayout = VertexBufferLayout::create();
                    pLayout->addBufferLayout(bitangentBufferIndex, pBitangentLayout);
                    pBitangentLayout->addElement(VERTEX_BITANGENT_NAME, 0, ResourceFormat::RGB32Float, 1, VERTEX_BITANGENT_LOC);
                    buffers[bitangentBufferIndex].resize(sizeof(glm::vec3) * numVertices);
                }
            }
            

            // Read the data
            // Read one vertex at a time
            for(int32_t i = 0; i < numVertices; i++)
            {
                for (int32_t attributes = 0; attributes < numAttribs; ++attributes)
                {
                    uint32_t stride = pLayout->getBufferLayout(attributes)->getStride();
                    uint8_t* pDest = buffers[attributes].data() + stride * i;
                    mStream.read(pDest, stride);
                }
            }

            for (int32_t i = 0; i < numAttribs; ++i)
            {
                pVBs[i] = Buffer::create(buffers[i].size(), Buffer::BindFlags::Vertex, Buffer::CpuAccess::None, buffers[i].data());
            }

            if(version <= 5)
            {
                importTextures(texData, numTextures, mStream, mModelName);
                textures.clear();
            }

            // Array of Submesh.
            // Falcor doesn't have a concept of submeshes, just create a new mesh for each submesh
            for(int submesh = 0; submesh < numSubmeshes; submesh++)
            {
                // create the material
                BasicMaterial basicMaterial;

                glm::vec3 ambient;
                glm::vec4 diffuse;
                glm::vec3 specular;
                float glossiness;

                mStream >> ambient >> diffuse >> specular >> glossiness;
                basicMaterial.diffuseColor = glm::vec3(diffuse);
                basicMaterial.opacity = 1 - diffuse.w;
                basicMaterial.specularColor = specular;
                basicMaterial.shininess = glossiness;

                if(version >= 3)
                {
                    float displacementCoeff;
                    float displacementBias;
                    mStream >> displacementCoeff >> displacementBias;
                    basicMaterial.bumpScale = displacementCoeff;
                    basicMaterial.bumpOffset = displacementBias;
                }

                for(int i = 0; i < numTextureSlots; i++)
                {
                    int32_t texID;
                    mStream >> texID;
                    if(texID < -1 || texID >= numTextures)
                    {
                        std::string msg = "Error when loading model " + mModelName + ".\nCorrupt binary mesh data!";
                        logError(msg);
                        return nullptr;
                    }
                    else if(texID != -1)
                    {
                        BasicMaterial::MapType falcorType = getFalcorMapType(TextureType(i));
                        if(BasicMaterial::MapType::Count == falcorType)
                        {
                            logWarning("Texture of Type " + std::to_string(i) + " is not supported by the material system (model " + mModelName + ")");
                            continue;
                        }

                        // Load the texture
                        TexSignature texSig;
                        texSig.format = getFormatFromMapType(loadTexAsSrgb, texData[texID].format, falcorType);
                        texSig.pData = texData[texID].data.data();
                        // Check if we already created a matching texture
                        auto existingTex = textures.find(texSig);
                        if(existingTex != textures.end())
                        {
                            basicMaterial.pTextures[falcorType] = existingTex->second;
                        }
                        else
                        {
                            auto pTexture = Texture::create2D(texData[texID].width, texData[texID].height, texSig.format, 1, Texture::kMaxPossible, texSig.pData);
                            pTexture->setSourceFilename(texData[texID].name);
                            textures[texSig] = pTexture;
                            basicMaterial.pTextures[falcorType] = pTexture;
                        }
                    }
                }

                // Create material and check if it already exists
                auto pMaterial = checkForExistingMaterial(basicMaterial.convertToMaterial());

                int32_t numTriangles;
                mStream >> numTriangles;
                if(numTriangles < 0)
                {
                    std::string Msg = "Error when loading model " + mModelName + ".\nMesh has negative number of triangles!";
                    logError(Msg);
                    return nullptr;
                }

                // create the index buffer
                uint32_t numIndices = numTriangles * 3;
                std::vector<uint32_t> indices(numIndices);
                uint32_t ibSize = 3 * numTriangles * sizeof(uint32_t);
                mStream.read(&indices[0], ibSize);

                auto pIB = Buffer::create(ibSize, Buffer::BindFlags::Index, Buffer::CpuAccess::None, indices.data());

                // Generate tangent space data if needed
                if(genTangentForMesh)
                {
                    if(texCoordBufferIndex != kInvalidBufferIndex)
                    {
                        logError("Model " + mModelName + " asked to generate tangents w/o texture coordinates");
                    }
                    uint32_t texCrdCount = 0;
                    glm::vec2* texCrd = nullptr;
                    if(texCoordBufferIndex != kInvalidBufferIndex)
                    {
                        texCrdCount = pLayout->getBufferLayout(texCoordBufferIndex)->getStride() / sizeof(glm::vec2);
                        texCrd = (glm::vec2*)buffers[texCoordBufferIndex].data();
                    }

                    ResourceFormat posFormat = pLayout->getBufferLayout(positionBufferIndex)->getElementFormat(0);

                    if (posFormat == ResourceFormat::RGB32Float)
                    {
                        generateSubmeshTangentData<glm::vec3>(indices, (glm::vec3*)buffers[positionBufferIndex].data(), (glm::vec3*)buffers[normalBufferIndex].data(), texCrd, texCrdCount, (glm::vec3*)buffers[tangentBufferIndex].data(), (glm::vec3*)buffers[bitangentBufferIndex].data());
                    }
                    else if (posFormat == ResourceFormat::RGBA32Float)
                    {
                        generateSubmeshTangentData<glm::vec4>(indices, (glm::vec4*)buffers[positionBufferIndex].data(), (glm::vec3*)buffers[normalBufferIndex].data(), texCrd, texCrdCount, (glm::vec3*)buffers[tangentBufferIndex].data(), (glm::vec3*)buffers[bitangentBufferIndex].data());
                    }

                    pVBs[tangentBufferIndex] = Buffer::create(buffers[tangentBufferIndex].size(), Buffer::BindFlags::Vertex, Buffer::CpuAccess::None, buffers[tangentBufferIndex].data());

                    pVBs[bitangentBufferIndex] = Buffer::create(buffers[bitangentBufferIndex].size(), Buffer::BindFlags::Vertex, Buffer::CpuAccess::None, buffers[bitangentBufferIndex].data());
                }
                

                // Calculate the bounding-box
                glm::vec3 max, min;
                for(uint32_t i = 0; i < numIndices; i++)
                {
                    uint32_t vertexID = indices[i];
                    uint8_t* pVertex = (pLayout->getBufferLayout(positionBufferIndex)->getStride() * vertexID) + buffers[positionBufferIndex].data();

                    float* pPosition = (float*)pVertex;

                    glm::vec3 xyz(pPosition[0], pPosition[1], pPosition[2]);
                    min = glm::min(min, xyz);
                    max = glm::max(max, xyz);
                }

                BoundingBox box = BoundingBox::fromMinMax(min, max);

                // create the mesh
                auto pMesh = Mesh::create(pVBs, numVertices, pIB, numIndices, pLayout, Vao::Topology::TriangleList, pMaterial, box, false);

                if (version >= 6)
                {
                    falcorMeshCache.push_back(pMesh);
                    meshToSubmeshesID[meshIdx].push_back((uint32_t)(falcorMeshCache.size() - 1));
                }
                else
                {
                    pModel->addMeshInstance(pMesh, glm::mat4());
                }
            }
        }

        if(version >= 6)
        {
            for(int32_t instanceID = 0; instanceID < numInstances; instanceID++)
            {
                int32_t meshIdx = 0;
                int32_t enabled = 1;
                glm::mat4 transformation;

                mStream >> meshIdx >> enabled >> transformation;
                //m_Stream >> inst.name >> inst.metadata;
                readString(mStream);   // Name
                readString(mStream);   // Meta-data

                if(enabled)
                {
                    for(uint32_t i : meshToSubmeshesID[meshIdx])
                    {
                        pModel->addMeshInstance(falcorMeshCache[i], transformation);
                    }
                }
            }
        }
        
        return pModel;
    }
}
