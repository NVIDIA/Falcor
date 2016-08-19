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
#include <string>
#include <vector>
#include <map>

namespace Falcor
{
    /** Shader pre-processor class.
        The class handles the following directives:
        - #include
        - #expect
        - #foreach
        - #for
        \nNesting of directives is allowed. For example, the code snippet is legal
        \code
        #foreach (a, b) in (2, dir), (2, tex), (3, color)
        #for(int i = 0 ; i < 2 ; ++i)
        in vec$(a) $(b)$(i);
        #endfor
        #endforeach
        \endcode
        see the descriptions of the different directives to understand what this code is doing.
        \n\nThe pre-processor can also embed macro-definitions into the shader string. The macro definitions will be embedded right after the #version directive.

        <h4>#include</h4>
        Similar to C/C++, will add the included file into the source. '#pragma once' can be used inside an header so that it's included only once.

        <h4>#expect</h4>
        \code
        #expect MACRO_NAME
        or
        #expect MACRO_NAME optional macro description.
        \endcode

        #expect can be used to let the compiler know we expect a macro to be defined. The definition should come **before** the expect. If this definition is not available, compilation will fail.\n

        <h4>#foreach</h4>
        \code
        #foreach key in val0, val1, val2, val3
            Do something with $(key);
        #endforeach
        \endcode

        #foreach directive will cause the inner body to be repeated multiple time, for each value field.\n
        Any occurrence of $(key) will be replaced with the value.\n\n

        The pre-processor will provide 2 implicit indices:
        * $(_keyIndex) - The index of the current key. If you are using a single key an not the tuple version, this is always zero. See explanation on key/value tuples below.
        * $(_valIndex) - The index of the current value or value tuple.

        For example, the following code:
        \code
        #foreach color in C0, C1, C2
            uniform vec4 $(color);
        #endforeach
        \endcode
    
        will be expanded to
        \code
        uniform vec4 C0;
        uniform vec4 C1;
        uniform vec4 C2;
        \endcode
        \n

        The key and value can be a tuple of strings.
        \code
        #foreach (key1, key2) in (val00, val01), (val10, val10)
            Do something with $(key1) and $(key2);
        #endforeach
        \endcode

        The parentheses around the keys tuple are optional, but mandatory for the values tuples. Each value tuple must contain the exact number of elements as the keys tuple.\n
        #foreach supports macro-expansion for macros defined using the C++ API. Macros defined in the shader file itself can't be used in #foreach loops.
        
        This:
        \code
        #foreach key in v0, v1, v2
        \endcode
        is equivalent to
        \code
        #foreach key in V123
        \endcode
        Where v123 was defined using the C++ interface with the string "V123 v0, v1, v2"
        This is very powerful, as it allows user to implement generic shaders and instantiate shaders dynamically by specifying macro-definition using the C++ interface.

        The values are optional. You can safely use
        \code
        #foreach k1 do
            do something with $(k1)
        #endfor
        \endcode
        This is helpful when passing macros using the C++ API, in case the macro is empty (For example, if the macro represent a list of objects, the list can be empty).

        <h4>#for</h4>
        \code
        #for (int iteratorName; iteratorName < IntegerLiteralOrMacro; ++iteratorName)
            Do something with $(iteratorName);
        #endfor
        \endcode

        The #for directive will cause the inner body to be repeated multiple time, based on the iterator.\n
        You can use whatever string you'd like as the iteratorName (even if it's illegal in the shading language).\n
        IntegerLiteralOrMacro must be an integer literal or a previously-defined macro which expands to an integer literal. The macro must be defined using the C++ interface. 
        Macros defined in the shader file itself can't be used in #for loops.\n
        This pragma has some limitations:
            - The iterator must be a signed-integer (declared with an 'int'). 
            - Only less-than operator is allowed.
            - Only prefix-increment operator is allowed (++iteratorName)

        \n\nAs an example, the following
        \code
        #for (int i = 0 ; i < TEX_CRD_COUNT ; ++i)
            in vec2 TexC$(i);
        #endfor
        \endcode
        will be expanded to 
        \code
        in vec2 TexC0;
        in vec2 TexC1;
        in vec2 TexC2;
        in vec2 TexC3;
        \endcode       
        where TEX_CRD_COUNT was defined using the C++ interface with the string "TEX_CRD_COUNT 4".
    */

    class ShaderPreprocessor
    {
    public:
        /** Load a shader from file and pre-process it
            \param[in] filename The shader file to open
            \param[out] shader On success, the parsed shader string.
            \param[out] errorMsg If an error occured, will contain the error message
            \param[in] shaderDefines Optional. A string containing a list of macro definitions to add. Do not put the #define directive, just the macro. Macro definitions are separated by a newline character.
            \return true if parsing was succesful, otherwise false. Call GetErrorString() to get the error message.
        */
        static bool parseShader(const std::string& filename, std::string& shader, std::string& errorMsg, const std::string& shaderDefines = "");

    private:
        ShaderPreprocessor(std::string& errorStr);
        bool parseInternal(const std::string& filename, std::string& shader, std::string& errorMsg, const std::string& shaderDefines = "");

        std::string& mErrorStr;

        using pragma_block_generate_body = bool(*)(const std::string& bodyTemplate, const std::string& linePragma, std::string& body, std::string& error);

        bool addDefines(std::string& shader, const std::string& shaderDefines);
        bool addIncludes(std::string& shader);
        bool parsePragmaBlock(std::string& shader, const std::string& startPragma, const std::string& endPragma, pragma_block_generate_body pfnGenerateBody);
        bool parseExpect(std::string& shader);
        bool addMacroDefinitionToMap(const std::string& defineString);

        std::map<std::string, std::string> mDefineMap;
        std::string mShaderPathAbs;
    };
}
