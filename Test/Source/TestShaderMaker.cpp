#include "TestShaderMaker.h"


using namespace Falcor;
using namespace rapidjson;

//  Load the Configuration File.
bool TestShaderMaker::loadConfigFile(const std::string & configFile, std::string errorMessage)
{
    //  The path to the configuration file.
    std::string fullpath;

    //  Find File in Data Directories.
    if (findFileInDataDirectories(configFile, fullpath))
    {
        // Load the file
        std::ifstream fileStream(fullpath);
        std::stringstream strStream;
        strStream << fileStream.rdbuf();
        std::string jsonData = strStream.str();
        rapidjson::StringStream JStream(jsonData.c_str());

        // Get the file directory
        auto last = fullpath.find_last_of("/\\");
        std::string mDirectory = fullpath.substr(0, last);
        mJDoc.ParseStream(JStream);

        //  Check if there is a parse error.
        if (mJDoc.HasParseError())
        {
            size_t line;
            line = std::count(jsonData.begin(), jsonData.begin() + mJDoc.GetErrorOffset(), '\n');
            errorMessage = (std::string("JSON Parse error in line ") + std::to_string(line) + ". " + rapidjson::GetParseError_En(mJDoc.GetParseError()));
            return false;
        }
    }
    else
    {
        errorMessage = (std::string("File Not Found!"));
        return false;
    }

    //  Version.
    Value::MemberIterator version = mJDoc.FindMember("version");
    if (version != mJDoc.MemberEnd())
    {
        mShaderTestLayout.version = version->value.GetString();
    }
    else
    {
        errorMessage = "Version Not Found!";
        return false;
    }


    //  Stages.
    Value::MemberIterator stages = mJDoc.FindMember("stages");
    if (stages != mJDoc.MemberEnd())
    {
        std::string stagesString = stages->value.GetString();
        if (stagesString == "vs/ps")
        {
            mShaderTestLayout.hasVertexShader = true;
            mShaderTestLayout.stages.push_back("vs");
            mShaderTestLayout.hasPixelShader = true;
            mShaderTestLayout.stages.push_back("ps");
        }

        else if (stages->value.GetString() == "cs")
        {
            mShaderTestLayout.hasComputeShader = true;
            mShaderTestLayout.stages.push_back("cs");
        }
        else
        {
            errorMessage = "Stages Not Valid!";
            return false;
        }
    }
    else
    {
        errorMessage = "Stages Not Found!";
        return false;
    }


    //  Random Seed.
    Value::MemberIterator randomseed = mJDoc.FindMember("random seed");
    if (randomseed != mJDoc.MemberEnd())
    {
        mShaderTestLayout.randomseed = randomseed->value.GetUint64();
    }
    else
    {
        errorMessage = "Random Seed Not Found!";
        return false;
    }




    //  Get the count of each resource.

    //  Constant Buffers.
    Value::MemberIterator cbs = mJDoc.FindMember("constant buffers");
    if (cbs != mJDoc.MemberEnd())
    {
        if (cbs->value.IsUint())
        {
            mShaderTestLayout.constantbuffers = cbs->value.GetUint();
        }
        else
        {
            mShaderTestLayout.constantbuffers = 0;
        }
    }
    else
    {
        mShaderTestLayout.constantbuffers = 0;
    }

    //  Texture Buffers.
    Value::MemberIterator tbs = mJDoc.FindMember("texture buffers");
    if (tbs != mJDoc.MemberEnd())
    {
        if (tbs->value.IsUint())
        {
            mShaderTestLayout.texturebuffers = tbs->value.GetUint();
        }
        else
        {
            mShaderTestLayout.texturebuffers = 0;
        }
    }
    else
    {
        mShaderTestLayout.texturebuffers = 0;
    }


    //  Structured Buffers.
    Value::MemberIterator sbs = mJDoc.FindMember("structured buffers");
    if (sbs != mJDoc.MemberEnd())
    {
        if (sbs->value.IsUint())
        {
            mShaderTestLayout.structuredbuffers = sbs->value.GetUint();
        }
        else
        {
            mShaderTestLayout.structuredbuffers = 0;
        }
    }
    else
    {
        mShaderTestLayout.structuredbuffers = 0;
    }

    //  Read/Write Structured Buffers.
    Value::MemberIterator rwSBs = mJDoc.FindMember("rw structured buffers");
    if (rwSBs != mJDoc.MemberEnd())
    {
        if (rwSBs->value.IsUint())
        {
            mShaderTestLayout.rwStructuredBuffers = rwSBs->value.GetUint();
        }
        else
        {
            mShaderTestLayout.rwStructuredBuffers = 0;
        }
    }
    else
    {
        mShaderTestLayout.rwStructuredBuffers = 0;
    }


    //  Raw Buffers.
    Value::MemberIterator rbs = mJDoc.FindMember("raw buffers");
    if (rbs != mJDoc.MemberEnd())
    {
        if (rbs->value.IsUint())
        {
            mShaderTestLayout.rawbuffers = rbs->value.GetUint();
        }
        else
        {
            mShaderTestLayout.rawbuffers = 0;
        }
    }
    else
    {
        mShaderTestLayout.rawbuffers = 0;
    }

    //  Raw Buffers.
    Value::MemberIterator rwRBs = mJDoc.FindMember("rw raw buffers");
    if (rwRBs != mJDoc.MemberEnd())
    {
        if (rwRBs->value.IsUint())
        {
            mShaderTestLayout.rwRawBuffers = rwRBs->value.GetUint();
        }
        else
        {
            mShaderTestLayout.rwRawBuffers = 0;
        }
    }
    else
    {
        mShaderTestLayout.rwRawBuffers = 0;
    }

    //  Textures.
    Value::MemberIterator texs = mJDoc.FindMember("textures");
    if (texs != mJDoc.MemberEnd())
    {
        if (texs->value.IsUint())
        {
            mShaderTestLayout.textures = texs->value.GetUint();
        }
        else
        {
            mShaderTestLayout.textures = 0;
        }
    }
    else
    {
        mShaderTestLayout.textures = 0;
    }

    //  RW Textures.
    Value::MemberIterator rwTexs = mJDoc.FindMember("rw textures");
    if (rwTexs != mJDoc.MemberEnd())
    {
        if (rwTexs->value.IsUint())
        {
            mShaderTestLayout.rwTextures = rwTexs->value.GetUint();
        }
        else
        {
            mShaderTestLayout.rwTextures = 0;
        }
    }
    else
    {
        mShaderTestLayout.rwTextures = 0;
    }



    //  Samplers.
    Value::MemberIterator samps = mJDoc.FindMember("samplers");
    if (samps != mJDoc.MemberEnd())
    {
        if (samps->value.IsUint())
        {
            mShaderTestLayout.samplers = samps->value.GetUint();
        }
        else
        {
            mShaderTestLayout.samplers = 0;
        }

    }
    else
    {
        mShaderTestLayout.samplers = 0;
    }



    //  Mode.
    Value::MemberIterator mode = mJDoc.FindMember("mode");
    if (mode != mJDoc.MemberEnd())
    {
        mShaderTestLayout.mode = mode->value.GetString();
    }
    else
    {
        mShaderTestLayout.mode = "both";
    }

    return true;
}

//  Set the Version Restricted Content.
void TestShaderMaker::setVersionRestrictedConstants()
{
    //  Maximum Struct Variables.
    mShaderTestLayout.maxStructVariablesCount = 3;

    //  
    if (mShaderTestLayout.version == "DX5.0")
    {
        mShaderTestLayout.maxUAVTargets = 8;
        mShaderTestLayout.maxPixelShaderOutput = 8;
    }



}

//  
const TestShaderMaker::TestShaderLayout & TestShaderMaker::viewShaderTestLayout() const
{
    return mShaderTestLayout;
}

TestShaderMaker::TestShaderLayout & TestShaderMaker::getShaderTestLayout()
{
    return mShaderTestLayout;
}

//  Return the Vertex Shader Code.
std::string TestShaderMaker::getVertexShaderCode() const
{
    return mShaderMaker.getVertexShaderCode();
}

//  Return the Hull Shader Code.
std::string TestShaderMaker::getHullShaderCode() const
{
    return mShaderMaker.getHullShaderCode();
}


//  Return the Domain Shader Code.
std::string TestShaderMaker::getDomainShaderCode() const
{
    return mShaderMaker.getDomainShaderCode();
}


//  Return the Geometry Shader Code.
std::string TestShaderMaker::getGeometryShaderCode() const
{
    return mShaderMaker.getGeometryShaderCode();
}

//  Return the Pixel Shader Code.
std::string TestShaderMaker::getPixelShaderCode() const
{
    return mShaderMaker.getPixelShaderCode();
}

//  Return the Compute Shader Code.
std::string TestShaderMaker::getComputeShaderCode() const
{
    return mShaderMaker.getComputeShaderCode();
}

//  Verify the Program Resources.
bool TestShaderMaker::verifyProgramResources(const TestShaderWriter::ShaderResourcesData & rsData, const GraphicsVars::SharedPtr & graphicsProgramVars)
{
    bool verifiedResources = verifyAllResources(rsData, graphicsProgramVars);

    return verifiedResources;
}

//  
bool TestShaderMaker::verifyProgramResources(const GraphicsVars::SharedPtr & graphicsProgramVars)
{
    //  Verify the Stages.
    bool vsVerify = true;
    if (mShaderTestLayout.hasVertexShader)
    {
        vsVerify = verifyProgramResources(mShaderMaker.viewVSResourceData(), graphicsProgramVars);
    }

    bool hsVerify = true;
    if (mShaderTestLayout.hasHullShader)
    {
        hsVerify = verifyProgramResources(mShaderMaker.viewHSResourceData(), graphicsProgramVars);
    }


    bool dsVerify = true;
    if (mShaderTestLayout.hasDomainShader)
    {
        dsVerify = verifyProgramResources(mShaderMaker.viewDSResourceData(), graphicsProgramVars);
    }


    bool gsVerify = true;
    if (mShaderTestLayout.hasGeometryShader)
    {
        gsVerify = verifyProgramResources(mShaderMaker.viewGSResourceData(), graphicsProgramVars);
    }
 
    
    bool psVerify = true;
    if (mShaderTestLayout.hasPixelShader)
    {
        psVerify = verifyProgramResources(mShaderMaker.viewPSResourceData(), graphicsProgramVars);
    }

    bool csVerify = true;
    if (mShaderTestLayout.hasComputeShader)
    {
        csVerify = verifyProgramResources(mShaderMaker.viewCSResourceData(), graphicsProgramVars);
    }

    //  
    return vsVerify && hsVerify && dsVerify && gsVerify && psVerify && csVerify;
}

//  Generate the Shader Program.
void TestShaderMaker::generateShaderProgram()
{
    //  Set up some generation constants.
    setVersionRestrictedConstants();


    for (uint32_t i = 0; i < mShaderTestLayout.stages.size(); i++)
    {
        TestShaderWriter::ShaderResourcesData * cstage = getShaderResourceData(mShaderTestLayout.stages[i]);

        if (cstage != nullptr && mShaderTestLayout.stages[i] == "vs")
        {
            cstage->outTargets = mShaderTestLayout.vertexShaderOutputs;
        }
    }

    for (uint32_t i = 0; i < mShaderTestLayout.stages.size(); i++)
    {
        TestShaderWriter::ShaderResourcesData * cstage = getShaderResourceData(mShaderTestLayout.stages[i]);

        if (cstage != nullptr && mShaderTestLayout.stages[i] == "ps")
        {
            cstage->inTargets = mShaderTestLayout.pixelShaderInputs;
            cstage->outTargets = mShaderTestLayout.pixelShaderOutputs;
        }
    }


    TestInstance testInstance;

    if (mShaderTestLayout.mode == "invalid")
    {
        testInstance.constantBuffer = Mode::INVALID;
    }

    //      
    std::vector<TestShaderWriter::ConstantBufferResource> cbs;
    std::vector<TestShaderWriter::TextureBufferResource> tbs;

    //  
    std::vector<TestShaderWriter::StructuredBufferResource> sbs;
    std::vector<TestShaderWriter::StructuredBufferResource> rwSBs;

    //
    std::vector<TestShaderWriter::RawBufferResource> rbs;
    std::vector<TestShaderWriter::RawBufferResource> rwRBs;

    //
    std::vector<TestShaderWriter::TextureResource> texs;
    std::vector<TestShaderWriter::TextureResource> rwTexs;

    //
    std::vector<TestShaderWriter::SamplerResource> samps;


    //
    RegisterCount initialRegisters;

    bSpaces[0] = initialRegisters;
    tSpaces[0] = initialRegisters;
    sSpaces[0] = initialRegisters;
    uSpaces[0] = initialRegisters;


    //  Generate all the Resources.
    generateConstantBufferResources(cbs, testInstance.constantBuffer);
    generateTextureBufferResources(tbs, testInstance.textureBuffer);
    generateStructuredBufferResources(sbs, rwSBs, testInstance.structredBuffer);
    generateRawBufferResources(rbs, rwRBs, testInstance.rawBuffers);
    generateSamplersResources(samps, testInstance.samplers);
    generateTextureResources(texs, rwTexs, testInstance.textures);

    //  Distribute the Resources.
    distributeConstantBuffers(cbs);
    distributeTextureBuffers(tbs);
    distributeStructuredBuffers(sbs, rwSBs);
    distributeSamplers(samps);
    distributeRawBuffers(rbs, rwRBs);
    distributeTextures(texs, rwTexs);

}


//  Generate the Constant Buffer Resources.
void TestShaderMaker::generateConstantBufferResources(std::vector<TestShaderWriter::ConstantBufferResource> & cbs, const Mode & mode)
{

    //  Generate the Constant Buffers.
    for (uint32_t i = 0; i < mShaderTestLayout.constantbuffers; i++)
    {
        //  
        TestShaderWriter::ConstantBufferResource cbResource;
        cbResource.variable = "cbR" + std::to_string(i);

        //  Check whether we use the resource.
        uint32_t useChance = rand() % 2;

        if (useChance)
        {
            cbResource.isUsedResource = true;
        }

        //  Chance for Explicit Index and Spaces.
        bool isExplicitIndex = ((rand() % 2) == 0);
        bool isExplicitSpace = ((rand() % 2) == 0) && mShaderTestLayout.allowExplicitSpaces;

        //  
        if (isExplicitSpace || isExplicitIndex)
        {
            bool findSpace = false;
            uint32_t availableRegisterIndex;
            uint32_t availableSpaceIndex;

            uint32_t lastSpaceIndex = 0;

            for (auto & currentSpace : bSpaces)
            {
                if (!currentSpace.second.isMaxed)
                {
                    availableRegisterIndex = currentSpace.second.lastRegisterIndex;
                    availableSpaceIndex = currentSpace.first;
                    findSpace = true;
                    break;
                }

                lastSpaceIndex++;
            }

            //  
            if (findSpace == false)
            {
                bSpaces[lastSpaceIndex].isMaxed = false;
                bSpaces[lastSpaceIndex].lastRegisterIndex = 0;

                availableRegisterIndex = bSpaces[lastSpaceIndex].lastRegisterIndex;
                availableSpaceIndex = lastSpaceIndex;
            }


            //
            if (isExplicitIndex)
            {
                cbResource.isExplicitIndex = true;
                cbResource.regIndex = availableRegisterIndex;
                
                //  
                if (mShaderTestLayout.allowConstantBuffersArray)
                {
                    //  Allow Unbounded Constant Buffers.
                    if (mShaderTestLayout.allowUnboundedConstantBuffers)
                    {

                    }
                }
                else
                {
                    bSpaces[availableSpaceIndex].lastRegisterIndex++;
                }
            }

            //  
            if (isExplicitSpace)
            {
                cbResource.isExplicitSpace = true;
                cbResource.regSpace = availableSpaceIndex;
                
                //
                if (mShaderTestLayout.allowConstantBuffersArray)
                {
                    //  Allow Unbounded Constant Buffers.
                    if (mShaderTestLayout.allowUnboundedConstantBuffers)
                    {

                    }
                }
                else
                {
                    bSpaces[availableSpaceIndex].lastRegisterIndex++;
                }
            }
        }


        //  Struct Variable Count.
        uint32_t structVariableCount = (rand() % (mShaderTestLayout.maxStructVariablesCount)) + 1;

        //  Which variable to use.
        uint32_t selectVariable = rand() % structVariableCount;

        //  
        for (uint32_t j = 0; j < structVariableCount; j++)
        {
            TestShaderWriter::StructVariable sv;

            if (j == selectVariable)
            {
                sv.isUsed = true;
            }
            else
            {
                sv.isUsed = false;
            }

            sv.structVariable = "cbR" + std::to_string(i) + "_sv" + std::to_string(j);
            sv.useStructVariable = "use_" + sv.structVariable;
            sv.variableType = ProgramReflection::Variable::Type::Float4;
            cbResource.structVariables.push_back(sv);
        }

        //  
        cbs.push_back(cbResource);
    }
}

//  Generate the Texture Buffer Resources.
void TestShaderMaker::generateTextureBufferResources(std::vector<TestShaderWriter::TextureBufferResource> & tbs, const Mode & mode)
{
    //  Generate the Texture Buffers.
    for (uint32_t i = 0; i < mShaderTestLayout.texturebuffers; i++)
    {
        //  


    }

}

//  Generate the Structured Buffer Resources.
void TestShaderMaker::generateStructuredBufferResources(std::vector<TestShaderWriter::StructuredBufferResource> & sbs, std::vector<TestShaderWriter::StructuredBufferResource> & rwSBs, const Mode & mode)
{
    //  Generate the Structured Buffers.
    for (uint32_t i = 0; i < mShaderTestLayout.structuredbuffers; i++)
    {
        //  
        TestShaderWriter::StructuredBufferResource sbResource;
        sbResource.usesExternalStructVariables = false;
        sbResource.isArray = false;
        sbResource.variable = "sbR" + std::to_string(i);
        sbResource.bufferType = ProgramReflection::BufferReflection::StructuredType::Default;
        sbResource.sbType = ProgramReflection::Variable::Type::Float4;


        //  The chance of it being used.
        uint32_t useChance = rand() % 2;
        if (useChance)
        {
            sbResource.isUsedResource = true;
        }


        //  Chance for Explicit Index and Spaces.
        bool isExplicitIndex = ((rand() % 2) == 0);
        bool isExplicitSpace = ((rand() % 2) == 0) && mShaderTestLayout.allowExplicitSpaces;

        //  
        if (isExplicitSpace || isExplicitIndex)
        {
            bool findSpace = false;
            uint32_t availableRegisterIndex;
            uint32_t availableSpaceIndex;

            uint32_t lastSpaceIndex = 0;

            for (auto & currentSpace : tSpaces)
            {
                if (!currentSpace.second.isMaxed)
                {
                    availableRegisterIndex = currentSpace.second.lastRegisterIndex;
                    availableSpaceIndex = currentSpace.first;
                    findSpace = true;
                    break;
                }

                lastSpaceIndex++;
            }

            //  
            if (findSpace == false)
            {
                tSpaces[lastSpaceIndex].isMaxed = false;
                tSpaces[lastSpaceIndex].lastRegisterIndex = 0;

                availableRegisterIndex = tSpaces[lastSpaceIndex].lastRegisterIndex;
                availableSpaceIndex = lastSpaceIndex;
            }


            //
            if (isExplicitIndex)
            {
                sbResource.isExplicitIndex = true;
                sbResource.regIndex = availableRegisterIndex;

                //  
                if (mShaderTestLayout.allowUnboundedResourceArrays)
                {

                }
                else
                {
                    tSpaces[availableSpaceIndex].lastRegisterIndex++;
                }
            }

            //  
            if (isExplicitSpace)
            {
                sbResource.isExplicitSpace = true;
                sbResource.regSpace = availableSpaceIndex;

                //
                if (mShaderTestLayout.allowUnboundedResourceArrays)
                {

                }
                else
                {
                    tSpaces[availableSpaceIndex].lastRegisterIndex++;
                }
            }
        }


        //  
        sbs.push_back(sbResource);
    }

}


//  Generate the Raw Buffer Resources.
void TestShaderMaker::generateRawBufferResources(std::vector<TestShaderWriter::RawBufferResource> & rbs, std::vector<TestShaderWriter::RawBufferResource> & rwRbs, const Mode & mode)
{
    //  Generate the Raw Buffers.
    for (uint32_t i = 0; i < mShaderTestLayout.rawbuffers; i++)
    {
        //  
        TestShaderWriter::RawBufferResource rbResource;
        rbResource.variable = "rbR" + std::to_string(i);

        rbs.push_back(rbResource);
    }
}

//  Generate the Samplers Resources.
void TestShaderMaker::generateSamplersResources(std::vector<TestShaderWriter::SamplerResource> & samps, const Mode & mode)
{
    //  Generate the Textures.
    for (uint32_t i = 0; i < mShaderTestLayout.samplers; i++)
    {
        //  Sampler Resource.
        TestShaderWriter::SamplerResource samplerResource;
        samplerResource.variable = "samplerR" + std::to_string(i);


        //  Push back the Sampler Resource.
        samps.push_back(samplerResource);
    }
}

//  Generate the Texture Resources.
void TestShaderMaker::generateTextureResources(std::vector<TestShaderWriter::TextureResource> & texs, std::vector<TestShaderWriter::TextureResource> & rwTexs, const Mode & mode)
{
    //  Generate the Textures.
    for (uint32_t i = 0; i < mShaderTestLayout.textures; i++)
    {
        TestShaderWriter::TextureResource textureResource;
        textureResource.textureResourceType = ProgramReflection::Resource::Dimensions::Texture2D;
        textureResource.textureType = ProgramReflection::Variable::Type::Float4;
        textureResource.variable = "textureR" + std::to_string(i);

        
        //  The chance of it being used.
        uint32_t useChance = rand() % 2;

        if (useChance)
        {
            textureResource.isUsedResource = true;
        }


        //  Chance for Explicit Index and Spaces.
        bool isExplicitIndex = ((rand() % 2) == 0);
        bool isExplicitSpace = ((rand() % 2) == 0) && mShaderTestLayout.allowExplicitSpaces;

        //  
        if (isExplicitSpace || isExplicitIndex)
        {
            bool findSpace = false;
            uint32_t availableRegisterIndex;
            uint32_t availableSpaceIndex;

            uint32_t lastSpaceIndex = 0;

            for (auto & currentSpace : tSpaces)
            {
                if (!currentSpace.second.isMaxed)
                {
                    availableRegisterIndex = currentSpace.second.lastRegisterIndex;
                    availableSpaceIndex = currentSpace.first;
                    findSpace = true;
                    break;
                }

                lastSpaceIndex++;
            }

            //  
            if (findSpace == false)
            {
                tSpaces[lastSpaceIndex].isMaxed = false;
                tSpaces[lastSpaceIndex].lastRegisterIndex = 0;

                availableRegisterIndex = tSpaces[lastSpaceIndex].lastRegisterIndex;
                availableSpaceIndex = lastSpaceIndex;
            }


            //
            if (isExplicitIndex)
            {
                textureResource.isExplicitIndex = true;
                textureResource.regIndex = availableRegisterIndex;

                //  
                if (mShaderTestLayout.allowUnboundedResourceArrays)
                {

                }
                else
                {
                    tSpaces[availableSpaceIndex].lastRegisterIndex++;
                }
            }

            //  
            if (isExplicitSpace)
            {
                textureResource.isExplicitSpace = true;
                textureResource.regSpace = availableSpaceIndex;

                //
                if (mShaderTestLayout.allowUnboundedResourceArrays)
                {

                }
                else
                {
                    tSpaces[availableSpaceIndex].lastRegisterIndex++;
                }
            }
        }

        texs.push_back(textureResource);
    }

}

//  
void TestShaderMaker::getResourceDistribution(std::vector<uint32_t> & declareAll, std::vector<uint32_t> & declareMixed, std::vector<uint32_t> & declareNone)
{
    //  
    for (uint32_t i = 0; i < mShaderTestLayout.declarations.size(); i++)
    {
        if (mShaderTestLayout.declarations[i] == "all")
        {
            declareAll.push_back(i);
        }
        else if (mShaderTestLayout.declarations[i] == "mixed")
        {
            declareMixed.push_back(i);
        }
        else
        {
            declareNone.push_back(i);
        }
    }
}

//  Get the Shader Resource Data.
TestShaderWriter::ShaderResourcesData * TestShaderMaker::getShaderResourceData(const std::string & stage)
{
    if (stage == "vs")
    {
        return mShaderMaker.getVSResourceData();
    }
    else if (stage == "hs")
    {
        return mShaderMaker.getHSResourceData();
    }
    else if (stage == "ds")
    {
        return mShaderMaker.getDSResourceData();
    }
    else if (stage == "gs")
    {
        return mShaderMaker.getGSResourceData();
    }
    else if (stage == "ps")
    {
        return mShaderMaker.getPSResourceData();
    }
    else
    {
        return nullptr;
    }


}

//  Generate the Randomized Stages.
std::vector<uint32_t> TestShaderMaker::generateRandomizedStages()
{
    std::vector<uint32_t> selectedStages;
    uint32_t stageCount = 0;

    //  
    if (mShaderTestLayout.stages.size() <= 1)
    {
        stageCount = 1;
    }
    else
    {
        stageCount = (rand() % mShaderTestLayout.stages.size()) + 1;
    }

    //  
    std::vector<uint32_t> allStagesIndices;
    for (uint32_t stageIndex = 0; stageIndex < mShaderTestLayout.stages.size(); stageIndex++)
    {
        allStagesIndices.push_back(stageIndex);
    }

    //  
    for (uint32_t stageIndex = 0; stageIndex < stageCount; stageIndex++)
    {
        uint32_t selectStage = rand() % allStagesIndices.size();
        selectedStages.push_back(allStagesIndices[selectStage]);
        allStagesIndices[selectStage] = allStagesIndices[allStagesIndices.size() - 1];
        allStagesIndices.pop_back();
    }
    
    //  
    return selectedStages;
}

//  Distribute the Constant Buffer Resources to the Shader Stages.
void TestShaderMaker::distributeConstantBuffers(std::vector<TestShaderWriter::ConstantBufferResource> & cbs)
{
    //  Iterate over the Constant Buffer Indices.
    for (uint32_t cbIndex = 0; cbIndex < cbs.size(); cbIndex++)
    {
        std::vector<uint32_t> selectedStages = generateRandomizedStages();

        uint32_t declareExplicitStage = 0;

        if (selectedStages.size() <= 1)
        {
            declareExplicitStage = (uint32_t)selectedStages.size();
        }
        else
        {
            declareExplicitStage = (rand() % selectedStages.size());
        }

        for (uint32_t stageIndex = 0; stageIndex < selectedStages.size(); stageIndex++)
        {
            TestShaderWriter::ConstantBufferResource currentCB(cbs[cbIndex]);

            if (stageIndex == declareExplicitStage)
            {
                currentCB.isExplicitIndex = true && currentCB.isExplicitIndex;
            }
            else
            {
                currentCB.isExplicitIndex = false;
                currentCB.isExplicitSpace = false;
            }

            //  
            TestShaderWriter::ShaderResourcesData * sRD = getShaderResourceData(mShaderTestLayout.stages[selectedStages[stageIndex]]);

            if (sRD)
            {
                sRD->cbs.push_back(currentCB);
            }
        }
    }
}







//  Distribute the Texture Buffers.
void TestShaderMaker::distributeTextureBuffers(std::vector<TestShaderWriter::TextureBufferResource> & tbs)
{
    //  Iterate the Texture Buffer Resource.
    for (uint32_t i = 0; i < tbs.size(); i++)
    {


    }

}

//  Distribute the Structured Buffers.
void TestShaderMaker::distributeStructuredBuffers(std::vector<TestShaderWriter::StructuredBufferResource> & sbs, std::vector<TestShaderWriter::StructuredBufferResource> & rwSBs)
{
    //
    for (uint32_t currentStructuredBufferIndex = 0; currentStructuredBufferIndex < sbs.size(); currentStructuredBufferIndex++)
    {
        std::vector<uint32_t> selectedStages = generateRandomizedStages();

        uint32_t declareExplicitStage = 0;

        if (selectedStages.size() <= 1)
        {
            declareExplicitStage = (uint32_t)selectedStages.size();
        }
        else
        {
            declareExplicitStage = (rand() % selectedStages.size());
        }

        for (uint32_t stageIndex = 0; stageIndex < selectedStages.size(); stageIndex++)
        {
            TestShaderWriter::StructuredBufferResource currentSB(sbs[currentStructuredBufferIndex]);

            if (stageIndex == declareExplicitStage)
            {
                currentSB.isExplicitIndex = true && currentSB.isExplicitIndex;
            }
            else
            {
                currentSB.isExplicitIndex = false;
                currentSB.isExplicitSpace = false;
            }

            //  
            TestShaderWriter::ShaderResourcesData * sRD = getShaderResourceData(mShaderTestLayout.stages[selectedStages[stageIndex]]);

            if (sRD)
            {
                sRD->sbs.push_back(currentSB);
            }
        }

    }


    //  Distribute the Read Write Textures.
    for (uint32_t currentStructuredBufferIndex = 0; currentStructuredBufferIndex < rwSBs.size(); currentStructuredBufferIndex++)
    {


    }


}

//  Distribute the Samplers.
void TestShaderMaker::distributeSamplers(std::vector<TestShaderWriter::SamplerResource> & samplerResources)
{
    //  
    for (uint32_t currentSamplerIndex = 0; currentSamplerIndex  < samplerResources.size(); currentSamplerIndex++)
    {

    }
}

//  Distribute the Textures.
void TestShaderMaker::distributeTextures(std::vector<TestShaderWriter::TextureResource> & texs, std::vector<TestShaderWriter::TextureResource> & rwTexs)
{
    for (uint32_t currentTextureIndex = 0; currentTextureIndex < texs.size(); currentTextureIndex++)
    {
        std::vector<uint32_t> selectedStages = generateRandomizedStages();

        uint32_t declareExplicitStage = 0;

        if (selectedStages.size() <= 1)
        {
            declareExplicitStage = (uint32_t)selectedStages.size();
        }
        else
        {
            declareExplicitStage = (rand() % selectedStages.size());
        }

        for (uint32_t stageIndex = 0; stageIndex < selectedStages.size(); stageIndex++)
        {
            TestShaderWriter::TextureResource currentTexture(texs[currentTextureIndex]);

            if (stageIndex == declareExplicitStage)
            {
                currentTexture.isExplicitIndex = true && currentTexture.isExplicitIndex;
            }
            else
            {
                currentTexture.isExplicitIndex = false;
                currentTexture.isExplicitSpace = false;
            }

            //  
            TestShaderWriter::ShaderResourcesData * sRD = getShaderResourceData(mShaderTestLayout.stages[selectedStages[stageIndex]]);

            if (sRD)
            {
                sRD->textures.push_back(currentTexture);
            }
        }
    }

    //  Distribute the Read Write Textures.
    for (uint32_t currentTextureIndex = 0; currentTextureIndex < rwTexs.size(); currentTextureIndex++)
    {


    }
}


//  Distribute the Raw Buffers.
void TestShaderMaker::distributeRawBuffers(std::vector<TestShaderWriter::RawBufferResource> & rBs, std::vector<TestShaderWriter::RawBufferResource> & rwRBs)
{
    //  Distribute the Raw Buffer Resources.
    for (uint32_t currentRBIndex = 0; currentRBIndex < rBs.size(); currentRBIndex++)
    {


    }

    //  Distribute the RW Raw Buffer Resources.
    for (uint32_t currentRBIndex = 0; currentRBIndex < rBs.size(); currentRBIndex++)
    {


    }


}


//  Verify All the Resources.
bool TestShaderMaker::verifyAllResources(const TestShaderWriter::ShaderResourcesData & rsData, const GraphicsVars::SharedPtr & graphicsProgramVars)
{
    //  Program Reflection.
    ProgramReflection::SharedConstPtr mpReflector = graphicsProgramVars->getReflection();

    //  Verify Constant Buffer Resources.
    const ProgramReflection::BufferMap& cbBufferMap = mpReflector->getBufferMap(ProgramReflection::BufferReflection::Type::Constant);

    //  Verify Structured Buffer Resources.
    const ProgramReflection::BufferMap& sbBufferMap = mpReflector->getBufferMap(ProgramReflection::BufferReflection::Type::Structured);

    //  Verify the Structured Buffers
    for (const auto & currentSB : rsData.sbs)
    {
        //  
        const ProgramReflection::Resource * cSBRef = mpReflector->getResourceDesc(currentSB.variable);
    }

    //  Verify the Raw Buffers
    for (const auto & currentRBs : rsData.rbs)
    {
        //  
        const ProgramReflection::Resource * cRBRef = mpReflector->getResourceDesc(currentRBs.variable);
    }


    //  Verify the Samplers.
    for (const auto & currentSampler : rsData.samplers)
    {
        //  
        const ProgramReflection::Resource * cSamplerRef = mpReflector->getResourceDesc(currentSampler.variable);
    }

    //  Verify the Textures.
    for (const auto & currentTexture : rsData.textures)
    {
        //  
        const ProgramReflection::Resource * cTextureRef = mpReflector->getResourceDesc(currentTexture.variable);

    }

    return true;
}
