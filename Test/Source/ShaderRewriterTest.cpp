
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
#include "ShaderRewriterTest.h"



//  Add the Tests to the Shader Test List.
void ShaderRewriterTest::addTests()
{
    addTestToList<TestBasicShaderRewriter>();
}


//
testing_func(ShaderRewriterTest, TestBasicShaderRewriter)
{
    //  
    std::string configFile = "config.json";
    std::string errorMessage = "";

    //  Create the Shader Test Maker.
    TestShaderMaker shaderTestMaker;

    //  Try and load the Configuration files.
    bool isConfigFileLoaded = shaderTestMaker.loadConfigFile(configFile, errorMessage);
    if (!isConfigFileLoaded)
    {
        return test_fail(errorMessage);
    }


    //  Seed the Random Number Generator.
    std::srand(shaderTestMaker.viewShaderTestLayout().randomseed);

    shaderTestMaker.getShaderTestLayout().vertexShaderOutputs = { 0, 1, 4 };
    shaderTestMaker.getShaderTestLayout().pixelShaderInputs = { 0, 1, 4, 2 };
    shaderTestMaker.getShaderTestLayout().pixelShaderOutputs = { 0, 3, 5, 7 };

    //  Generate the Shader Program.
    shaderTestMaker.generateShaderProgram();

    //  
    std::string vsCode = "";
    std::string hsCode = "";
    std::string dsCode = "";
    std::string gsCode = "";
    std::string psCode = "";


    if (shaderTestMaker.viewShaderTestLayout().hasVertexShader && shaderTestMaker.viewShaderTestLayout().hasHullShader && shaderTestMaker.viewShaderTestLayout().hasDomainShader && shaderTestMaker.viewShaderTestLayout().hasGeometryShader && shaderTestMaker.viewShaderTestLayout().hasPixelShader)
    {
        //  Check for a Complete Rendering pipeline.
        vsCode = shaderTestMaker.getVertexShaderCode();
        hsCode = shaderTestMaker.getHullShaderCode();
        dsCode = shaderTestMaker.getDomainShaderCode();
        gsCode = shaderTestMaker.getGeometryShaderCode();
        psCode = shaderTestMaker.getPixelShaderCode();

    }
    else if (shaderTestMaker.viewShaderTestLayout().hasVertexShader && shaderTestMaker.viewShaderTestLayout().hasPixelShader)
    {
        //  Check if we have a VS and PS pipeline.
        vsCode = shaderTestMaker.getVertexShaderCode();
        psCode = shaderTestMaker.getPixelShaderCode();
    }
    else if (shaderTestMaker.viewShaderTestLayout().hasComputeShader)
    {
        //  Don't yet support
        test_fail("Compute Not Yet Tested!")
    }
    else
    {
        test_fail("Test Case does not have a valid Shader Stage Set!")
    }


    //  Write the Pixel Shader to file, for further review.
    std::ofstream ofvs;
    ofvs.open("Data/VS.vs.hlsl", std::ofstream::trunc);
    ofvs << vsCode;
    ofvs.close();

    //  Write the Pixel Shader to file, for further review.
    std::ofstream ofps;
    ofps.open("Data/PS.ps.hlsl", std::ofstream::trunc);
    ofps << psCode;
    ofps.close();


    //  Create the Graphics Program.
    GraphicsProgram::SharedPtr graphicsProgram = GraphicsProgram::createFromFile("VS.vs.hlsl", "PS.ps.hlsl");

    //  Create the Graphics Program.
    GraphicsVars::SharedPtr graphicsProgramVars = GraphicsVars::create(graphicsProgram->getActiveVersion()->getReflector());

    //  
    bool resourceVerify = shaderTestMaker.verifyProgramResources(graphicsProgramVars);

    //  
    if (resourceVerify)
    {
        //  Return Test Pass.
        return test_pass();
    }
    else
    {
        return test_fail("Error - Could not verify.");
    }

}



int main()
{
    ShaderRewriterTest st;
    st.init(true);
    st.run();
    return 0;
}
