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
#include "ShaderPreprocessor.h"
#include <sstream>
#include "Utils/OS.h"
#include "Utils/StringUtils.h"
#include <cctype>
#include <set>

#if FALCOR_USE_SPIRE_AS_PREPROCESSOR
#include "Externals/Spire/Spire.h"
#endif

namespace Falcor
{
    size_t npos = std::string::npos;

    bool isInComment(const std::string& str, size_t offset)
    {
        // Check for /* */ style
        size_t prevCommentStart = str.rfind("/*", offset);
        size_t prevCommentEnd = str.rfind("*/", offset);
        if((prevCommentStart > prevCommentEnd) && (prevCommentStart != npos))
        {
            return true;
        }
        
        // Check for '//'
        size_t prevNewLine = str.rfind("\n", offset);
        size_t prevComment = str.rfind("//", offset);
        if((prevComment > prevNewLine) && (prevComment != npos))
        {
            return true;
        }

        return false;
    }

    /*  Valid tokens are:
        +;,=()
        Strings (Whitespaces are ignored)
    */

    size_t getNextToken(const std::string& Str, size_t offset, std::string& token)
    {
        const std::string whitspace = " \t\n\r";
        const std::string charTokens = "+;,=()<";

        token = std::string();

        // Skip whitespaces
        offset = Str.find_first_not_of(whitspace, offset);
        if(offset == npos)
        {
            return npos;
        }

        // See if this is one of the single char tokesn
        if(charTokens.find(Str[offset]) != npos)
        {
            token = Str[offset];
            offset++;
            offset = (offset == Str.length()) ? npos : offset;
            return offset;
        }


        // Find the next whitespace/char token
        size_t endTokenOffset = Str.find_first_of(whitspace + charTokens, offset);
        token = (endTokenOffset != npos) ? Str.substr(offset, endTokenOffset - offset) : Str.substr(offset);
        token = removeTrailingWhitespaces(token);
        return endTokenOffset;
    }

    size_t getLine(const std::string& str, size_t offset, std::string& subStr)
    {
        size_t end = str.find('\n', offset);
        subStr = (end == npos) ? str.substr(offset) : str.substr(offset, end - offset);
        return end;
    }

    size_t getIncludedFileName(const std::string& str, size_t offset, std::string& filename)
    {
        size_t filenameStart = str.find_first_of("<\"\n", offset);
        if(filenameStart == npos || str[filenameStart] == '\n')
        {
            return npos;
        }
        char token = str[filenameStart];
        filenameStart += 1;

        std::string endToken = std::string("\n") + token;
        size_t filenameEnd = str.find_first_of(endToken, filenameStart);
        if(filenameEnd == npos || str[filenameEnd] == '\n')
        {
            return npos;
        }

        size_t length = filenameEnd - filenameStart;
        filename = canonicalizeFilename(str.substr(filenameStart, length));
        return filenameEnd;
    }

    template<bool bReverse>
    size_t findShaderDirective(const std::string& code, size_t offset, const std::string directive)
    {
        while(offset != npos)
        {
            offset = bReverse ? code.rfind(directive, offset) : code.find(directive, offset);
            if(offset != npos)
            {
                if(isInComment(code, offset) == false)
                {
                    return offset;
                }

                if(bReverse)
                {
                    offset -= directive.size();
                }
                else
                {
                    offset += directive.size();

                }
            }
        }
        return offset;
    }

    void findDirectivePair(const std::string& startPragma, const std::string& endPragma, const std::string& str, size_t& startOffset, size_t& endOffset)
    {
        // In case of nesting, this code will return the outer pair
        startOffset = findShaderDirective<false>(str, 0, startPragma);
        if(startOffset == npos)
        {
            // Nothing to do
            endOffset = findShaderDirective<false>(str, 0, endPragma);
            return;
        }

        uint32_t startCount = 1;
        size_t offset = startOffset + startPragma.size();
        while(startCount != 0)
        {
            size_t nextStart = findShaderDirective<false>(str, offset, startPragma);
            endOffset = findShaderDirective<false>(str, offset, endPragma);

            if(endOffset == npos)
            {
                // Didn't find an end pragma
                return;
            }

            if(nextStart == npos || endOffset < nextStart)
            {
                startCount--;
                offset = endOffset + endPragma.size();
            }
            else
            {
                startCount++;
                offset = nextStart + startPragma.size();
            }
        }
    }

    size_t countNewLines(const std::string& str, size_t start, size_t offset)
    {
        const auto& end = (offset == npos) ? str.end() : str.begin() + offset;
        size_t line = std::count(str.begin() + start, end, '\n');
        return line;
    }

    void getLineInformation(const std::string& code, size_t offset, size_t& line, std::string& filename, const std::string& rootFileName)
    {
        // Find the previous line pragma
        const std::string lineDirective("#line");

        size_t precedingLinePragmaOffset = findShaderDirective<true>(code, offset, "#line");
        if(precedingLinePragmaOffset == npos)
        {
            // No preceding line directive; this must be the root file
            filename = rootFileName;
            line = countNewLines(code, 0, offset) + 1; // Count new lines is zero based, so we add 1 for index offset
        }
        else
        {
            // Assuming a well defined line pragma
            // Find the next newline after the preceding line pragma
            size_t endLine = code.find_first_of("\n", precedingLinePragmaOffset);
            std::string pragmaLine = (endLine == npos) ? code.substr(precedingLinePragmaOffset) : code.substr(precedingLinePragmaOffset, endLine - precedingLinePragmaOffset);

            std::vector<std::string> tokens = splitString(pragmaLine, " \t");
            assert(tokens.size() == 2 || tokens.size() == 3);

            // Get the line information
            assert(std::isdigit(tokens[1][0]));
            line = atoi(tokens[1].c_str()) - 1; // #line actually tells where the next line is, so we subtract one to compensate for that
            line += countNewLines(code, precedingLinePragmaOffset, offset);

            if(tokens.size() == 3)
            {
                // Pragma of the form "#line N \"filename\"".
                const auto& f = tokens[2];
                assert(f[0] == '"');
                assert(f[f.length() - 1] == '"');
                filename = f.substr(1, f.length() - 2);
                filename = replaceSubstring(filename,"/","\\");
            }
            else
            {
                // Pragma of the form "#line N", meaning that the file is the root file.
                filename = rootFileName;
            }
        }
    }

    inline std::string getLinePragma(size_t line, const std::string& filename)
    {
        // Note replacing backslashes with forward slashes; otherwise GLSL interprets as escape characters.
        std::string p = std::string("#line ") + std::to_string(line) + " \"" + replaceSubstring(filename,"\\","/") + "\"\n";
        return p;
    }

    inline std::string getLinePragmaFromOffset(const std::string& code, size_t offset, const std::string& rootFileName)
    {
        size_t line;
        std::string filename;
        getLineInformation(code, offset, line, filename, rootFileName);
        return getLinePragma(line, filename);
    }

    static bool endsWith(const std::string& str, const char* suffix)
    {
        size_t strLen = str.length();
        size_t suffixLen = strlen(suffix);
        if (strLen < suffixLen) return false;
        return strcmp(str.c_str() + (strLen - suffixLen), suffix) == 0;
    }

    static bool checkSpireErrors(
        SpireDiagnosticSink* spireSink,
        std::string& outDiagnostics)
    {
        int diagnosticsSize = spGetDiagnosticOutput(spireSink, NULL, 0);
        if (diagnosticsSize != 0)
        {
            char* diagnostics = (char*)malloc(diagnosticsSize);
            spGetDiagnosticOutput(spireSink, diagnostics, diagnosticsSize);
            outDiagnostics += diagnostics;
            free(diagnostics);
        }

        return spDiagnosticSinkHasAnyErrors(spireSink) != 0;
    }

    static bool translateSpireToTargetShadingLanguage(
        const std::string& path,
        const std::string& spireContent,
        std::string& outTranslatedContent,
        std::string& outDiagnostics)
    {
        // TODO: actually invoke Spire compiler here

        SpireCompilationContext* spireContext = spCreateCompilationContext(NULL);

        SpireDiagnosticSink* spireSink = spCreateDiagnosticSink(spireContext);

#if defined(FALCOR_GL)
        spSetCodeGenTarget(spireContext, SPIRE_GLSL);
#elif defined(FALCOR_D3D11) || defined(FALCOR_D3D12)
        spSetCodeGenTarget(spireContext, SPIRE_HLSL);
#else
#error unknown shader compilation target
#endif

        spLoadModuleLibraryFromSource(spireContext, spireContent.c_str(), path.c_str(), spireSink);
        if (checkSpireErrors(spireSink, outDiagnostics))
        {
            spDestroyDiagnosticSink(spireSink);
            spDestroyCompilationContext(spireContext);
            return false;
        }

        SpireCompilationResult* result = spCompileShaderFromSource(spireContext, "", "", spireSink);
        if (checkSpireErrors(spireSink, outDiagnostics))
        {
            spDestroyDiagnosticSink(spireSink);
            spDestroyCompilationContext(spireContext);
            return false;
        }

        outTranslatedContent = spGetShaderStageSource(result, NULL, NULL, NULL);

        spDestroyCompilationResult(result);
        spDestroyDiagnosticSink(spireSink);
        spDestroyCompilationContext(spireContext);

        return true;
    }

    bool ShaderPreprocessor::addIncludes(std::string& code, Shader::unordered_string_set& includeFileList)
    {
        auto getDirAbs = [](const std::string& path) -> std::string 
        {
            auto last = path.find_last_of("/\\");
            return path.substr(0, last);
        };

        // Map of a file's absolute path onto the absolute directory that contains it.
        std::map<std::string,std::string> pathsAbsToDirsAbs;
        pathsAbsToDirsAbs[mShaderPathAbs] = getDirAbs(mShaderPathAbs);

        // Set of all included files' absolute paths
        std::set<std::string> includedPathsAbs;

        // Loop over every found include in the file
        const std::string includeMacro = "#include";
        const std::string pragmaStatement = "#pragma once";
        while(true) 
        {
            size_t offset = findShaderDirective<false>(code, 0, includeMacro);
            if(offset == npos) 
            {
                break; //Completed
            }

            // Get absolute path to the including file.  Note that it is absolute because we only ever write absolute paths in #line directives.
            std::string includingPathAbs;
            size_t line;
            getLineInformation(code, offset, line, includingPathAbs, mShaderPathAbs);

            // Get raw path to the include (may be relative to the including file or absolute)
            std::string includedPathRaw;
            size_t postFilenameOffset = getIncludedFileName(code, offset, includedPathRaw);
            if(postFilenameOffset == npos)
            {
                mErrorStr += mShaderPathAbs + "(" + std::to_string(line) + "):Missing included filename";
                return false;
            }

            // Resolve absolute path of included file.  Error if cannot be found.
            std::string includedPathAbs;
            if(doesFileExist(includedPathRaw))
            {
                // Path was absolute.
                includedPathAbs = includedPathRaw;
            }
            else
            {
                // Path is relative to including file, and may include Falcor builtins.

                // Search for Falcor builtin
                if(findFileInDataDirectories(includedPathRaw, includedPathAbs)) goto SUCCESS;

                // Search relative to the including file.
                // Note canonicalization is necessary because the relative path might contain "..\\".
                includedPathAbs = canonicalizeFilename( pathsAbsToDirsAbs.at(includingPathAbs) + "\\" + includedPathRaw );
                if(doesFileExist(includedPathAbs)) goto SUCCESS;

                // Could not find file!
                mErrorStr += includingPathAbs + "(" + std::to_string(line) + "):Cannot find apparent relative include file \"" + includedPathRaw + "\".";
                return false;

                SUCCESS:;
            }

            // Add the file to the include list
            includeFileList.insert(includedPathAbs);

            // Read the included file.
            std::string includedContent;
            readFileToString(includedPathAbs, includedContent);

            // Add a trailing newline as required.  TODO: emit a warning while doing this.
            if(!includedContent.empty() && includedContent.back() != '\n')
            {
                includedContent += '\n';
            }

            // If the included file contains "#pragma once", and we already included it, ignore it.  TODO: need to check that the pragma is valid.
            bool shouldInclude = true;
            if(findShaderDirective<false>(includedContent, 0, pragmaStatement) != npos)
            {
                if(includedPathsAbs.find(includedPathAbs) != includedPathsAbs.end())
                {
                    shouldInclude = false;
                }
            }

            std::string prologue = code.substr(0, offset);
            std::string epilogue = code.substr(postFilenameOffset + 2);
            if(shouldInclude)
            {
                includedPathsAbs.insert(includedPathAbs);
                pathsAbsToDirsAbs[includedPathAbs] = getDirAbs(includedPathAbs);

                std::string preIncludeLine = getLinePragma(1, includedPathAbs);
                std::string replacedIncludeLine = getLinePragma(line+1, includingPathAbs);
                code = prologue + preIncludeLine + includedContent + replacedIncludeLine + epilogue;
            }
            else
            {
                code = prologue + epilogue;
            }

        }

        return true;
    }

    using string_tuple = std::vector < std::string >;
    using string_tuple_vector = std::vector < string_tuple >;

    bool getStringTuples(const std::string& line, size_t& offset, const std::string& endToken, string_tuple& stringVec, std::string& errorStr, bool shouldBeginWithParenthesis)
    {
        // Read the keys. The first key might contain a '(', so check that
        std::string token;
        bool hasParanthesis = false;
        offset = getNextToken(line, offset, token);
        if(token == "(")
        {
            hasParanthesis = true;
            if(offset == npos)
            {
                errorStr = "No values found after '('";
                return false;
            }
            offset = getNextToken(line, offset, token);
        }
        else if(shouldBeginWithParenthesis)
        {
            errorStr = "tuple should start with a '('.";
            return false;
        }

        while(true)
        {
            if(token == "(")
            {
                errorStr = "Unexpected '('.";
                return false;
            }

            // Insert the token
            stringVec.push_back(token);

            // Check if this is the end of the tuple
            offset = getNextToken(line, offset, token);

            if(token == ")")
            {
                if(hasParanthesis == false)
                {
                    errorStr = "Unexpected ')' when parsing tuple.";
                    return false;
                }
                // Reached the end of the tuple
                hasParanthesis = false;
                break;
            }

            if(token == endToken || offset == npos)
            {
                if(token == endToken)
                {
                    // Correct the offset
                    offset = (offset == npos) ? (line.length() - endToken.length()) : offset - endToken.length();
                }
                break;
            }


            // Otherwise, we must have a ,
            if(token != ",")
            {
                errorStr = "Expecting ',' after symbol.";
                return false;
            }

            // Skip the ','
            offset = getNextToken(line, offset, token);
        }

        if(hasParanthesis)
        {
            errorStr = "Missing ')' at the end of a tuple";
            return false;
        }
        return true;
    }

    bool parseForEachLine(const std::string& line, string_tuple& keyTable, string_tuple_vector& valueTable, std::string& errorStr)
    {
        // The line looks like
        // #foreach key1 in value1, value2, ..., valuen
        // #foreach key1, key2, ..., keyn in (value1, value2, ..., valuen), (value1, value2, ..., valuen), ..., n tuples
        // #foreach (key1, key2, ..., keyn) in (value1, value2, ..., valuen), (value1, value2, ..., valuen), ... n tuples

        std::string token;
        size_t offset = getNextToken(line, 0, token);
        assert(token == "#foreach");

        std::string cleanLine = removeLeadingTrailingWhitespaces(line);
        if(getStringTuples(cleanLine, offset, "in", keyTable, errorStr, false) == false)
        {
            errorStr = "Error when parsing key list in #foreach pragma. " + errorStr;
            return false;
        }

        if(keyTable.size() == 0)
        {
            errorStr = "No keys found";
            return false;
        }

        // Next token must be 'in'
        std::string in;
        offset = getNextToken(cleanLine, offset, in);
        if(in != "in")
        {
            errorStr = "missing 'in' statement";
            return false;
        }

        // Support the option when there are no values
        if(offset == npos)
        {
            valueTable.clear();
            return true;
        }

        // If there's only one key, we don't really have tuples. Just read the values one by one
        if(keyTable.size() == 1)
        {
            string_tuple val;
            if(getStringTuples(cleanLine, offset, std::string(), val, errorStr, false) == false)
            {
                return false;
            }
            for(const auto& a : val)
            {
                string_tuple v;
                v.push_back(a);
                valueTable.push_back(v);
            }
        }
        else
        {
            while(true)
            {
                string_tuple val;
                if(getStringTuples(cleanLine, offset, std::string(), val, errorStr, true) == false)
                {
                    errorStr = "Error when parsing value tuple in #foraeach pragma. " + errorStr;
                    return false;
                }
                if(val.size() != keyTable.size())
                {
                    errorStr = "Value list size (" + std::to_string(val.size()) + ") is different than key list size (" + std::to_string(keyTable.size()) + ").";
                    return false;
                }
                valueTable.push_back(val);

                offset = getNextToken(cleanLine, offset, token);
                if(offset == npos)
                {
                    if(token != "")
                    {
                        errorStr = "Unexpected '" + token + "' at end of #foreach line.";
                        return false;
                    }
                    break;
                }
                if(token != ",")
                {
                    errorStr = "Value tuples should be separated by a ','.";
                    return false;
                }
            }
        }

        return true;
    }

    bool parseForLine(const std::string& forLine, std::string& iteratorName, int32_t& startRange, int32_t& endRange, int32_t& delta, std::string& errorMsg)
    {
        // Currently, must match exactly:
        // #for (int arg = 0; arg < MacroOrIntLiteral; ++arg)
        // where only arg and MacroOrIntLiteral can be changed. The rest of the line must look exactly the same

        std::string token;
        size_t offset = getNextToken(forLine, 0, token);
        assert(token == "#for");

        offset = getNextToken(forLine, offset, token);
        if(token != "(")
        {
            errorMsg = "Expecting '(' after #for";
            return false;
        }

        offset = getNextToken(forLine, offset, token);
        if(token != "int")
        {
            errorMsg = "#for directive only support int variables";
            return false;
        }

        // Get the iterator name
        offset = getNextToken(forLine, offset, iteratorName);

        offset = getNextToken(forLine, offset, token);
        if(token != "=")
        {
            errorMsg = "missing variable initialization in #for directive.";
            return false;
        }

        // Get the start value
        offset = getNextToken(forLine, offset, token);
        char* pNumEnd;
        startRange = strtol(token.c_str(), &pNumEnd, 0);
        if(*pNumEnd != 0)
        {
            errorMsg = "#for variable initializer must be an integer number (or a macro which expands to an integer number)";
            return false;
        }

        offset = getNextToken(forLine, offset, token);
        if(token != ";")
        {
            errorMsg = "Missing ';' in #for loop";
            return false;
        }

        offset = getNextToken(forLine, offset, token);
        if(token != iteratorName)
        {
            errorMsg = "conditional expression must use " + iteratorName;
            return false;
        }

        offset = getNextToken(forLine, offset, token);
        if(token != "<")
        {
            errorMsg = "#for loop only supports '<' conditional operator";
            return false;
        }

        // Get the range end
        offset = getNextToken(forLine, offset, token);
        endRange = strtol(token.c_str(), &pNumEnd, 0);
        if(*pNumEnd != 0)
        {
            errorMsg = "#for conditional R-value must be an integer number (or a macro which expands to an integer number)";
            return false;
        }

        offset = getNextToken(forLine, offset, token);
        if(token != ";")
        {
            errorMsg = "Missing ';' in #for loop";
            return false;
        }

        std::string token2;
        offset = getNextToken(forLine, offset, token);
        offset = getNextToken(forLine, offset, token2);
        if(token + token2 != "++")
        {
            errorMsg = "#for loop only support prefix increment operator (++arg).";
            return false;
        }

        delta = 1;

        offset = getNextToken(forLine, offset, token);
        if(token != iteratorName)
        {
            errorMsg = "Loop expression must operate on " + iteratorName;
            return false;
        }

        offset = getNextToken(forLine, offset, token);
        if(token != ")")
        {
            errorMsg = "Expecting ')' after conditional expression";
            return false;
        }

        offset = getNextToken(forLine, offset, token);
        if(offset != npos || token.size() != 0)
        {
            errorMsg = "Unexpected end of #for line. Found " + token;
            return false;
        }

        return true;
    }

    bool generateForEachBody(const std::string& bodyTemplate, const std::string& foreachLine, std::string& body, std::string& error)
    {
        string_tuple keyTable;
        string_tuple_vector valueTable;
        if(parseForEachLine(foreachLine, keyTable, valueTable, error) == false)
        {
            return false;
        }

        const std::string valueIndex = "$(_valIndex)";
        const std::string keyIndex = "$(_keyIndex)";

        for(size_t value = 0; value < valueTable.size(); value++)
        {
            const auto& valueList = valueTable[value];
            std::string valBody = bodyTemplate;
            for(size_t key = 0; key < keyTable.size(); key++)
            {
                const std::string& decoratedKey = "$(" + keyTable[key] + ")";
                if(decoratedKey == valueIndex || decoratedKey == keyIndex)
                {
                    error = "Key '" + keyTable[key] + "' is reserved for the " + ((decoratedKey == valueIndex) ? "value" : "key") + " index.";
                    return false;
                }

                const std::string& V = valueList[key];
                valBody = replaceSubstring(valBody, decoratedKey, V);
                valBody = replaceSubstring(valBody, keyIndex, std::to_string(key));
            }
            valBody = replaceSubstring(valBody, valueIndex, std::to_string(value));

            body += valBody;
        }

        return true;
    }

    bool generateForLoopBody(const std::string& bodyTemplate, const std::string& forLine, std::string& body, std::string& error)
    {
        std::string iteratorName;
        int32_t startRange = 0;
        int32_t endRange = 0;
        int32_t delta = 0;

        if(parseForLine(forLine, iteratorName, startRange, endRange, delta, error) == false)
        {
            return false;
        }

        iteratorName = "$(" + iteratorName + ")";
        for(int32_t i = startRange; i < endRange; i += delta)
        {
            body += replaceSubstring(bodyTemplate, iteratorName, std::to_string(i));
        }

        return true;
    }

    std::string expandMacros(const std::string& line, const std::map<std::string, std::string>& defines)
    {
        std::string S = line;
        std::string token;
        size_t offset = 0;

        while(offset != npos)
        {
            offset = getNextToken(S, offset, token);
            const auto& def = defines.find(token);

            if(def != defines.end())
            {
                const std::string& value = def->second;
                std::string start, end;

                // Changing the offset to make sure we continue parsing from the place we inserted the code. This handles cases where one macro was using another one.
                if(offset == npos)
                {
                    offset = S.length() - token.size();
                    start = S.substr(0, offset);

                }
                else
                {
                    end = S.substr(offset);
                    offset = offset - token.size();
                    start = S.substr(0, offset);

                }
                S = start + ' ' + def->second + end;
            }
        }

        return S;
    }

    bool ShaderPreprocessor::parsePragmaBlock(std::string& shader, const std::string& startDirective, const std::string& endDirective, pragma_block_generate_body pfnGenerateBody)
    {
        size_t startDirectiveOffset, endDirectiveOffset;
        findDirectivePair(startDirective, endDirective, shader, startDirectiveOffset, endDirectiveOffset);

        while(startDirectiveOffset != npos)
        {
            // Error checks
            if(endDirectiveOffset == npos)
            {
                size_t line;
                std::string filename;
                getLineInformation(shader, startDirectiveOffset, line, filename, mShaderPathAbs);
                mErrorStr += filename + "(" + std::to_string(line) + "): Found " + startDirective + " directive with no matching " + endDirective + ".";
                return false;
            }

            // Get the for start pragma line
            std::string startDirectiveLine;
            size_t startDirectiveLineOffset = getLine(shader, startDirectiveOffset, startDirectiveLine);

            // expand macro definitions
            startDirectiveLine = expandMacros(startDirectiveLine, mDefineMap);


            // Generate the block body
            std::string bodyTemplate = getLinePragmaFromOffset(shader, startDirectiveLineOffset, mShaderPathAbs) + shader.substr(startDirectiveLineOffset, endDirectiveOffset - startDirectiveLineOffset);
            std::string body;
            if(pfnGenerateBody(bodyTemplate, startDirectiveLine, body, mErrorStr) == false)
            {
                size_t line;
                std::string filename;
                getLineInformation(shader, startDirectiveOffset, line, filename, mShaderPathAbs);
                mErrorStr = filename + "(" + std::to_string(line) + "): " + mErrorStr;
                return false;
            }


            // Find all the pieces of the puzzle
            std::string prolog = shader.substr(0, startDirectiveOffset);
            std::string epilogue;
            size_t endOfEndOffset = shader.find('\n', endDirectiveOffset);
            if(endOfEndOffset != npos)
            {
                epilogue = getLinePragmaFromOffset(shader, endOfEndOffset, mShaderPathAbs) + shader.substr(endOfEndOffset);
            }

            shader = prolog + body + epilogue;

            // Get the next directive
            findDirectivePair(startDirective, endDirective, shader, startDirectiveOffset, endDirectiveOffset);
        }

        // Error checks
        if(endDirectiveOffset != npos)
        {
            size_t line;
            std::string filename;
            getLineInformation(shader, endDirectiveOffset, line, filename, mShaderPathAbs);
            mErrorStr += filename + "(" + std::to_string(line) + "): Found " + endDirective + " directive with no matching " + startDirective + ".";
            return false;
        }
        return true;
    }

    bool ShaderPreprocessor::parseExpect(std::string& shader)
    {
        const std::string expect("#expect");
        size_t expectOffset = findShaderDirective<false>(shader, 0, expect);

        while(expectOffset != npos)
        {
            // Store the current line information for error messages
            size_t line;
            std::string file;
            getLineInformation(shader, expectOffset, line, file, mShaderPathAbs);

            // Get the expect line
            std::string expectLine;
            size_t endLine = getLine(shader, expectOffset + expect.size(), expectLine);
            expectLine = removeLeadingTrailingWhitespaces(expectLine);

            // Get the macro
            std::string macro;
            size_t macroOffset = getNextToken(expectLine, 0, macro);

            if(macro.size() == 0)
            {
                mErrorStr += file + "(" + std::to_string(line) + "): Incorrect #expect syntax. Should be '#expect <macro name> <optional macro description>'";
                return false;
            }

            // Check if there is a description
            std::string desc = expectLine.substr(macro.size()); 
            desc = removeLeadingWhitespaces(desc);

            const auto& def = mDefineMap.find(macro);
            if(def == mDefineMap.end())
            {
                mErrorStr += file + "(" + std::to_string(line) + "): Expected " + macro + " macro definition. " + desc;
                return false;
            }

            // Get line directives so that the error will appear in the correct location
            std::string linePragma = getLinePragma(line, file);

            std::string prolog = shader.substr(0, expectOffset);
            std::string epilogue = (endLine == npos) ? std::string() : shader.substr(endLine);

            shader = prolog + linePragma + epilogue;

            // Get the next directive
            expectOffset = findShaderDirective<false>(shader, endLine, expect);
        }

        return true;
    }

    bool ShaderPreprocessor::addDefines(std::string& code, const Program::DefineList& shaderDefines)
    {
        // Adding the defines right after the version string
        size_t verStart = 0;
        size_t verEnd = 0;

        verStart = code.find("#version");
        if(verStart == npos)
        {
#ifdef FALCOR_GL
            mErrorStr += "Can't find version directive\n";
            return false;
#endif
            verStart = 0;
        }
        else
        {
            // Skip the version pragma line
            verEnd = code.find("\n", verStart);
            if(verEnd == npos)
            {
                code += "\n";
                verEnd = code.length() - 1;
            }
            verEnd++;
        }
        // Store the current line number
        size_t line;
        std::string currentFile;
        getLineInformation(code, verStart, line, currentFile, mShaderPathAbs);


#ifdef FALCOR_D3D
        static const std::string api = "FALCOR_HLSL";
        static const std::string extensions;
#elif defined FALCOR_GL
        static const std::string api = "FALCOR_GLSL";
        static const std::string extensions("#extension GL_ARB_bindless_texture : enable");
#endif

        std::string allDefines = "#ifndef " + api + "\n#define " + api + "\n#endif\n" + extensions + "\n";

        for(const auto& defDcl : shaderDefines)
        {
            std::string def = defDcl.first;
            if(defDcl.second.size())
            {
                def += ' ' + defDcl.second;
            }
            addMacroDefinitionToMap(def);
            allDefines += "#define " + def + "\n";;
        }

        // Patch the code
        allDefines += getLinePragma(verEnd == 0 ? 0 : line, currentFile) + "\n";
        code.insert(verEnd, allDefines);

#ifdef FALCOR_D3D
        if(verEnd != verStart)
        {
            // Remove the version pragma
            code.erase(verStart, verEnd - verStart);
        }
#endif
        return true;
    }

    bool ShaderPreprocessor::addMacroDefinitionToMap(const std::string& defineString)
    {
        // String is without the #define directive
        std::string define = removeLeadingTrailingWhitespaces(defineString);

        // Get the macro name
        std::string macroName;
        size_t defineOffset = getNextToken(define, 0, macroName);
        assert(macroName.size());

        // Make sure no macro-redefinition
        if(mDefineMap.find(macroName) != mDefineMap.end())
        {
            logError(mShaderPathAbs + ":\"" + macroName + "\" Macro redefinition.");
            return false;
        }

        // Skip the macro
        std::string value = define.substr(macroName.size());

        // Check if there is a definition
        value = removeLeadingWhitespaces(value);
        mDefineMap[macroName] = value;

        return true;
    }

    ShaderPreprocessor::ShaderPreprocessor(std::string& errorStr) : mErrorStr(errorStr)
    {
        mErrorStr.clear();
        mDefineMap.clear();
    }

#if FALCOR_USE_SPIRE_AS_PREPROCESSOR
    static bool checkSpireErrors(
        SpireDiagnosticSink* spireSink,
        std::string& outDiagnostics)
    {
        int diagnosticsSize = spGetDiagnosticOutput(spireSink, NULL, 0);
        if (diagnosticsSize != 0)
        {
            char* diagnostics = (char*)malloc(diagnosticsSize);
            spGetDiagnosticOutput(spireSink, diagnostics, diagnosticsSize);
            outDiagnostics += diagnostics;
            free(diagnostics);
        }

        return spDiagnosticSinkHasAnyErrors(spireSink) != 0;
    }
#endif


    bool ShaderPreprocessor::parseShader(const std::string& filename, std::string& shader, std::string& errorMsg, Shader::unordered_string_set& includeFileList, const Program::DefineList& shaderDefines)
    {
#if FALCOR_USE_SPIRE_AS_PREPROCESSOR
        SpireCompilationContext* spireContext = spCreateCompilationContext(NULL);

        // TODO: Spire should really support a callback API for all file I/O,
        // rather than having us specify data directories to it...
        for (auto path : getDataDirectoriesList())
        {
            spAddSearchPath(spireContext, path.c_str());
        }

        spAddPreprocessorDefine(spireContext, "__SPIRE__", "1");

        for(auto shaderDefine : shaderDefines)
        {
            spAddPreprocessorDefine(spireContext, shaderDefine.first.c_str(), shaderDefine.second.c_str());
        }

        spAddPreprocessorDefine(spireContext, "FALCOR_HLSL", "1");

        SpireDiagnosticSink* spireSink = spCreateDiagnosticSink(spireContext);

#if defined(FALCOR_GL)
        spSetCodeGenTarget(spireContext, SPIRE_GLSL);
#elif defined(FALCOR_D3D11)
        spSetCodeGenTarget(spireContext, SPIRE_HLSL);
#elif defined(FALCOR_D3D12)
        spSetCodeGenTarget(spireContext, SPIRE_HLSL);
#else
#error unknown shader compilation target
#endif

        SpireCompilationResult* result = spCompileShaderFromSource(spireContext, shader.c_str(), filename.c_str(), spireSink);
        if (checkSpireErrors(spireSink, errorMsg))
        {
            spDestroyDiagnosticSink(spireSink);
            spDestroyCompilationContext(spireContext);
            return false;
        }

        shader = spGetShaderStageSource(result, NULL, NULL, NULL);

        spDestroyCompilationResult(result);
        spDestroyDiagnosticSink(spireSink);
        spDestroyCompilationContext(spireContext);

        return true;

#else

        ShaderPreprocessor preProc(errorMsg);

        preProc.mShaderPathAbs = canonicalizeFilename(filename);

        // First, add include files as the rest of the directive might rely on their content
        if(preProc.addIncludes(shader, includeFileList) &&
            preProc.addDefines(shader, shaderDefines) &&
            preProc.parseExpect(shader) &&
            preProc.parsePragmaBlock(shader, "#foreach", "#endforeach", generateForEachBody) &&
            preProc.parsePragmaBlock(shader, "#for", "#endfor", generateForLoopBody))
        {
            return true;
        }
        return false;
#endif
    }
}
