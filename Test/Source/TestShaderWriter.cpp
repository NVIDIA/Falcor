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
#include "TestShaderWriter.h"



//  Return the Vertex Shader Code.
std::string TestShaderWriter::getVertexShaderCode() const
{
    return writeVSShaderCode(mVSData);
}

std::string TestShaderWriter::getHullShaderCode() const
{
    return "";
}

std::string TestShaderWriter::getDomainShaderCode() const
{
    return "";
}

std::string TestShaderWriter::getGeometryShaderCode() const
{
    return "";
}

//  Return the Pixel Shader Code.
std::string TestShaderWriter::getPixelShaderCode() const
{
    return writePSShaderCode(mPSData);
}

//  Return the Vertex Shader Data.
TestShaderWriter::ShaderResourcesData * TestShaderWriter::getVSResourceMetaData()
{
    return &mVSData;
}

//  Return the Hull Shader Data.
TestShaderWriter::ShaderResourcesData * TestShaderWriter::getHSResourceMetaData()
{
    return &mHSData;
}

//  Return the Domain Shader Data.
TestShaderWriter::ShaderResourcesData * TestShaderWriter::getDSResourceMetaData()
{
    return &mDSData;
}

//  Return the Geometry Shader Data.
TestShaderWriter::ShaderResourcesData * TestShaderWriter::getGSResourceMetaData()
{
    return &mGSData;
}

//  Return the Pixel Shader Meta Data.
TestShaderWriter::ShaderResourcesData * TestShaderWriter::getPSResourceMetaData()
{
    return &mPSData;
}

//  Return the Vertex Shader Data.
const TestShaderWriter::ShaderResourcesData & TestShaderWriter::viewVSResourceMetaData() const
{
    return mVSData;
}

//  Return the Hull Shader Data.
const TestShaderWriter::ShaderResourcesData & TestShaderWriter::viewHSResourceMetaData() const
{
    return mHSData;
}

//  Return the Domain Shader Data.
const TestShaderWriter::ShaderResourcesData & TestShaderWriter::viewDSResourceMetaData() const
{
    return mDSData;
}

//  Return the Geometry Shader Data.
const TestShaderWriter::ShaderResourcesData & TestShaderWriter::viewGSResourceMetaData() const
{
    return mGSData;
}

//  Return the Pixel Shader Meta Data.
const TestShaderWriter::ShaderResourcesData & TestShaderWriter::viewPSResourceMetaData() const
{
    return mPSData;
}

//  Return the Vertex Shader Code.
std::string TestShaderWriter::writeVSShaderCode(const ShaderResourcesData & vsMetaData)
{
    //  Vertex Shader.
    std::string vertexShader = "\n";

    //  Get the Vertex Shader Output Shader Code.
    std::string vsOut = writeShaderOutputStruct(vsMetaData, "VS_OUT", "OUTPUT");
    vertexShader = vertexShader + vsOut;
    vertexShader = vertexShader + "\n";

    //  Write the Vertex Shader Resource Code.
    vertexShader = vertexShader + "\n";


    std::string cbResourcesCode = writeDeclareConstantBufferResources(vsMetaData);
    vertexShader = vertexShader + cbResourcesCode;
    vertexShader = vertexShader + "\n";

    std::string sbResourcesCode = writeDeclareStructuredBufferResources(vsMetaData.sbs);
    vertexShader = vertexShader + sbResourcesCode;
    vertexShader = vertexShader + "\n";

    std::string samplersResourcesCode = writeDeclareSamplerResources(vsMetaData.samplers);
    vertexShader = vertexShader + samplersResourcesCode;
    vertexShader = vertexShader + "\n";

    std::string textureResourcesCode = writeDeclareTextureResources(vsMetaData.textures);
    vertexShader = vertexShader + textureResourcesCode;
    vertexShader = vertexShader + "\n";

    std::string rbResourcesCode = writeDeclareRawBufferResources(vsMetaData.rbs);
    vertexShader = vertexShader + rbResourcesCode;
    vertexShader = vertexShader + "\n";

    //  Create the Shader body for the Vertex Shader.
    vertexShader = vertexShader + "VS_OUT main(float4 pos : POSITION) \n";
    vertexShader = vertexShader + "{ \n";
    vertexShader = vertexShader + "\n";

    //  Add in the Vertex Shader Output.
    vertexShader = vertexShader + "VS_OUT vsOut; \n";


    //  Create the Return Lines Shader Code.
    std::string returnLines = writeShaderReturnCode(vsMetaData, "vsOut");
    vertexShader = vertexShader + returnLines;
    vertexShader = vertexShader + "\n";

    //  Add in the Vertex Shader Return.
    vertexShader = vertexShader + "return vsOut; \n";
    vertexShader = vertexShader + "} \n";
    
    //  
    return vertexShader;
}

//  Return the Hull Shader Code.
std::string TestShaderWriter::writeHSShaderCode(const ShaderResourcesData & hsMetaData)
{
    return "";
}

//  Return the Domain Shader Code.
std::string TestShaderWriter::writeDSShaderCode(const ShaderResourcesData & dsMetaData)
{
    return "";
}

//  Return the Geometry Shader Code.
std::string TestShaderWriter::writeGSShaderCode(const ShaderResourcesData & gsMetaData)
{
    return "";
}

//  Return the Pixel Shader Code.
std::string TestShaderWriter::writePSShaderCode(const ShaderResourcesData & psMetaData)
{ 
    //  Pixel Shader.
    std::string pixelShader = "\n";

    //  Get the Vertex Shader Output Shader Code.
    std::string psIn = writeShaderInputStruct(psMetaData, "VS_OUT", "OUTPUT");

    //  
    pixelShader = pixelShader + psIn;

    //  Create the Output Struct Shader Code.
    std::string psOut = writeShaderOutputStruct(psMetaData, "PS_OUT", "SV_TARGET");
    pixelShader = pixelShader + psOut;
    pixelShader = pixelShader + "\n";


    std::string cbResourcesCode = writeDeclareConstantBufferResources(psMetaData);
    pixelShader = pixelShader + cbResourcesCode;
    pixelShader = pixelShader + "\n";

    std::string sbResourcesCode = writeDeclareStructuredBufferResources(psMetaData.sbs);
    pixelShader = pixelShader + sbResourcesCode;
    pixelShader = pixelShader + "\n";

    std::string samplersResourcesCode = writeDeclareSamplerResources(psMetaData.samplers);
    pixelShader = pixelShader + samplersResourcesCode;
    pixelShader = pixelShader + "\n";

    std::string textureResourcesCode = writeDeclareTextureResources(psMetaData.textures);
    pixelShader = pixelShader + textureResourcesCode;
    pixelShader = pixelShader + "\n";

    std::string rbResourcesCode = writeDeclareRawBufferResources(psMetaData.rbs);
    pixelShader = pixelShader + rbResourcesCode;
    pixelShader = pixelShader + "\n";


    //
    //  Create the Shader body for the Pixel Shader.
    pixelShader = pixelShader + "PS_OUT main(VS_OUT vOut) \n";
    pixelShader = pixelShader + "{ \n";
    pixelShader = pixelShader + "\n";

    //  Add in the Pixel Shader Output.
    pixelShader = pixelShader + "PS_OUT psOut; \n";

    //  Create the Return Lines Shader Code.
    std::string returnLines = writeShaderReturnCode(psMetaData, "psOut");
    pixelShader = pixelShader + returnLines;
    pixelShader = pixelShader + "\n";

    //  Add in the Pixel Shader Return.
    pixelShader = pixelShader + "return psOut; \n";
    pixelShader = pixelShader + "} \n";

    //  Return the Pixel Shader.
    return pixelShader;

}


//  Return the Compute Shader Code.
std::string TestShaderWriter::writeCSShaderCode(const ShaderResourcesData & csMetaData)
{
    return "";
}

//  Write the Shader Input Struct.
std::string TestShaderWriter::writeShaderInputStruct(const ShaderResourcesData & rsMetaData, const std::string & inputStructName, const std::string & semanticPrefix)
{

    //  Generate the Shader Code from the output set for the output.
    std::string inputStruct = "";
    inputStruct = inputStruct + "\n";

    //  Create the Output Struct.
    inputStruct = inputStruct + "struct " + inputStructName + " \n";
    inputStruct = inputStruct + "{ \n";

    //  Add in the active Render Targets.
    for (const auto & currentOutput : rsMetaData.inTargets)
    {
        inputStruct = inputStruct + "float4 output" + std::to_string(currentOutput) + " : " + semanticPrefix + std::to_string(currentOutput) + "; \n";
    }

    //  
    inputStruct = inputStruct + "}; \n";
    inputStruct = inputStruct + "\n";

    //  Return the Output Struct.
    return inputStruct;

}

//  Write the Shader Output Struct.
std::string TestShaderWriter::writeShaderOutputStruct(const ShaderResourcesData & rsMetaData, const std::string & outputStructName, const std::string & semanticPrefix)
{
    //  Generate the Shader Code from the output set for the output.
    std::string outputStruct = "";
    outputStruct = outputStruct + "\n";

    //  Create the Output Struct.
    outputStruct = outputStruct + "struct " + outputStructName + " \n";
    outputStruct = outputStruct + "{ \n";

    //  Add in the active Render Targets.
    for (const auto & currentOutput : rsMetaData.outTargets)
    {
        outputStruct = outputStruct + "float4 output" + std::to_string(currentOutput) + " : " + semanticPrefix + std::to_string(currentOutput) + "; \n";
    }

    //  
    outputStruct = outputStruct + "}; \n";
    outputStruct = outputStruct + "\n";

    //  Return the Output Struct.
    return outputStruct;
}




//  Return the Vertex Shader Code for the Output Lines.
std::string TestShaderWriter::writeShaderReturnCode(const ShaderResourcesData & rsMetaData, const std::string & outputStruct)
{
    std::string outputLines = "";
    outputLines = "\n";

    //  Link the used resources to the outputs.
    for (const auto & currentOutput : rsMetaData.outTargets)
    {
        outputLines = outputLines + outputStruct + "." + "output" + std::to_string(currentOutput) + " = ";

        outputLines = outputLines + writeUseConstantBufferResources(rsMetaData);

        for (const auto currentResource : rsMetaData.textures)
        {
            outputLines = outputLines + writeUseTextureResource(currentResource);
        }

        for (const auto currentResource : rsMetaData.sbs)
        {
            outputLines = outputLines + writeUseStructuredBufferResource(currentResource);
        }

        for (const auto currentResource : rsMetaData.rbs)
        {
            outputLines = outputLines + writeUseRawBufferResource(currentResource);
        }

        outputLines = outputLines + "float4(0.05, 0.05, 0.05, 0.05);";

        outputLines = outputLines + "\n";
    }

    outputLines = outputLines + "\n";

    //  Return the Output Lines.
    return outputLines;

};



//  Return the Requested Type - Can only convert down.
std::string TestShaderWriter::convertToType(const std::string & variable, const ProgramReflection::Variable::Type & startType, const ProgramReflection::Variable::Type & endType)
{
    std::string variableConversion = "";

    if (startType == endType)
    {
        return variable;
    }

    //  Convert to Float2 from Floats.
    if (startType == ProgramReflection::Variable::Type::Float && endType == ProgramReflection::Variable::Type::Float2)
    {
        return "float2( " + variable + " , " + variable + " )";
    }
    
    //  
    if (startType == ProgramReflection::Variable::Type::Float3 && endType == ProgramReflection::Variable::Type::Float2)
    {
        return "float2( " + variable + ".x , " + variable + ".y )";
    }

    //  
    if (startType == ProgramReflection::Variable::Type::Float4 && endType == ProgramReflection::Variable::Type::Float2)
    {
        return "float2( " + variable + ".x , " + variable + ".y )";
    }


    //  Convert to Float3 from Floats.
    if (startType == ProgramReflection::Variable::Type::Float && endType == ProgramReflection::Variable::Type::Float3)
    {
        return "float3( " + variable + " , " + variable + ", " + variable + ")";
    }

    if (startType == ProgramReflection::Variable::Type::Float2 && endType == ProgramReflection::Variable::Type::Float3)
    {
        return "float3( " + variable + ".x , " + variable + ".y , 0)";
    }

    if (startType == ProgramReflection::Variable::Type::Float4 && endType == ProgramReflection::Variable::Type::Float3)
    {
        return "float3( " + variable + ".x , " + variable + ".y, " + variable + ".z)";
    }

    
    //  Convert to Float4 from Floats.
    if (startType == ProgramReflection::Variable::Type::Float && endType == ProgramReflection::Variable::Type::Float4)
    {
        return "float4( " + variable + " , " + variable + ", " + variable + " , " + variable + ")";
    }

    if (startType == ProgramReflection::Variable::Type::Float2 && endType == ProgramReflection::Variable::Type::Float4)
    {
        return "float4( " + variable + ".x , " + variable + ".y , 0, 0)";
    }

    if (startType == ProgramReflection::Variable::Type::Float3 && endType == ProgramReflection::Variable::Type::Float4)
    {
        return "float4( " + variable + ".x , " + variable + ".y, " + variable + ".z, 0)";
    }

    return variableConversion;
}


//
std::string TestShaderWriter::writeDeclareConstantBufferResources(const ShaderResourcesData & shaderResourcesData)
{

    //  Create the Constant Buffer Code.
    std::string cbsCode = "\n";
    cbsCode = cbsCode + "\n";
    
    //
    for (const auto & currentCB : shaderResourcesData.cbs)
    {
        std::string currentCBCode = "";

        std::string resourcesCode = "";

        std::string sbResources = "";
        for (const auto & currentSB : shaderResourcesData.sbs)
        {
            if (currentSB.isWithinConstantBuffer && currentSB.cbVariable == currentCB.variable)
            {
                sbResources = sbResources + writeDeclaredStructuredBufferResource(currentSB);
            }
        }
        resourcesCode = resourcesCode + sbResources;

        std::string samplerResources = "";
        for (const auto currentSampler : shaderResourcesData.samplers)
        {
            if (currentSampler.isWithinConstantBuffer && currentSampler.cbVariable == currentCB.variable)
            {
                samplerResources = samplerResources + writeDeclareSamplerResource(currentSampler);
            }
        }
        resourcesCode = resourcesCode + samplerResources;


        std::string texturesResources = "";
        for (const auto & currentTexture : shaderResourcesData.textures)
        {
            if (currentTexture.isWithinConstantBuffer && currentTexture.cbVariable == currentCB.variable)
            {
                texturesResources = texturesResources + writeDeclareTextureResource(currentTexture);
            }
        }
        resourcesCode = resourcesCode + texturesResources;


        std::string rbResources = "";
        for (const auto currentRawBuffer : shaderResourcesData.rbs)
        {
            if (currentRawBuffer.isWithinConstantBuffer && currentRawBuffer.cbVariable == currentCB.variable)
            {
                rbResources = rbResources + writeDeclareRawBufferResource(currentRawBuffer);
            }
        }
        resourcesCode = resourcesCode + rbResources;


        std::string variablesCode = "";

        //  Check whether we have any variables, and write the variables if we do.
        for (const auto & currentStructVariable : currentCB.structVariables)
        {
            std::string currentVariableCode = "";

            std::string variableType = to_string(currentStructVariable.variableType);
            std::transform(variableType.begin(), variableType.end(), variableType.begin(), ::tolower);

            currentVariableCode = currentVariableCode + variableType + " " + currentStructVariable.structVariable + ";";

            if (currentVariableCode != "")
            {
                variablesCode = variablesCode + currentVariableCode + "\n";
            }

        }

        std::string indexSpaceSuffix = "";

        //  Check if we need to add in the explicit index or space.
        if (currentCB.isExplicitIndex || currentCB.isExplicitSpace)
        {
            //  Add in the common register code.
            indexSpaceSuffix = indexSpaceSuffix + " : ";
            indexSpaceSuffix = indexSpaceSuffix + "register ( ";

            //  Check to see if we need the explicit index.
            if (currentCB.isExplicitIndex)
            {
                indexSpaceSuffix = indexSpaceSuffix + "b" + std::to_string(currentCB.regIndex);
            }

            //  Check if need the comma separation.
            if (currentCB.isExplicitIndex && currentCB.isExplicitSpace)
            {
                indexSpaceSuffix = indexSpaceSuffix + " , ";
            }

            //  Check if we need to add in the explicit index or space.
            if (currentCB.isExplicitSpace)
            {
                indexSpaceSuffix = indexSpaceSuffix + "space = " + std::to_string(currentCB.regSpace);
            }

            //  Add in the closing brackets.
            indexSpaceSuffix = indexSpaceSuffix + " )";
        }


        if (currentCB.usesExternalStruct)
        {
            //  
            std::string structCode = "\n";
            structCode = structCode + "struct " + currentCB.externalStructVariable + "\n";
            structCode = structCode + "{ \n";

            structCode = structCode + variablesCode + "\n";
            structCode = structCode + resourcesCode + "\n";

            structCode = structCode + "}; \n";
            structCode = structCode + "\n";

            currentCBCode = currentCBCode + structCode + "\n";

            currentCBCode = currentCBCode + "ConstantBuffer<" + currentCB.externalStructVariable + "> " + currentCB.variable;


            //  Add in the Array syntax as needed.
            if (currentCB.isArray && currentCB.arraySize > 0)
            {
                currentCBCode = currentCBCode + "[";

                //  
                if (!currentCB.isUnbounded)
                {
                    currentCBCode = currentCBCode + std::to_string(currentCB.arraySize);
                }

                currentCBCode = currentCBCode + "]";
            }

            currentCBCode = currentCBCode + indexSpaceSuffix;
        }
        else
        {
            std::string cbufferCode = "\n";
            cbufferCode = cbufferCode + "cbuffer " + currentCB.variable;

            cbufferCode = cbufferCode + " " + indexSpaceSuffix + "\n";
            cbufferCode = cbufferCode + "{ \n";

            cbufferCode = cbufferCode + variablesCode + "\n";
            cbufferCode = cbufferCode + resourcesCode + "\n";

            cbufferCode = cbufferCode + "}; \n";

            currentCBCode = cbufferCode + currentCBCode;
        }
    
        if (currentCBCode != "")
        {
            cbsCode = cbsCode + currentCBCode;
        }

   }

   return cbsCode;    
}

//
std::string TestShaderWriter::writeUseConstantBufferResources(const ShaderResourcesData & shaderResourcesData)
{
    std::string useConstantBuffersCode = "";

    for (const auto currentCB : shaderResourcesData.cbs)
    {
        if (currentCB.isUsedResource)
        {
            //  Get the Use Constant Buffer Code for all of the Variables.
            for (const auto & currentVariable : currentCB.structVariables)
            {
                if (currentVariable.isUsed)
                {
                    std::string currentVariableCode = currentVariable.structVariable;
                    
                    //
                    if (currentCB.isArray && currentCB.arraySize > 0)
                    {
                        currentVariableCode = currentCB.variable + "[0]" + "." + currentVariableCode;
                    }

                    currentVariableCode = currentVariableCode + " + ";

                    //  
                    if (currentVariableCode != "")
                    {
                        useConstantBuffersCode = useConstantBuffersCode + currentVariableCode;
                    }
                }
            }
        }
    }

    return useConstantBuffersCode;
}

//
std::string TestShaderWriter::writeDeclareTextureBufferResources(const ShaderResourcesData & shaderResourcesData)
{

    return "";
}

//  
std::string TestShaderWriter::writeDeclareStructuredBufferResources(const std::vector<StructuredBufferResource> & givenStructuredBufferResources)
{
    //
    std::string structuredBuffersCode = "";

    //  Iterate over the Structured Buffer Resources.
    for (uint32_t i = 0; i < givenStructuredBufferResources.size(); i++)
    {
        //  
        std::string currentSBResource = writeDeclaredStructuredBufferResource(givenStructuredBufferResources[i]);

        //
        structuredBuffersCode = structuredBuffersCode + currentSBResource;
    }

    //  
    return structuredBuffersCode;

}

//
std::string TestShaderWriter::writeDeclaredStructuredBufferResource(const StructuredBufferResource & givenStructuredBufferResource)
{
    std::string currentStructuredBufferCode = "";

    //  Check if the Structured Buffer Resources uses an external struct.
    if (givenStructuredBufferResource.usesExternalStructVariables)
    {
        //  
        std::string structCode = "\n";
        structCode = structCode + "struct " + givenStructuredBufferResource.structVariable;
        structCode = structCode + "{ \n";

        //  Check whether we have any variables, and write the variables if we do.
        for (const auto & currentStructVariable : givenStructuredBufferResource.externalStructVariables)
        {
            std::string structVariableCode = "";

            std::string variableType = to_string(currentStructVariable.variableType);
            std::transform(variableType.begin(), variableType.end(), variableType.begin(), ::tolower);

            structVariableCode = structVariableCode + variableType + " " + currentStructVariable.structVariable + ";";

            if (structVariableCode != "")
            {
                structCode = structCode + structVariableCode + "\n";
            }

        }

        //  
        structCode = structCode + "}; \n";
        structCode = structCode + "\n";

        currentStructuredBufferCode = currentStructuredBufferCode + structCode;
    }


    //  
    std::string sbDeclareType = to_string(ProgramReflection::Resource::ResourceType::StructuredBuffer);

    //  
    if (givenStructuredBufferResource.usesExternalStructVariables)
    {
        sbDeclareType = sbDeclareType + "<" + givenStructuredBufferResource.structVariable + ">";

    }
    else
    {
        std::string variableType = to_string(givenStructuredBufferResource.sbType);
        std::transform(variableType.begin(), variableType.end(), variableType.begin(), ::tolower);
        sbDeclareType = sbDeclareType + "<" + variableType + ">";
    }

    //
    if (givenStructuredBufferResource.shaderAccess == ProgramReflection::ShaderAccess::ReadWrite)
    {
        sbDeclareType = "RW" + sbDeclareType;
    }

    //  Write the Structured Buffer Declaration.
    std::string completeStructuredBufferDeclaration = writeCombinedResourceCode(givenStructuredBufferResource, sbDeclareType, "t", givenStructuredBufferResource.shaderAccess, givenStructuredBufferResource.isArray, givenStructuredBufferResource.isUnbounded, givenStructuredBufferResource.arrayDimensions);


    //  
    if (completeStructuredBufferDeclaration != "")
    {
        currentStructuredBufferCode = completeStructuredBufferDeclaration + "\n\n";
    }

    //  
    return currentStructuredBufferCode;
}


//  
std::string TestShaderWriter::writeUseStructuredBufferResource(const StructuredBufferResource & givenStructuredBufferResource)
{

    //  
    std::string useSBResource = "";

    //  
    if (givenStructuredBufferResource.isUsedResource)
    {
        //  
        useSBResource = givenStructuredBufferResource.variable;


        //  Write the Array Access.
        if (givenStructuredBufferResource.isArray)
        {
            useSBResource = useSBResource + "[0]";
        }
        else
        {
            useSBResource = useSBResource + "[0]";
        }

        //  Whether the Structured Variables. 
        if (givenStructuredBufferResource.usesExternalStructVariables)
        {
            std::string structVariablesCode = "";

            //  
            for (const auto & currentStructVariable : givenStructuredBufferResource.externalStructVariables)
            {
                if (currentStructVariable.isUsed)
                {
                    structVariablesCode = structVariablesCode + useSBResource + "." + currentStructVariable.structVariable + " + ";
                }
            }

            if (structVariablesCode != "")
            {
                useSBResource = structVariablesCode;
            }
        }
        else
        {
            useSBResource = useSBResource + " + ";
        }
    }

    //  Return Use Structured Buffer Resource.
    return useSBResource;
}


//  
std::string TestShaderWriter::writeDeclareSamplerResources(const std::vector<SamplerResource> & givenSamplers)
{
    //  
    std::string samplersCode = "";

    //
    for (uint32_t i = 0; i < givenSamplers.size(); i++)
    {
        //  
        std::string currentSamplerCode = writeDeclareSamplerResource(givenSamplers[i]);

        //  
        if (currentSamplerCode != "")
        {
            samplersCode = currentSamplerCode + "\n\n";
        }
    }

    //  Return Samplers Code.
    return samplersCode;
}


std::string TestShaderWriter::writeDeclareSamplerResource(const SamplerResource & givenSampler)
{
    return writeCombinedResourceCode(givenSampler, "SamplerState", "s", ProgramReflection::ShaderAccess::Read, false, false, std::vector<uint32_t>());
}

//  
std::string TestShaderWriter::writeDeclareTextureResources(const std::vector<TextureResource> & givenTextures)
{

    std::string texturesCode = "";

    for (uint32_t i = 0; i < givenTextures.size(); i++)
    {
        //
        std::string currentTexturesCode = writeDeclareTextureResource(givenTextures[i]);
        //
        if (currentTexturesCode != "")
        {
            texturesCode = texturesCode + currentTexturesCode + "\n\n";
        }

    }

    return texturesCode;
}

std::string TestShaderWriter::writeDeclareTextureResource(const TextureResource & givenTexture)
{

    //
    std::string variableType = to_string(givenTexture.textureType);
    std::transform(variableType.begin(), variableType.end(), variableType.begin(), ::tolower);

    //  
    std::string textureDeclareType = to_string(givenTexture.textureResourceType) + "<" + variableType + ">";

    //
    if (givenTexture.shaderAccess == ProgramReflection::ShaderAccess::ReadWrite)
    {
        textureDeclareType = "RW" + textureDeclareType;
    }

    //  
    std::string currentTexturesCode = writeCombinedResourceCode(givenTexture, textureDeclareType, "t", givenTexture.shaderAccess, givenTexture.isArray, givenTexture.isUnbounded, givenTexture.arrayDimensions);

    //
    return currentTexturesCode;

}

//  
std::string TestShaderWriter::writeUseTextureResource(const TextureResource & givenTexture)
{
    std::string useTextureResource = "";
    
    //  
    useTextureResource = givenTexture.variable;

    if (givenTexture.isArray && givenTexture.arrayDimensions.size() > 0)
    {
        useTextureResource = useTextureResource + "[0]";

    }

    if (givenTexture.isSampled && givenTexture.samplerVariable != "")
    {
        useTextureResource = useTextureResource + writeSamplerTextureAccessCode(givenTexture);
    }
    else
    {
        useTextureResource = useTextureResource + writeNonSampleTextureAccessCode(givenTexture);
    }

    //  
    useTextureResource = useTextureResource + " + ";

    return useTextureResource;
}

//  
std::string TestShaderWriter::writeNonSampleTextureAccessCode(const TextureResource & textureResource)
{
    std::string texAccess = "";

    //  Write the Access Code Based on the Current Texture Resource Dimensions.
    if (textureResource.textureResourceType == ProgramReflection::Resource::Dimensions::Buffer)
    {
        texAccess = texAccess + ".Load(0)";
    }
    else if (textureResource.textureResourceType == ProgramReflection::Resource::Dimensions::Texture1D || textureResource.textureResourceType == ProgramReflection::Resource::Dimensions::Texture2DMS)
    {
        texAccess = texAccess + ".Load(int2(0, 0))";
    }
    else if (textureResource.textureResourceType == ProgramReflection::Resource::Dimensions::Texture1DArray || textureResource.textureResourceType == ProgramReflection::Resource::Dimensions::Texture2D || textureResource.textureResourceType == ProgramReflection::Resource::Dimensions::Texture2DMSArray)
    {
        texAccess = texAccess + ".Load(int3(0, 0, 0))";
    }
    else if (textureResource.textureResourceType == ProgramReflection::Resource::Dimensions::Texture2DArray || textureResource.textureResourceType == ProgramReflection::Resource::Dimensions::Texture3D)
    {
        texAccess = texAccess + ".Load(int4(0, 0, 0, 0))";
    }

    return texAccess;

}

//  Return the Code for the Texture Sampler Access.
std::string TestShaderWriter::writeSamplerTextureAccessCode(const TextureResource & textureResource)
{
    std::string texAccess = "";

    //  Write the Access Code Based on the Current Texture Resource Dimensions.
    if (textureResource.textureResourceType == ProgramReflection::Resource::Dimensions::Buffer)
    {
        texAccess = texAccess + ".Sample(" + textureResource.samplerVariable + ", 0)";
    }
    else if (textureResource.textureResourceType == ProgramReflection::Resource::Dimensions::Texture1DArray || textureResource.textureResourceType == ProgramReflection::Resource::Dimensions::Texture2D)
    {
        texAccess = texAccess + ".Sample(" + textureResource.samplerVariable + ", float2(0,0))";
    }
    else if (textureResource.textureResourceType == ProgramReflection::Resource::Dimensions::Texture2DArray || textureResource.textureResourceType == ProgramReflection::Resource::Dimensions::Texture3D || textureResource.textureResourceType == ProgramReflection::Resource::Dimensions::TextureCube)
    {
        texAccess = texAccess + ".Sample(" + textureResource.samplerVariable + ", float3(0, 0, 0))";
    }
    else if (textureResource.textureResourceType == ProgramReflection::Resource::Dimensions::TextureCubeArray)
    {
        texAccess = texAccess + ".Sample(" + textureResource.samplerVariable + ", float4(0, 0, 0, 0))";
    }

    return texAccess;
}



//  
std::string TestShaderWriter::writeDeclareRawBufferResources(const std::vector<RawBufferResource> & givenRawBuffers)
{

    std::string rawBuffersCode = "";

    for (uint32_t i = 0; i < givenRawBuffers.size(); i++)
    {
        //  
        std::string currentRawBufferCode = writeDeclareRawBufferResource(givenRawBuffers[i]);

        if (currentRawBufferCode != "")
        {
            rawBuffersCode = currentRawBufferCode + "\n\n";
        }
    }
    
    return rawBuffersCode;
}


std::string TestShaderWriter::writeDeclareRawBufferResource(const RawBufferResource & givenRawBuffers)
{
    return "";
}

//  
std::string TestShaderWriter::writeUseRawBufferResource(const RawBufferResource & givenRawBuffers)
{
    std::string rawBufferUseCode = "";

    if (givenRawBuffers.isUsedResource)
    {
        rawBufferUseCode = givenRawBuffers.variable + ".Load4(0) + ";
    }


    return rawBufferUseCode;
}


//  Get the Combined Resource Code.
std::string TestShaderWriter::writeCombinedResourceCode(const ShaderResource & shaderResource, std::string declareType, std::string defaultRegisterType, ProgramReflection::ShaderAccess resourceAccess, bool isArray, bool isUnbounded, std::vector<uint32_t> arrayDimensions)
{
    //  
    std::string combinedDeclaration = "";

    combinedDeclaration = declareType + " " + shaderResource.variable;

    if (isArray)
    {
        if (isUnbounded)
        {
            combinedDeclaration = combinedDeclaration + "[]";
        }
        else
        {
            combinedDeclaration = combinedDeclaration + "[" + std::to_string(arrayDimensions[0]) + "]";
        }
    }


    if (shaderResource.isExplicitIndex || shaderResource.isExplicitSpace)
    {
        //  Add in the common register code.
        combinedDeclaration = combinedDeclaration + " : ";
        combinedDeclaration = combinedDeclaration + "register ( ";

        std::string registerType = defaultRegisterType;

        if (resourceAccess == ProgramReflection::ShaderAccess::ReadWrite)
        {
            registerType = "u";
        }

        //  Check to see if we need the explicit index.
        if (shaderResource.isExplicitIndex)
        {
            combinedDeclaration = combinedDeclaration + registerType + std::to_string(shaderResource.regIndex);
        }

        //  Check if need the comma separation.
        if (shaderResource.isExplicitIndex && shaderResource.isExplicitSpace)
        {
            combinedDeclaration = combinedDeclaration + " , ";
        }

        //  Check if we need to add in the explicit index or space.
        if (shaderResource.isExplicitSpace)
        {
            combinedDeclaration = combinedDeclaration + "space = " + std::to_string(shaderResource.regSpace);
        }

        //  Add in the closing brackets.
        combinedDeclaration = combinedDeclaration + " )";
    }


    combinedDeclaration = combinedDeclaration + ";";

    //  Return the Combined Declaration.
    return combinedDeclaration;
}

