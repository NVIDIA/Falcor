#ifndef LIB_BAKER_SL_H
#define LIB_BAKER_SL_H

#include "../CoreLib/Basic.h"
#include "../CoreLib/Tokenizer.h"
#include "../SpireCore/ShaderCompiler.h"

namespace SpireLib
{
    class ShaderLibFile : public CoreLib::Basic::Object
    {
    public:
        CoreLib::Basic::EnumerableDictionary<CoreLib::Basic::String, Spire::Compiler::StageSource> Sources; // indexed by world
        Spire::Compiler::ShaderMetaData MetaData;
        void AddSource(CoreLib::Basic::String source, CoreLib::Text::TokenReader & parser);
        void FromString(const CoreLib::String & str);
        CoreLib::String ToString();
        void SaveToFile(CoreLib::Basic::String fileName);
        ShaderLibFile() = default;
        void Clear();
        void Load(CoreLib::Basic::String fileName);
    };
    
#if 0
    CoreLib::Basic::List<ShaderLibFile> CompileShaderSourceFromFile(Spire::Compiler::CompileResult & result,
        const CoreLib::Basic::String & sourceFileName,
        Spire::Compiler::CompileOptions &options);

    CoreLib::Basic::List<ShaderLibFile> CompileShaderSource(Spire::Compiler::CompileResult & result,
        const CoreLib::Basic::String &source, const CoreLib::Basic::String & sourceFileName, Spire::Compiler::CompileOptions &options);
#endif

    class ShaderLib : public ShaderLibFile
    {
    public:
        Spire::Compiler::StageSource GetStageSource(CoreLib::Basic::String world);
        ShaderLib() = default;
        ShaderLib(CoreLib::Basic::String fileName);
        void Reload(CoreLib::Basic::String fileName);
        bool CompileFrom(CoreLib::Basic::String symbolName, CoreLib::Basic::String sourceFileName);
    };


    int executeCompilerDriverActions(Spire::Compiler::CompileOptions const& options);
}

#endif