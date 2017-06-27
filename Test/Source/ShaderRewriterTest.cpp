
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
    addTestToList<TestShaderRewriter>();
}


//
testing_func(ShaderRewriterTest, TestShaderRewriter)
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

    //  Generate the Shader Program.
    shaderTestMaker.generateShaderProgram();

    //  
    bool resourceVerify = false;

    if (shaderTestMaker.viewShaderTestLayout().hasVertexShader && shaderTestMaker.viewShaderTestLayout().hasHullShader && shaderTestMaker.viewShaderTestLayout().hasDomainShader && shaderTestMaker.viewShaderTestLayout().hasGeometryShader && shaderTestMaker.viewShaderTestLayout().hasPixelShader)
    {

        //  Check for a Complete Rendering pipeline.
        std::string vsCode = shaderTestMaker.getVertexShaderCode();
        std::string hsCode = shaderTestMaker.getHullShaderCode();
        std::string dsCode = shaderTestMaker.getDomainShaderCode();
        std::string gsCode = shaderTestMaker.getGeometryShaderCode();
        std::string psCode = shaderTestMaker.getPixelShaderCode();

        std::ofstream ofvs;
        ofvs.open("Data/VS.vs.hlsl", std::ofstream::trunc);
        ofvs << vsCode;
        ofvs.close();

        std::ofstream ofps;
        ofps.open("Data/PS.ps.hlsl", std::ofstream::trunc);
        ofps << psCode;
        ofps.close();

        std::ofstream ofgs;
        ofgs.open("Data/GS.gs.hlsl", std::ofstream::trunc);
        ofgs << gsCode;
        ofgs.close();

        std::ofstream ofhs;
        ofhs.open("Data/HS.hs.hlsl", std::ofstream::trunc);
        ofhs << hsCode;
        ofhs.close();

        std::ofstream ofds;
        ofds.open("Data/DS.ds.hlsl", std::ofstream::trunc);
        ofds << dsCode;
        ofds.close();

        //  Create the Graphics Program.
        GraphicsProgram::SharedPtr graphicsProgram = GraphicsProgram::createFromFile("VS.vs.hlsl", "PS.ps.hlsl", "GS.gs.hlsl", "HS.hs.hlsl", "DS.ds.hlsl");

        //
        ProgramVersion::SharedConstPtr graphicsProgramVersion = (graphicsProgram->getActiveVersion());

        if (graphicsProgramVersion)
        {
            //  Create the Graphics Program.
            GraphicsVars::SharedPtr graphicsProgramVars = GraphicsVars::create(graphicsProgram->getActiveVersion()->getReflector());
          
            std::string errorMessage = "";

            //  Verify the Program Resources.
            resourceVerify = shaderTestMaker.verifyProgramResources(graphicsProgramVars, errorMessage);

            //
            if (!resourceVerify)
            {
                test_fail(errorMessage);
            }
        }
        else
        {
            if (shaderTestMaker.viewShaderTestLayout().mode == "invalid")
            {
                return test_pass();
            }
            else
            {
                return test_fail("Error - Could not compile shaders!");
            }
        }

    }
    else if (shaderTestMaker.viewShaderTestLayout().hasVertexShader && shaderTestMaker.viewShaderTestLayout().hasGeometryShader && shaderTestMaker.viewShaderTestLayout().hasPixelShader)
    {

        //  Get the Vertex Shader Code.
        std::string vsCode = shaderTestMaker.getVertexShaderCode();
        
        //  Get the Pixel Shader Code.
        std::string psCode = shaderTestMaker.getPixelShaderCode();
        
        //  Get the Geometry Shader Code.
        std::string gsCode = shaderTestMaker.getGeometryShaderCode();

        std::ofstream ofvs;
        ofvs.open("Data/VS.vs.hlsl", std::ofstream::trunc);
        ofvs << vsCode;
        ofvs.close();

        std::ofstream ofps;
        ofps.open("Data/PS.ps.hlsl", std::ofstream::trunc);
        ofps << psCode;
        ofps.close();

        std::ofstream ofgs;
        ofgs.open("Data/GS.gs.hlsl", std::ofstream::trunc);
        ofgs << gsCode;
        ofgs.close();

        //  Create the Graphics Program.
        GraphicsProgram::SharedPtr graphicsProgram = GraphicsProgram::createFromFile("VS.vs.hlsl", "PS.ps.hlsl", "GS.gs.hlsl", "", "");

        //
        ProgramVersion::SharedConstPtr graphicsProgramVersion = (graphicsProgram->getActiveVersion());

        if (graphicsProgramVersion)
        {
            if (shaderTestMaker.viewShaderTestLayout().mode == "invalid")
            {
                return test_fail("Error - Compiled Invalid Shaders!");
            }

            //  Create the Graphics Program.
            GraphicsVars::SharedPtr graphicsProgramVars = GraphicsVars::create(graphicsProgramVersion->getReflector());

            std::string errorMessage = "";

            //  Verify the Program Resources.
            resourceVerify = shaderTestMaker.verifyProgramResources(graphicsProgramVars, errorMessage);
            
            //
            if (!resourceVerify)
            {
                test_fail(errorMessage);
            }
        }
        else
        {
            if (shaderTestMaker.viewShaderTestLayout().mode == "invalid")
            {
                return test_pass();
            }
            else
            {
                return test_fail("Error - Could not compile shaders!");
            }
        }

    }
    else if (shaderTestMaker.viewShaderTestLayout().hasVertexShader && shaderTestMaker.viewShaderTestLayout().hasPixelShader)
    {
        //  Check if we have a VS and PS pipeline.
        std::string vsCode = shaderTestMaker.getVertexShaderCode();
        std::string psCode = shaderTestMaker.getPixelShaderCode();

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

        //
        ProgramVersion::SharedConstPtr graphicsProgramVersion = (graphicsProgram->getActiveVersion(true));

        if (graphicsProgramVersion)
        {
            if (shaderTestMaker.viewShaderTestLayout().mode == "invalid")
            {
                return test_fail("Error - Compiled Invalid Shaders!");
            }

            //  Create the Graphics Program.
            GraphicsVars::SharedPtr graphicsProgramVars = GraphicsVars::create(graphicsProgramVersion->getReflector());

            std::string errorMessage;

            //  Verify the Program Resources.
            resourceVerify = shaderTestMaker.verifyProgramResources(graphicsProgramVars, errorMessage);

            //
            if (!resourceVerify)
            {
                return test_fail(errorMessage);
            }
        }
        else
        {
            if (shaderTestMaker.viewShaderTestLayout().mode == "invalid")
            {
                return test_pass();
            }
            else
            {
                return test_fail("Error - Could not compile shaders!");
            }
        }
    }
    else if (shaderTestMaker.viewShaderTestLayout().hasComputeShader)
    {
        std::string csCode = shaderTestMaker.getComputeShaderCode();

        //  Write the Compute Shader to file, for further review.
        std::ofstream ofvs;
        ofvs.open("Data/CS.cs.hlsl", std::ofstream::trunc);
        ofvs << csCode;
        ofvs.close();
 
    
        //  Create the Compute Program.
        ComputeProgram::SharedPtr computeProgram = ComputeProgram::createFromFile("CS.cs.hlsl");

        //
        ProgramVersion::SharedConstPtr computeProgramVersion = (computeProgram->getActiveVersion(true));

        if (computeProgramVersion)
        {
            if (shaderTestMaker.viewShaderTestLayout().mode == "invalid")
            {
                return test_fail("Error - Compiled Invalid Shaders!");
            }

            //  Create the Graphics Program.
            ComputeVars::SharedPtr graphicsProgramVars = ComputeVars::create(computeProgramVersion->getReflector());

            std::string errorMessage;

            //  Verify the Program Resources.
            resourceVerify = shaderTestMaker.verifyProgramResources(graphicsProgramVars, errorMessage);

            //
            if (!resourceVerify)
            {
                test_fail(errorMessage);
            }
        }
        else
        {
            if (shaderTestMaker.viewShaderTestLayout().mode == "invalid")
            {
                return test_pass();
            }
            else
            {
                return test_fail("Error - Could not compile shaders!");
            }
        }
    }
    else
    {
        test_fail("Test Case does not have a valid Shader Stage Set!")
    }


    //  
    if (resourceVerify)
    {
        //  Return Test Pass.
        return test_pass();
    }
    else
    {
        return test_fail("Error - Could not verify resources.");
    }

}



int main()
{
    ShaderRewriterTest st;
    st.init(true);
    st.run();
    return 0;
}
