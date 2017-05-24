#include "SpireLib.h"
#include "../CoreLib/LibIO.h"
#include "../CoreLib/Tokenizer.h"
#include "../SpireCore/StdInclude.h"
#include "../../Spire.h"
#include "../SpireCore/Parser.h"
#include "../SpireCore/Preprocessor.h"
#include "../SpireCore/Reflection.h"
#include "../SpireCore/TypeLayout.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX
#endif

using namespace CoreLib::Basic;
using namespace CoreLib::IO;
using namespace CoreLib::Text;
using namespace Spire::Compiler;

namespace SpireLib
{
    void ReadSource(EnumerableDictionary<String, StageSource> & sources, CoreLib::Text::TokenReader & parser, String src)
    {
        auto getShaderSource = [&]()
        {
            auto token = parser.ReadToken();
            int endPos = token.Position.Pos + 1;
            int brace = 0;
            while (endPos < src.Length() && !(src[endPos] == '}' && brace == 0))
            {
                if (src[endPos] == '{')
                    brace++;
                else if (src[endPos] == '}')
                    brace--;
                endPos++;
            }
            while (!parser.IsEnd() && parser.NextToken().Position.Pos != endPos)
                parser.ReadToken();
            parser.ReadToken();
            return src.SubString(token.Position.Pos + 1, endPos - token.Position.Pos - 1);
        };
        while (!parser.IsEnd() && !parser.LookAhead("}"))
        {
            auto worldName = parser.ReadWord();
            StageSource compiledSrc;
            if (parser.LookAhead("binary"))
            {
                parser.ReadToken();
                parser.Read("{");
                while (!parser.LookAhead("}") && !parser.IsEnd())
                {
                    auto val = parser.ReadUInt();
                    compiledSrc.BinaryCode.AddRange((unsigned char*)&val, sizeof(unsigned int));
                    if (parser.LookAhead(","))
                        parser.ReadToken();
                }
                parser.Read("}");
            }
            if (parser.LookAhead("text"))
            {
                parser.ReadToken();
                compiledSrc.MainCode = getShaderSource();
            }
            sources[worldName] = compiledSrc;
        }
    }
    StageSource ShaderLib::GetStageSource(String stage)
    {
        StageSource rs;
        Sources.TryGetValue(stage, rs);
        return rs;
    }
    ShaderLib::ShaderLib(CoreLib::Basic::String fileName)
    {
        Reload(fileName);
    }
    void ShaderLib::Reload(CoreLib::Basic::String fileName)
    {
        Load(fileName);
    }
#if 0
    bool ShaderLib::CompileFrom(String symbolName, String sourceFileName)
    {
        Spire::Compiler::CompileResult result;
        CompileOptions options;
        options.SymbolToCompile = symbolName;
        options.Mode = CompilerMode::ProduceShader;
        auto shaderLibs = CompileShaderSourceFromFile(result, sourceFileName, options);
        if (result.GetErrorCount() == 0)
        {
            for (auto & lib : shaderLibs)
            {
                if (lib.MetaData.ShaderName == symbolName)
                {
                    FromString(shaderLibs[0].ToString());
                    return true;
                }
            }
        }
        result.PrintDiagnostics();
        return false;
    }
#endif

#if 0
    List<ShaderLibFile> CompileUnits(Spire::Compiler::CompileResult & compileResult,
        ShaderCompiler * compiler, List<CompileUnit> & units,
        Spire::Compiler::CompileOptions & options)
    {
        List<ShaderLibFile> resultFiles;
        compiler->Compile(compileResult, units, options);
        if (compileResult.GetErrorCount() == 0)
        {
            if (options.Mode == CompilerMode::ProduceShader)
            {
                EnumerableDictionary<String, ShaderLibFile> shaderLibs;
                for (auto file : compileResult.CompiledSource)
                {
                    ShaderLibFile libFile;
                    libFile.MetaData = file.Value.MetaData;
                    libFile.Sources = file.Value.Stages;
                    resultFiles.Add(libFile);
                }
            }
        }
        return resultFiles;
    }
#endif

#if 0
    List<ShaderLibFile> CompileShaderSource(Spire::Compiler::CompileResult & compileResult,
        const CoreLib::String & src, const CoreLib::String & fileName, Spire::Compiler::CompileOptions & options)
    {
        IncludeHandlerImpl includeHandler;
        includeHandler.searchDirs = options.SearchDirectories;

        Spire::Compiler::NamingCounter = 0;
        RefPtr<ShaderCompiler> compiler = CreateShaderCompiler();
        List<CompileUnit> units;
        HashSet<String> processedUnits;
        List<String> unitsToInclude;
        unitsToInclude.Add(fileName);
        processedUnits.Add(fileName);
        auto searchDirs = options.SearchDirectories;
        searchDirs.Add(Path::GetDirectoryName(fileName));
        searchDirs.Reverse();


#if 0
        // If we are being asked to do pass-through, then we need to do that here...
        if (options.passThrough != PassThroughMode::None)
        {
            compiler->PassThrough(compileResult, src, fileName, options);
            return List<ShaderLibFile>();
        }
#endif



        CompileUnit predefUnit;
        predefUnit = compiler->Parse(
            options,
            compileResult, SpireStdLib::GetCode(), "stdlib", &includeHandler, options.PreprocessorDefinitions, predefUnit);

        // Add this one first, so that it gets processed first...
        units.Add(predefUnit);


        for (int i = 0; i < unitsToInclude.Count(); i++)
        {
            auto inputFileName = unitsToInclude[i];
            try
            {
                String source = src;
                if (i > 0)
                    source = File::ReadAllText(inputFileName);
                auto unit = compiler->Parse(
                    options,
                    compileResult, source, inputFileName, &includeHandler, options.PreprocessorDefinitions, predefUnit);
                units.Add(unit);
                if (unit.SyntaxNode)
                {
                    for (auto inc : unit.SyntaxNode->GetUsings())
                    {
                        bool found = false;
                        for (auto & dir : searchDirs)
                        {
                            String includeFile = Path::Combine(dir, inc->fileName.Content);
                            if (File::Exists(includeFile))
                            {
                                if (processedUnits.Add(includeFile))
                                {
                                    unitsToInclude.Add(includeFile);
                                }
                                found = true;
                                break;
                            }
                        }
                        if (!found)
                        {
                            compileResult.GetErrorWriter()->diagnose(inc->fileName.Position, Diagnostics::cannotFindFile, inc->fileName);
                        }
                    }
                }
            }
            catch (IOException)
            {
                compileResult.GetErrorWriter()->diagnose(CodePosition(0, 0, 0, ""), Diagnostics::cannotOpenFile, inputFileName);
            }
        }
        if (compileResult.GetErrorCount() == 0)
            return CompileUnits(compileResult, compiler.Ptr(), units, options);
        else
            return List<ShaderLibFile>();
    }

    List<ShaderLibFile> CompileShaderSourceFromFile(Spire::Compiler::CompileResult & compileResult,
        const CoreLib::Basic::String & sourceFileName,
        Spire::Compiler::CompileOptions & options)
    {
        try
        {
            return CompileShaderSource(compileResult, File::ReadAllText(sourceFileName), sourceFileName, options);
        }
        catch (IOException)
        {
            compileResult.GetErrorWriter()->diagnose(CodePosition(0, 0, 0, ""), Diagnostics::cannotOpenFile, Path::GetFileName(sourceFileName));
        }
        return List<ShaderLibFile>();
    }
#endif

    void ShaderLibFile::AddSource(CoreLib::Basic::String source, CoreLib::Text::TokenReader & parser)
    {
        ReadSource(Sources, parser, source);
    }

    CoreLib::String ShaderLibFile::ToString()
    {
        StringBuilder writer;
        writer << "name " << MetaData.ShaderName << EndLine;
        for (auto & ublock : MetaData.ParameterSets)
        {
            writer << "paramset \"" << ublock.Key << "\" size " << ublock.Value->BufferSize
                << " binding " << ublock.Value->DescriptorSetId << "\n{\n";
            for (auto & entry : ublock.Value->Parameters)
            {
                writer << entry.Value->Name << "(\"" << entry.Key << "\") : ";
                entry.Value->Type->Serialize(writer);
                writer << " at ";
                if (entry.Value->BufferOffset == -1)
                {
                    writer << "binding(";
                    for (auto binding : entry.Value->BindingPoints)
                        writer << binding << " ";
                    writer << ")";
                }
                else
                {
                    writer << "buffer(" << entry.Value->BufferOffset << ", "
                        << (int)GetTypeSize(entry.Value->Type.Ptr(), LayoutRule::Std140) << ")";
                }
                writer << ";\n";
            }
            writer << "}\n";
        }
        writer << "source" << EndLine << "{" << EndLine;
        for (auto & src : Sources)
        {
            writer << src.Key << EndLine;
            if (src.Value.BinaryCode.Count())
            {
                writer << "binary" << EndLine << "{" << EndLine;
                auto binaryBuffer = (unsigned int*)src.Value.BinaryCode.Buffer();
                for (int i = 0; i < src.Value.BinaryCode.Count() / 4; i++)
                {
                    writer << String((long long)binaryBuffer[i]) << ",";
                    if ((i + 1) % 10)
                        writer << EndLine;
                }
                writer << EndLine << "}" << EndLine;
            }
            writer << "text" << EndLine << "{" << EndLine;
            writer << src.Value.MainCode << EndLine;

            writer << "}" << EndLine;
        }
        writer << "}" << EndLine;
        StringBuilder formatSB;
        IndentString(formatSB, writer.ProduceString());
        return formatSB.ProduceString();
    }

    void ShaderLibFile::Clear()
    {
        Sources.Clear();
        MetaData.ParameterSets.Clear();
        Sources.Clear();
    }

    void ShaderLibFile::SaveToFile(CoreLib::Basic::String fileName)
    {
        StreamWriter fwriter(fileName);
        fwriter.Write(ToString());
    }

    void ShaderLibFile::FromString(const String & src)
    {
        Clear();
        CoreLib::Text::TokenReader parser(src);
        while (!parser.IsEnd())
        {
            auto fieldName = parser.ReadWord();
            if (fieldName == "name")
            {
                MetaData.ShaderName = parser.ReadWord();
            }
            else if (fieldName == "source")
            {
                parser.Read("{");
                ReadSource(Sources, parser, src);
                parser.Read("}");
            }
            else if (fieldName == "paramset")
            {
                RefPtr<ILModuleParameterSet> paramSet = new ILModuleParameterSet();
                paramSet->BindingName = parser.ReadStringLiteral();
                if (parser.LookAhead("size"))
                {
                    parser.ReadToken();
                    paramSet->BufferSize = parser.ReadInt();
                }
                if (parser.LookAhead("binding"))
                {
                    parser.ReadToken();
                    paramSet->DescriptorSetId = parser.ReadInt();
                }
                parser.Read("{");
                while (!parser.LookAhead("}"))
                {
                    RefPtr<ILModuleParameterInstance> inst = new ILModuleParameterInstance();
                    inst->Name = parser.ReadWord();
                    parser.Read("(");
                    auto key = parser.ReadStringLiteral();
                    parser.Read(")");
                    inst->Type = ILType::Deserialize(parser);
                    parser.Read("at");
                    if (parser.LookAhead("binding"))
                    {
                        parser.ReadToken();
                        parser.Read("(");
                        while (!parser.LookAhead(")"))
                            inst->BindingPoints.Add(parser.ReadInt());
                        parser.Read(")");
                    }
                    else
                    {
                        parser.Read("buffer");
                        parser.Read("(");
                        inst->BufferOffset = parser.ReadInt();
                        parser.Read(")");
                    }
                    paramSet->Parameters.Add(key, inst);
                }
                parser.Read("}");
                MetaData.ParameterSets.Add(paramSet->BindingName, paramSet);
            }
        }
    }

    void ShaderLibFile::Load(String fileName)
    {
        String src = File::ReadAllText(fileName);
        FromString(src);
    }


    class Shader
    {
        friend class CompilationContext;
    private:
        String shaderName;
        String src;
    public:
        int Id;
        Shader(String name, String source)
        {
            static int idAllocator = 0;
            Id = idAllocator++;
            shaderName = name;
            src = source;
        }
        String GetName() const
        {
            return shaderName;
        }
        String GetSource() const
        {
            return src;
        }
    };

    static void stdlibDiagnosticCallback(
        char const* message,
        void*       userData)
    {
        fputs(message, stderr);
        fflush(stderr);
#ifdef WIN32
        OutputDebugStringA(message);
#endif
    }

    class Session
    {
    public:
        bool useCache = false;
        CoreLib::String cacheDir;
        struct State
        {
            List<CompileUnit> moduleUnits;
            HashSet<String> processedModuleUnits;
            int errorCount = 0;
        };
        Array<State, 128> states;
        List<RefPtr<Spire::Compiler::CompilationContext>> compileContext;
        RefPtr<ShaderCompiler> compiler;
        CompileUnit predefUnit;


        Session(bool /*pUseCache*/, CoreLib::String /*pCacheDir*/)
        {
            compiler = CreateShaderCompiler();
            compileContext.Add(new Spire::Compiler::CompilationContext());
            states.Add(State());

            predefUnit = createPredefUnit();
            addBuiltinSource("stdlib", SpireStdLib::GetCode());
        }

        CompileUnit createPredefUnit()
        {
            CompileUnit translationUnit;


            RefPtr<ProgramSyntaxNode> translationUnitSyntax = new ProgramSyntaxNode();

            TranslationUnitOptions translationUnitOptions;
            translationUnit.options = translationUnitOptions;
            translationUnit.SyntaxNode = translationUnitSyntax;

            return translationUnit;
        }

        void addBuiltinSource(
            String const& path,
            String const& source)
        {
            DiagnosticSink sink;
            sink.callback = &stdlibDiagnosticCallback;

            RefPtr<SourceFile> sourceFile = new SourceFile();
            sourceFile->path = path;
            sourceFile->content = source;

            CompileOptions options;
            auto& preprocesorDefinitions = options.PreprocessorDefinitions;

            auto tokens = PreprocessSource(
                sourceFile->content,
                sourceFile->path,
                &sink,
                nullptr,
                preprocesorDefinitions);
            if(sink.GetErrorCount())
            {
                assert(!"error in stdlib");
            }

            predefUnit.options.sourceFiles.Add(sourceFile);


            auto translationUnitSyntax = predefUnit.SyntaxNode;

            parseSourceFile(
                translationUnitSyntax.Ptr(),
                options,
                tokens,
                &sink,
                sourceFile->path,
                nullptr);
            if(sink.GetErrorCount())
            {
                assert(!"error in stdlib");
            }

            // Now perform semantic checks, emit output, etc.

            CollectionOfTranslationUnits collectionOfTranslationUnits;
            collectionOfTranslationUnits.translationUnits.Add(predefUnit);

            CompileResult compileResult;
            compileResult.mSink = &sink;

            compiler->Compile(
                compileResult,
                &collectionOfTranslationUnits,
                options);
            if(compileResult.GetErrorCount())
            {
                assert(!"error in stdlib");
            }
        }

        ~Session()
        {
            SpireStdLib::Finalize();
        }

#if 0
        SpireModule * FindModule(CoreLib::String moduleName)
        {
            auto ptr = states.Last().modules.TryGetValue(moduleName);
            if (ptr)
                return ptr->Ptr();
            else
                return nullptr;
        }

        StringBuilder moduleKeyBuilder;
        SpireModule * SpecializeModule(SpireModule * /*module*/, int * /*params*/, int /*numParams*/, SpireDiagnosticSink * /*sink*/)
        {
            return nullptr;
        }

        void UpdateModuleLibrary(List<CompileUnit> & units, SpireDiagnosticSink * sink)
        {
            Spire::Compiler::CompileResult result;
            compiler->Compile(result, *compileContext.Last(), units, Options);
            
            //TODO: update metadata

            if (sink)
            {
                sink->diagnostics.AddRange(result.sink.diagnostics);
                sink->errorCount += result.GetErrorCount();
            }
        }

        int LoadModuleSource(CoreLib::String src, CoreLib::String fileName, SpireDiagnosticSink* sink)
        {
            List<CompileUnit> units;
            int errCount = LoadModuleUnits(units, src, fileName, sink);
            states.Last().moduleUnits.AddRange(units);
            UpdateModuleLibrary(units, sink);
            return errCount;
        }

        int LoadModuleUnits(List<CompileUnit> & units, CoreLib::String src, CoreLib::String fileName, SpireDiagnosticSink* sink)
        {
            auto & processedUnits = states.Last().processedModuleUnits;

            Spire::Compiler::CompileResult result;
            List<String> unitsToInclude;
            unitsToInclude.Add(fileName);
            processedUnits.Add(fileName);
            auto searchDirs = Options.SearchDirectories;
            searchDirs.Reverse();
            searchDirs.Add(Path::GetDirectoryName(fileName));
            searchDirs.Reverse();
            includeHandler.searchDirs = searchDirs;
            for (int i = 0; i < unitsToInclude.Count(); i++)
            {
                auto inputFileName = unitsToInclude[i];
                try
                {
                    String source = src;
                    if (i > 0)
                        source = File::ReadAllText(inputFileName);
                    auto unit = compiler->Parse(
                        Options,
                        result, source, inputFileName, &includeHandler, Options.PreprocessorDefinitions, predefUnit);

                    // HACK(tfoley): Assume that the first thing we parse represents the predef unit!
                    if (!predefUnit.SyntaxNode)
                    {
                        predefUnit = unit;
                    }

                    units.Add(unit);
                    if (unit.SyntaxNode)
                    {
                        for (auto inc : unit.SyntaxNode->GetUsings())
                        {
                            bool found = false;
                            for (auto & dir : searchDirs)
                            {
                                String includeFile = Path::Combine(dir, inc->fileName.Content);
                                if (File::Exists(includeFile))
                                {
                                    if (processedUnits.Add(includeFile))
                                    {
                                        unitsToInclude.Add(includeFile);
                                    }
                                    found = true;
                                    break;
                                }
                            }
                            if (!found)
                            {
                                result.GetErrorWriter()->diagnose(inc->fileName.Position, Diagnostics::cannotFindFile, inc->fileName);
                            }
                        }
                    }
                }
                catch (IOException)
                {
                    result.GetErrorWriter()->diagnose(CodePosition(0, 0, 0, ""), Diagnostics::cannotOpenFile, inputFileName);
                }
            }
            if (sink)
            {
                sink->diagnostics.AddRange(result.sink.diagnostics);
                sink->errorCount += result.GetErrorCount();
            }
            return result.GetErrorCount();
        }
#endif

#if 0
        Shader * NewShaderFromSource(const char * source, const char * fileName)
        {
            Spire::Compiler::CompileResult result;
            auto unit = compiler->Parse(
                Options,
                result, source, fileName, nullptr, Dictionary<String, String>(), predefUnit);
            auto list = unit.SyntaxNode->GetMembersOfType<TemplateShaderSyntaxNode>();
            if (list.Count())
                return new Shader((*list.begin())->Name.Content, String(source));
            return nullptr;
        }
        Shader * NewShaderFromFile(const char * fileName)
        {
            try
            {
                return NewShaderFromSource(File::ReadAllText(fileName).Buffer(), fileName);
            }
            catch (Exception)
            {
                return nullptr;
            }
        }
#endif

#if 0
        bool Compile(CompileResult & result, const Shader & shader, ArrayView<SpireModule*> modulesArgs, const char * additionalSource, SpireDiagnosticSink* sink)
        {
            Options.SymbolToCompile = shader.GetName();
            Options.TemplateShaderArguments.Clear();
            for (auto module : modulesArgs)
                Options.TemplateShaderArguments.Add(module->Name);
            return Compile(result, additionalSource + shader.GetSource(), shader.GetName(), sink);
        }

        bool Compile(CompileResult & result, CoreLib::String source, CoreLib::String fileName, SpireDiagnosticSink* sink)
        {
            if (states.Last().errorCount != 0)
                return false;
            PushContext();
            List<CompileUnit> units;
            states.Last().errorCount += LoadModuleUnits(units, source, fileName, sink);
            if (states.Last().errorCount != 0)
            {
                PopContext();
                return false;
            }

            Spire::Compiler::CompileResult cresult;
            compiler->Compile(cresult, *compileContext.Last(), units, Options);
            result.Sources = cresult.CompiledSource;
            states.Last().errorCount += cresult.GetErrorCount();
            if (sink)
            {
                sink->diagnostics.AddRange(cresult.sink.diagnostics);
                sink->errorCount += cresult.GetErrorCount();
            }
            if (states.Last().errorCount == 0)
            {
                for (auto shader : result.Sources)
                {
                    List<SpireParameterSet> paramSets;
                    for (auto & pset : shader.Value.MetaData.ParameterSets)
                    {
                        SpireParameterSet set;
                        set.paramSet = pset.Value.Ptr();
                        set.uniformBufferLegacyBindingPoint = pset.Value->UniformBufferLegacyBindingPoint;
                        for (auto & item : pset.Value->Parameters)
                        {
                            auto resType = item.Value->Type->GetBindableResourceType();
                            if (resType != BindableResourceType::NonBindable)
                            {
                                SpireResourceBindingInfo info;
                                info.Type = (SpireBindableResourceType)resType;
                                info.NumLegacyBindingPoints = item.Value->BindingPoints.Count();
                                info.LegacyBindingPoints = item.Value->BindingPoints.Buffer();
                                info.Name = item.Value->Name.Buffer();
                                set.bindings.Add(info);
                            }
                        }
                        paramSets.Add(_Move(set));
                    }
                    result.ParamSets[shader.Key] = _Move(paramSets);
                }

                result.reflectionBlob = cresult.reflectionBlob;
                cresult.reflectionBlob = NULL;
            }
            bool succ = states.Last().errorCount == 0;
            PopContext();
            return succ;
        }
#endif




    };

    struct CompileRequest
    {
        // Pointer to parent session
        Session* mSession;

        // Input options
        CompileOptions Options;

        // Output stuff
        DiagnosticSink mSink;
        String mDiagnosticOutput;

        RefPtr<ProgramLayout> mReflectionData;

        List<String> mTranslationUnitSources;

        List<String> mDependencyFilePaths;

        CompileRequest(Session* session)
            : mSession(session)
        {}

        ~CompileRequest()
        {}

        struct IncludeHandlerImpl : IncludeHandler
        {
            CompileRequest* request;

            List<String> searchDirs;

            virtual bool TryToFindIncludeFile(
                CoreLib::String const& pathToInclude,
                CoreLib::String const& pathIncludedFrom,
                CoreLib::String* outFoundPath,
                CoreLib::String* outFoundSource) override
            {
                String path = Path::Combine(Path::GetDirectoryName(pathIncludedFrom), pathToInclude);
                if (File::Exists(path))
                {
                    *outFoundPath = path;
                    *outFoundSource = File::ReadAllText(path);

                    request->mDependencyFilePaths.Add(path);

                    return true;
                }

                for (auto & dir : searchDirs)
                {
                    path = Path::Combine(dir, pathToInclude);
                    if (File::Exists(path))
                    {
                        *outFoundPath = path;
                        *outFoundSource = File::ReadAllText(path);

                        request->mDependencyFilePaths.Add(path);

                        return true;
                    }
                }
                return false;
            }
        };


        CompileUnit parseTranslationUnit(
            Spire::Compiler::CompileResult& result,
            TranslationUnitOptions const&   translationUnitOptions)
        {
            auto& options = Options;
            auto predefUnit = mSession->predefUnit.SyntaxNode.Ptr();

            IncludeHandlerImpl includeHandler;
            includeHandler.request = this;

            CompileUnit translationUnit;


            auto& preprocesorDefinitions = options.PreprocessorDefinitions;

            RefPtr<ProgramSyntaxNode> translationUnitSyntax = new ProgramSyntaxNode();

            for( auto sourceFile : translationUnitOptions.sourceFiles )
            {
                auto sourceFilePath = sourceFile->path;

                auto searchDirs = options.SearchDirectories;
                searchDirs.Reverse();
                searchDirs.Add(Path::GetDirectoryName(sourceFilePath));
                searchDirs.Reverse();
                includeHandler.searchDirs = searchDirs;

                String source = sourceFile->content;

                auto tokens = PreprocessSource(source, sourceFilePath, result.GetErrorWriter(), &includeHandler, preprocesorDefinitions);

                parseSourceFile(
                    translationUnitSyntax.Ptr(), options, tokens, result.GetErrorWriter(), sourceFilePath,
                    predefUnit);
            }

            translationUnit.options = translationUnitOptions;
            translationUnit.SyntaxNode = translationUnitSyntax;

            return translationUnit;
        }

        int executeCompilerDriverActions(
            Spire::Compiler::CompileResult& result)
        {
            // If we are being asked to do pass-through, then we need to do that here...
            if (Options.passThrough != PassThroughMode::None)
            {
                for( auto& translationUnitOptions : Options.translationUnits )
                {
                    switch( translationUnitOptions.sourceLanguage )
                    {
                    // We can pass-through code written in a native shading language
                    case SourceLanguage::GLSL:
                    case SourceLanguage::HLSL:
                        break;

                    // All other translation units need to be skipped
                    default:
                        continue;
                    }

                    auto sourceFile = translationUnitOptions.sourceFiles[0];
                    auto sourceFilePath = sourceFile->path;
                    String source = sourceFile->content;

                    mSession->compiler->PassThrough(
                        result,
                        source,
                        sourceFilePath,
                        Options,
                        translationUnitOptions);
                }
                return 0;
            }

            // TODO: load the stdlib

            CollectionOfTranslationUnits collectionOfTranslationUnits;

            // Parse everything from the input files requested
            //
            // TODO: this may trigger the loading and/or compilation of additional modules.
            for( auto& translationUnitOptions : Options.translationUnits )
            {
                auto translationUnit = parseTranslationUnit(result, translationUnitOptions);
                collectionOfTranslationUnits.translationUnits.Add(translationUnit);
            }
            if( result.GetErrorCount() != 0 )
                return 1;

            // TODO(tfoley): Probably get rid of this type for now...
            CompilationContext compileContext;

            // Now perform semantic checks, emit output, etc.
            mSession->compiler->Compile(
                result, &collectionOfTranslationUnits, Options);
            if(result.GetErrorCount() != 0)
                return 1;

            mReflectionData = collectionOfTranslationUnits.layout;

            return 0;
        }


#if 0
        // Act as exepcted of the command-line compiler
        int executeCompilerDriverActions()
        {
            Spire::Compiler::CompileResult result;

            int err = executeCompilerDriverActions(result);
            result.PrintDiagnostics();
            return err;
        }
#endif

        // Act as expected of the API-based compiler
        int executeAPIActions()
        {
            Spire::Compiler::CompileResult result;
            result.mSink = &mSink;

            int err = executeCompilerDriverActions(result);

            mDiagnosticOutput = mSink.outputBuffer.ProduceString();

            if(mSink.GetErrorCount() != 0)
                return mSink.GetErrorCount();

            // Copy over the per-translation-unit results
            int translationUnitCount = result.translationUnits.Count();
            for( int tt = 0; tt < translationUnitCount; ++tt )
            {
                auto source = result.translationUnits[tt].outputSource;
                mTranslationUnitSources.Add(source);
            }

            return err;
        }

    };

#if 0
    int executeCompilerDriverActions(Spire::Compiler::CompileOptions const& options)
    {
        Session session(false, "");
        CompileRequest request(&session);
        request.Options = options;

        return request.executeCompilerDriverActions();
    }
#endif

}

using namespace SpireLib;

// implementation of C interface

#define SESSION(x) reinterpret_cast<SpireLib::Session *>(x)
#define REQ(x) reinterpret_cast<SpireLib::CompileRequest*>(x)

SPIRE_API SpireSession* spCreateSession(const char * cacheDir)
{
    return reinterpret_cast<SpireSession *>(new SpireLib::Session((cacheDir ? true : false), cacheDir));
}

SPIRE_API void spDestroySession(
    SpireSession*   session)
{
    if(!session) return;
    delete SESSION(session);
}

SPIRE_API void spAddBuiltins(
    SpireSession*   session,
    char const*     sourcePath,
    char const*     sourceString)
{
    auto s = SESSION(session);
    s->addBuiltinSource(sourcePath, sourceString);
}


SPIRE_API SpireCompileRequest* spCreateCompileRequest(
    SpireSession* session)
{
    auto s = SESSION(session);
    auto req = new SpireLib::CompileRequest(s);
    return reinterpret_cast<SpireCompileRequest*>(req);
}

/*!
@brief Destroy a compile request.
*/
SPIRE_API void spDestroyCompileRequest(
    SpireCompileRequest*    request)
{
    if(!request) return;
    auto req = REQ(request);
    delete req;
}

SPIRE_API void spSetCompileFlags(
    SpireCompileRequest*    request,
    SpireCompileFlags       flags)
{
    REQ(request)->Options.flags = flags;
}

SPIRE_API void spSetCodeGenTarget(
        SpireCompileRequest*    request,
        int target)
{
    REQ(request)->Options.Target = (CodeGenTarget)target;
}

SPIRE_API void spSetPassThrough(
    SpireCompileRequest*    request,
    SpirePassThrough        passThrough)
{
    REQ(request)->Options.passThrough = PassThroughMode(passThrough);
}

SPIRE_API void spSetDiagnosticCallback(
    SpireCompileRequest*    request,
    SpireDiagnosticCallback callback,
    void const*             userData)
{
    if(!request) return;
    auto req = REQ(request);

    req->mSink.callback = callback;
    req->mSink.callbackUserData = (void*) userData;
}

SPIRE_API void spAddSearchPath(
        SpireCompileRequest*    request,
        const char*             searchDir)
{
    REQ(request)->Options.SearchDirectories.Add(searchDir);
}

SPIRE_API void spAddPreprocessorDefine(
    SpireCompileRequest*    request,
    const char*             key,
    const char*             value)
{
    REQ(request)->Options.PreprocessorDefinitions[key] = value;
}

SPIRE_API char const* spGetDiagnosticOutput(
    SpireCompileRequest*    request)
{
    if(!request) return 0;
    auto req = REQ(request);
    return req->mDiagnosticOutput.begin();
}

// New-fangled compilation API

SPIRE_API int spAddTranslationUnit(
    SpireCompileRequest*    request,
    SpireSourceLanguage     language,
    char const*             name)
{
    auto req = REQ(request);
    int result = req->Options.translationUnits.Count();

    TranslationUnitOptions translationUnit;

    translationUnit.sourceLanguage = SourceLanguage(language);

    req->Options.translationUnits.Add(translationUnit);

    return result;
}

SPIRE_API void spAddTranslationUnitSourceFile(
    SpireCompileRequest*    request,
    int                     translationUnitIndex,
    char const*             path)
{
    if(!request) return;
    auto req = REQ(request);
    if(!path) return;
    if(translationUnitIndex < 0) return;
    if(translationUnitIndex >= req->Options.translationUnits.Count()) return;

    String content;
    try
    {
        content = File::ReadAllText(path);
    }
    catch( ... )
    {
        // Emit a diagnostic!
        req->mSink.diagnose(
            CodePosition(0,0,0,path),
            Diagnostics::cannotOpenFile,
            path);
        return;
    }

    RefPtr<SourceFile> sourceFile = new SourceFile();
    sourceFile->path = path;
    sourceFile->content = content;

    req->Options.translationUnits[translationUnitIndex].sourceFiles.Add(sourceFile);
    req->mDependencyFilePaths.Add(path);
}

// Add a source string to the given translation unit
SPIRE_API void spAddTranslationUnitSourceString(
    SpireCompileRequest*    request,
    int                     translationUnitIndex,
    char const*             path,
    char const*             source)
{
    if(!request) return;
    auto req = REQ(request);
    if(!source) return;
    if(translationUnitIndex < 0) return;
    if(translationUnitIndex >= req->Options.translationUnits.Count()) return;

    if(!path) path = "";

    RefPtr<SourceFile> sourceFile = new SourceFile();
    sourceFile->path = path;
    sourceFile->content = source;

    req->Options.translationUnits[translationUnitIndex].sourceFiles.Add(sourceFile);
}

SPIRE_API SpireProfileID spFindProfile(
    SpireSession*   session,
    char const*     name)
{
    return Profile::LookUp(name).raw;
}

SPIRE_API int spAddTranslationUnitEntryPoint(
    SpireCompileRequest*    request,
    int                     translationUnitIndex,
    char const*             name,
    SpireProfileID          profile)
{
    if(!request) return -1;
    auto req = REQ(request);
    if(!name) return -1;
    if(translationUnitIndex < 0) return -1;
    if(translationUnitIndex >= req->Options.translationUnits.Count()) return -1;

    EntryPointOption entryPoint;
    entryPoint.name = name;
    entryPoint.profile = Profile(Profile::RawVal(profile));

    // TODO: realistically want this to be global across all TUs...
    int result = req->Options.translationUnits[translationUnitIndex].entryPoints.Count();

    req->Options.translationUnits[translationUnitIndex].entryPoints.Add(entryPoint);
    return result;
}


// Compile in a context that already has its translation units specified
SPIRE_API int spCompile(
    SpireCompileRequest*    request)
{
    auto req = REQ(request);

    int anyErrors = req->executeAPIActions();
    return anyErrors;
}

SPIRE_API int
spGetDependencyFileCount(
    SpireCompileRequest*    request)
{
    if(!request) return 0;
    auto req = REQ(request);
    return req->mDependencyFilePaths.Count();
}

/** Get the path to a file this compilation dependend on.
*/
SPIRE_API char const*
spGetDependencyFilePath(
    SpireCompileRequest*    request,
    int                     index)
{
    if(!request) return 0;
    auto req = REQ(request);
    return req->mDependencyFilePaths[index].begin();
}


// Get the output code associated with a specific translation unit
SPIRE_API char const* spGetTranslationUnitSource(
    SpireCompileRequest*    request,
    int                     translationUnitIndex)
{
    auto req = REQ(request);
    return req->mTranslationUnitSources[translationUnitIndex].begin();
}

// Reflection API

SPIRE_API SpireReflection* spGetReflection(
    SpireCompileRequest*    request)
{
    if( !request ) return 0;

    auto req = REQ(request);
    return (SpireReflection*) req->mReflectionData.Ptr();
}


// ... rest of reflection API implementation is in `Reflection.cpp`
