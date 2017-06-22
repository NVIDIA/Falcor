#pragma once

#include "Falcor.h"
#include "TestShaderWriter.h"
#include <string.h>
#include <map>
#include <vector>
#include <set>
#include <random>
#include <algorithm>    // std::random_shuffle
#include <ctime>        // std::time
#include <cstdlib>      // std::rand, std::srand

#include "Utils/OS.h"
#include "Externals/RapidJson/include/rapidjson/document.h"
#include "Externals/RapidJson/include/rapidjson/stringbuffer.h"
#include "Externals/RapidJson/include/rapidjson/prettywriter.h"
#include "Externals/RapidJson/include/rapidjson/error/en.h"
#include <sstream>
#include <fstream>


class TestShaderMaker
{

public:

    struct TestShaderLayout
    {
        //  The Random Seed.
        uint32_t randomseed;

        //  The Version for the Shader Test Layout.
        std::string version;

        //  The Shader Stages.
        std::vector<std::string> stages;

        //  The Declarations.
        std::vector<std::string> declarations;

        //  The Constant Buffers Count.
        uint32_t constantbuffers;

        //  The Texture Buffers Count.
        uint32_t texturebuffers;

        //  The Structured Buffers Count.
        uint32_t structuredbuffers;
        uint32_t rwStructuredBuffers;

        //  The Raw Buffers Count.
        uint32_t rawbuffers;
        uint32_t rwRawBuffers;

        //  The Textures Count.
        uint32_t textures;
        uint32_t rwTextures;

        //  The Samplers Count.
        uint32_t samplers;

        //  The Mode.
        std::string mode;

        //  Whether or not we have a Vertex Shader.
        bool hasVertexShader;

        //  Whether or not we have a Hull Shader.
        bool hasHullShader;

        //  Whether or not we have a DomainShader.
        bool hasDomainShader;

        //  Whether or not we have a Geometry Shader.
        bool hasGeometryShader;

        //  Whether or not we have a Pixel Shader.
        bool hasPixelShader;

        //  Compute Shader.
        bool hasComputeShader;

        //  Some general constants.

        //  Maximum number of Struct Variables.
        uint32_t maxStructVariablesCount = 3;


        //  Maximum UAV Targets.
        uint32_t maxUAVTargets = 8;

        //  Maximum Pixel Shader Output.
        uint32_t maxPixelShaderOutput = 8;


        //  Maximum Constant Buffer Count.
        uint32_t maximumConstantBufferCount = 15;

        //  Maximum Sampler Count.
        uint32_t maximumSamplerCount = 15;

        //  Allow Unbounded Constant Buffers.
        bool allowUnboundedConstantBuffers = false;
        bool allowConstantBuffersArray = false;
        
        //  Allow Struct Constant Buffers.
        bool allowStructConstantBuffers = false;

        //  Allow Unbounded Resource Arrays.
        bool allowUnboundedResourceArrays = false;

        //  Allow Explicit Spaces.
        bool allowExplicitSpaces = false;
        

        //  
        std::vector<uint32_t> vertexShaderOutputs;
        std::vector<uint32_t> pixelShaderInputs;
        std::vector<uint32_t> pixelShaderOutputs;
    };


    //  Types of Invalid Configurations.
    //  Constant Buffer Invalid - Redefinition / Overlapping Registers.
    //  Texture Buffer Invalid - Redefinition / Overlapping Registers.
    //  StructuredBuffer Invalid - Redefinition / Overlapping Registers.
    //  Textures - Redefinition / Overlapping Registers.
    //  RawBuffer - Redefinition / Overlapping Registers.
    //  Samplers - Redefinition / Overlapping Registers.
    enum class Mode : int
    {
        VALID = 0, 
        INVALID = 1
    };

    struct TestInstance
    {
        Mode constantBuffer;
        Mode textureBuffer;
        Mode structredBuffer;
        Mode textures;
        Mode rawBuffers;
        Mode samplers;
    };


    //  
    struct RegisterCount
    {
        bool isMaxed = false;
        uint32_t lastRegisterIndex = 0;
    };


    //  Load the Configuration File.
    bool loadConfigFile(const std::string & configFile, std::string errorMessage);

    //  Set the Version Restricted Constants.
    void setVersionRestrictedConstants();

    //  Return the Shader Test Layout in a const format.
    const TestShaderLayout & viewShaderTestLayout() const;

    //  Return the Shader Test Layout.
    TestShaderLayout & getShaderTestLayout();

    //  Generate the Shader Program.
    void generateShaderProgram();

    //  Get Vertex Shader Code.
    std::string getVertexShaderCode() const;

    //  Get Hull Shader Code.
    std::string getHullShaderCode() const;

    //  Get Domain Shader Code.
    std::string getDomainShaderCode() const;

    //  Get Geometry Shader Code.
    std::string getGeometryShaderCode() const;

    //  Get Pixel Shader Code.
    std::string getPixelShaderCode() const;

    
    //  Verify that the Resources are in keeping with the Program Creation.
    bool verifyProgramResources(const GraphicsVars::SharedPtr & graphicsProgramVars);

    //  Verify that the Resources are in keepign with the Shader Stage.
    bool verifyProgramResources(const TestShaderWriter::ShaderResourcesData & rsData, const GraphicsVars::SharedPtr & graphicsProgramVars);


private:


    
    //  Generate the Constant Buffer Resources.
    void generateConstantBufferResources(std::vector<TestShaderWriter::ConstantBufferResource> & cbs, const Mode & mode);

    //  Generate the Texture Buffer Resources.
    void generateTextureBufferResources(std::vector<TestShaderWriter::TextureBufferResource> & tbs, const Mode & mode);

    //  Generate the Structured Buffer Resources.
    void generateStructuredBufferResources(std::vector<TestShaderWriter::StructuredBufferResource> & sbs, std::vector<TestShaderWriter::StructuredBufferResource> & rwSBs, const Mode & mode);

    //  Generate the Raw Buffer Resources.
    void generateRawBufferResources(std::vector<TestShaderWriter::RawBufferResource> & rbs, std::vector<TestShaderWriter::RawBufferResource> & rwRBs, const Mode & mode);

    //  Generate the Sampler Resources.
    void generateSamplersResources(std::vector<TestShaderWriter::SamplerResource> & samps, const Mode & mode);

    //  Generate the Texture Resources.
    void generateTextureResources(std::vector<TestShaderWriter::TextureResource> & texs, std::vector<TestShaderWriter::TextureResource> & rwTexs, const Mode & mode);

    //  Get the Resource Distribution amongst the stages.
    void getResourceDistribution(std::vector<uint32_t> & declareAll, std::vector<uint32_t> & declareMixed, std::vector<uint32_t> & declareNone);

    //  Return a pointer to the Resource Meta Data of the specified Stage. 
    TestShaderWriter::ShaderResourcesData * getShaderResourceData(const std::string & stage);

    //  Return a Randomized Selection of Stages.
    std::vector<uint32_t> generateRandomizedStages();

    //  Distribute the Constant Buffer Resources.
    void distributeConstantBuffers(std::vector<TestShaderWriter::ConstantBufferResource> & cbs);

    //   Distribute the Texture BUffer Resources.
    void distributeTextureBuffers(std::vector<TestShaderWriter::TextureBufferResource> & tbs);

    //  Distribute the Structured Buffers.
    void distributeStructuredBuffers(std::vector<TestShaderWriter::StructuredBufferResource> & sbs, std::vector<TestShaderWriter::StructuredBufferResource> & rwSBs);

    //  Distribute the Samplers.
    void distributeSamplers(std::vector<TestShaderWriter::SamplerResource> & samplerResources);

    //  Distribute the Textures.
    void distributeTextures(std::vector<TestShaderWriter::TextureResource> & texs, std::vector<TestShaderWriter::TextureResource> & rwTexs);

    //  Distribute the Raw Buffers.
    void distributeRawBuffers(std::vector<TestShaderWriter::RawBufferResource> & rBs, std::vector<TestShaderWriter::RawBufferResource> & rwRBs);

    //  Verify All Resources.
    bool verifyAllResources(const TestShaderWriter::ShaderResourcesData & rsData, const GraphicsVars::SharedPtr & graphicsProgramVars);


    //  JSON Document.
    rapidjson::Document mJDoc;

    //  Shader Maker.
    TestShaderWriter mShaderMaker;

    //  Shader Test Layout.
    TestShaderLayout mShaderTestLayout;

    //  b Registers.
    std::map<uint32_t, RegisterCount> bSpaces;

    //  s Register.
    std::map<uint32_t, RegisterCount> sSpaces;

    //  t Registers.
    std::map<uint32_t, RegisterCount> tSpaces;

    //  u Registers.
    std::map<uint32_t, RegisterCount> uSpaces;

};

