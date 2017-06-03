// Compiler.cpp : Defines the entry point for the console application.
//
#include "../CoreLib/Basic.h"
#include "../CoreLib/LibIO.h"
#include "ShaderCompiler.h"
#include "Lexer.h"
#include "ParameterBinding.h"
#include "Parser.h"
#include "Preprocessor.h"
#include "SyntaxVisitors.h"
#include "StdInclude.h"
#include "CodeGenBackend.h"
#include "../CoreLib/Tokenizer.h"
#include "Naming.h"

#include "Reflection.h"
#include "Emit.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX
#include <d3dcompiler.h>
#endif

#ifdef CreateDirectory
#undef CreateDirectory
#endif

using namespace CoreLib::Basic;
using namespace CoreLib::IO;
using namespace Spire::Compiler;

namespace Spire
{
    namespace Compiler
    {
        //

        Profile Profile::LookUp(char const* name)
        {
            #define PROFILE(TAG, NAME, STAGE, VERSION)	if(strcmp(name, #NAME) == 0) return Profile::TAG;
            #define PROFILE_ALIAS(TAG, NAME)			if(strcmp(name, #NAME) == 0) return Profile::TAG;
            #include "ProfileDefs.h"

            return Profile::Unknown;
        }



        //

        int compilerInstances = 0;

        class ShaderCompilerImpl : public ShaderCompiler
        {
        private:
            Dictionary<String, RefPtr<CodeGenBackend>> backends;
        public:
            virtual CompileUnit Parse(CompileOptions& options, CompileResult & result, String source, String fileName, IncludeHandler* includeHandler, Dictionary<String,String> const& preprocesorDefinitions,
                CompileUnit predefUnit) override
            {
                auto tokens = PreprocessSource(source, fileName, result.GetErrorWriter(), includeHandler, preprocesorDefinitions);
                CompileUnit rs;
                rs.SyntaxNode = ParseProgram(options, tokens, result.GetErrorWriter(), fileName, predefUnit.SyntaxNode.Ptr());
                return rs;
            }

            // Actual context for compilation... :(
            struct ExtraContext
            {
                CompileOptions const* options = nullptr;
                TranslationUnitOptions const* translationUnitOptions = nullptr;

                CompileResult* compileResult = nullptr;

                RefPtr<ProgramSyntaxNode>   programSyntax;
                ProgramLayout*              programLayout;

                String sourceText;
                String sourcePath;

                CompileOptions const& getOptions() { return *options; }
                TranslationUnitOptions const& getTranslationUnitOptions() { return *translationUnitOptions; }
            };


            String EmitHLSL(ExtraContext& context)
            {
                if (context.getOptions().passThrough != PassThroughMode::None)
                {
                    return context.sourceText;
                }
                else
                {
                    // TODO(tfoley): probably need a way to customize the emit logic...
                    return emitProgram(
                        context.programSyntax.Ptr(),
                        context.programLayout);
                }
            }

            char const* GetHLSLProfileName(Profile profile)
            {
                switch(profile.raw)
                {
                #define PROFILE(TAG, NAME, STAGE, VERSION) case Profile::TAG: return #NAME;
                #include "ProfileDefs.h"

                default:
                    // TODO: emit an error here!
                    return "unknown";
                }
            }

#ifdef _WIN32
            void* GetD3DCompilerDLL()
            {
                // TODO(tfoley): let user specify version of d3dcompiler DLL to use.
                static HMODULE d3dCompiler =  LoadLibraryA("d3dcompiler_47");
                // TODO(tfoley): handle case where we can't find it gracefully
                assert(d3dCompiler);
                return d3dCompiler;
            }

            List<uint8_t> EmitDXBytecodeForEntryPoint(
                ExtraContext&				context,
                EntryPointOption const&		entryPoint)
            {
                static pD3DCompile D3DCompile_ = nullptr;
                if (!D3DCompile_)
                {
                    HMODULE d3dCompiler = (HMODULE)GetD3DCompilerDLL();
                    assert(d3dCompiler);

                    D3DCompile_ = (pD3DCompile)GetProcAddress(d3dCompiler, "D3DCompile");
                    assert(D3DCompile_);
                }

                // The HLSL compiler will try to "canonicalize" our input file path,
                // and we don't want it to do that, because they it won't report
                // the same locations on error messages that we would.
                //
                // To work around that, we prepend a custom `#line` directive.

                String rawHlslCode = EmitHLSL(context);

                StringBuilder hlslCodeBuilder;
                hlslCodeBuilder << "#line 1 \"";
                for(auto c : context.sourcePath)
                {
                    char buffer[] = { c, 0 };
                    switch(c)
                    {
                    default:
                        hlslCodeBuilder << buffer;
                        break;

                    case '\\':
                        hlslCodeBuilder << "\\\\";
                    }
                }
                hlslCodeBuilder << "\"\n";
                hlslCodeBuilder << rawHlslCode;

                auto hlslCode = hlslCodeBuilder.ProduceString();

                ID3DBlob* codeBlob;
                ID3DBlob* diagnosticsBlob;
                HRESULT hr = D3DCompile_(
                    hlslCode.begin(),
                    hlslCode.Length(),
                    context.sourcePath.begin(),
                    nullptr,
                    nullptr,
                    entryPoint.name.begin(),
                    GetHLSLProfileName(entryPoint.profile),
                    0,
                    0,
                    &codeBlob,
                    &diagnosticsBlob);
                List<uint8_t> data;
                if (codeBlob)
                {
                    data.AddRange((uint8_t const*)codeBlob->GetBufferPointer(), (int)codeBlob->GetBufferSize());
                    codeBlob->Release();
                }
                if (diagnosticsBlob)
                {
                    // TODO(tfoley): need a better policy for how we translate diagnostics
                    // back into the Spire world (although we should always try to generate
                    // HLSL that doesn't produce any diagnostics...)
                    String diagnostics = (char const*) diagnosticsBlob->GetBufferPointer();
                    fprintf(stderr, "%s", diagnostics.begin());
                    OutputDebugStringA(diagnostics.begin());
                    diagnosticsBlob->Release();
                }
                if (FAILED(hr))
                {
                    // TODO(tfoley): What to do on failure?
                }
                return data;
            }

            List<uint8_t> EmitDXBytecode(
                ExtraContext&				context)
            {
                if(context.getTranslationUnitOptions().entryPoints.Count() != 1)
                {
                    if(context.getTranslationUnitOptions().entryPoints.Count() == 0)
                    {
                        // TODO(tfoley): need to write diagnostics into this whole thing...
                        fprintf(stderr, "no entry point specified\n");
                    }
                    else
                    {
                        fprintf(stderr, "multiple entry points specified\n");
                    }
                    return List<uint8_t>();
                }

                return EmitDXBytecodeForEntryPoint(context, context.getTranslationUnitOptions().entryPoints[0]);
            }

            String EmitDXBytecodeAssemblyForEntryPoint(
                ExtraContext&				context,
                EntryPointOption const&		entryPoint)
            {
                static pD3DDisassemble D3DDisassemble_ = nullptr;
                if (!D3DDisassemble_)
                {
                    HMODULE d3dCompiler = (HMODULE)GetD3DCompilerDLL();
                    assert(d3dCompiler);

                    D3DDisassemble_ = (pD3DDisassemble)GetProcAddress(d3dCompiler, "D3DDisassemble");
                    assert(D3DDisassemble_);
                }

                List<uint8_t> dxbc = EmitDXBytecodeForEntryPoint(context, entryPoint);
                if (!dxbc.Count())
                {
                    return "";
                }

                ID3DBlob* codeBlob;
                HRESULT hr = D3DDisassemble_(
                    &dxbc[0],
                    dxbc.Count(),
                    0,
                    nullptr,
                    &codeBlob);

                String result;
                if (codeBlob)
                {
                    result = String((char const*) codeBlob->GetBufferPointer());
                    codeBlob->Release();
                }
                if (FAILED(hr))
                {
                    // TODO(tfoley): need to figure out what to diagnose here...
                }
                return result;
            }


            String EmitDXBytecodeAssembly(
                ExtraContext&				context)
            {
                if(context.getTranslationUnitOptions().entryPoints.Count() == 0)
                {
                    // TODO(tfoley): need to write diagnostics into this whole thing...
                    fprintf(stderr, "no entry point specified\n");
                    return "";
                }

                StringBuilder sb;
                for (auto entryPoint : context.getTranslationUnitOptions().entryPoints)
                {
                    sb << EmitDXBytecodeAssemblyForEntryPoint(context, entryPoint);
                }
                return sb.ProduceString();
            }
#endif

            TranslationUnitResult DoNewEmitLogic(ExtraContext& context)
            {
                //
                TranslationUnitResult result;

                switch (context.getOptions().Target)
                {
                case CodeGenTarget::HLSL:
                    {
                        String hlslProgram = EmitHLSL(context);

                        if (context.compileResult)
                        {
                            result.outputSource = hlslProgram;
#if 0
                            StageSource stageSource;
                            stageSource.MainCode = hlslProgram;
                            CompiledShaderSource compiled;
                            compiled.Stages[""] = stageSource;
                            context.compileResult->CompiledSource[""] = compiled;
#endif
                        }
                        else
                        {
                            fprintf(stdout, "%s", hlslProgram.begin());
                        }
                        return result;
                    }
                    break;

                case CodeGenTarget::DXBytecode:
                    {
                        auto code = EmitDXBytecode(context);
                        if (context.compileResult)
                        {
                            StringBuilder sb;
                            sb.Append((char*) code.begin(), code.Count());

                            String codeString = sb.ProduceString();
                            result.outputSource = codeString;

#if 0
                            StageSource stageSource;
                            stageSource.MainCode = codeString;
                            CompiledShaderSource compiled;
                            compiled.Stages[""] = stageSource;
                            context.compileResult->CompiledSource[""] = compiled;
#endif
                        }
                        else
                        {
                            int col = 0;
                            for(auto ii : code)
                            {
                                if(col != 0) fputs(" ", stdout);
                                fprintf(stdout, "%02X", ii);
                                col++;
                                if(col == 8)
                                {
                                    fputs("\n", stdout);
                                    col = 0;
                                }
                            }
                            if(col != 0)
                            {
                                fputs("\n", stdout);
                            }
                        }
                        return result;
                    }
                    break;

                case CodeGenTarget::DXBytecodeAssembly:
                    {
                        String hlslProgram = EmitDXBytecodeAssembly(context);

                        // HACK(tfoley): just print it out since that is what people probably expect.
                        // TODO: need a way to control where output gets routed across all possible targets.
                        fprintf(stdout, "%s", hlslProgram.begin());
                        return result;
                    }
                    break;

                // Note(tfoley): We currently hit this case when compiling the stdlib
                case CodeGenTarget::Unknown:
                    break;

                default:
                    throw "unimplemented";
                }

                return result;
            }

            void DoNewEmitLogic(
                ExtraContext&                   context,
                CollectionOfTranslationUnits*   collectionOfTranslationUnits)
            {
                switch (context.getOptions().Target)
                {
                default:
                    // For most targets, we will do things per-translation-unit
                    for( auto translationUnit : collectionOfTranslationUnits->translationUnits )
                    {
                        ExtraContext innerContext = context;
                        innerContext.translationUnitOptions = &translationUnit.options;
                        innerContext.programSyntax = translationUnit.SyntaxNode;
                        innerContext.sourcePath = "spire"; // don't have this any more!
                        innerContext.sourceText = "";

                        TranslationUnitResult translationUnitResult = DoNewEmitLogic(innerContext);
                        context.compileResult->translationUnits.Add(translationUnitResult);
                    }
                    break;

                case CodeGenTarget::ReflectionJSON:
                    {
                        String reflectionJSON = emitReflectionJSON(context.programLayout);

                        // HACK(tfoley): just print it out since that is what people probably expect.
                        // TODO: need a way to control where output gets routed across all possible targets.
                        fprintf(stdout, "%s", reflectionJSON.begin());
                    }
                    break;
                }
            }

            virtual void Compile(
                CompileResult&                  result,
                CollectionOfTranslationUnits*   collectionOfTranslationUnits,
                const CompileOptions&           options) override
            {
                RefPtr<SyntaxVisitor> visitor = CreateSemanticsVisitor(result.GetErrorWriter(), options);
                try
                {
                    for( auto& translationUnit : collectionOfTranslationUnits->translationUnits )
                    {
                        translationUnit.SyntaxNode->Accept(visitor.Ptr());
                    }
                    if (result.GetErrorCount() > 0)
                        return;

#if ENABLE_ILGEN

                    RefPtr<ILProgram> program = new ILProgram();
                    RefPtr<SyntaxVisitor> codeGen = CreateILCodeGenerator(result.GetErrorWriter(), program.Ptr(), const_cast<CompileOptions*>(&options));
                    codeGen->VisitProgram(programSyntaxNode.Ptr());
                    
                    if (result.GetErrorCount() > 0)
                        return;

                    RefPtr<CodeGenBackend> backend = nullptr;
                    switch (options.Target)
                    {
                    case CodeGenTarget::HLSL:
                        backend = CreateHLSLCodeGen();
                        break;
                    case CodeGenTarget::GLSL:
                        backend = CreateGLSLCodeGen();
                        break;
                    case CodeGenTarget::GLSL_Vulkan:
                        backend = CreateGLSL_VulkanCodeGen();
                        break;
                    default:
                        result.sink.diagnose(CodePosition(), Diagnostics::unimplemented, "specified backend");
                        return;
                    }

                    CompiledShaderSource compiledSource = backend->GenerateShader(options, program.Ptr(), &result.sink);
                    compiledSource.MetaData.ShaderName = options.outputName;
                    StageSource stageSrc;
                    stageSrc.MainCode = program->ToString();
                    compiledSource.Stages["il"] = stageSrc;
                    result.CompiledSource["code"] = compiledSource;

#endif

                    // Do binding generation, and then reflection (globally)
                    // before we move on to any code-generation activites.
                    GenerateParameterBindings(collectionOfTranslationUnits);


                    // HACK(tfoley): for right now I just want to pretty-print an AST
                    // into another language, so the whole compiler back-end is just
                    // getting in the way.
                    //
                    // I'm going to bypass it for now and see what I can do:

                    ExtraContext extra;
                    extra.options = &options;
                    extra.programLayout = collectionOfTranslationUnits->layout.Ptr();
                    extra.compileResult = &result;
                    DoNewEmitLogic(extra, collectionOfTranslationUnits);
                }
                catch (int)
                {
                }
                catch (...)
                {
                    throw;
                }
                return;
            }

            ShaderCompilerImpl()
            {
                if (compilerInstances == 0)
                {
                    BasicExpressionType::Init();
                }
                compilerInstances++;
                backends.Add("glsl", CreateGLSLCodeGen());
                backends.Add("hlsl", CreateHLSLCodeGen());
                backends.Add("glsl_vk", CreateGLSL_VulkanCodeGen());
                backends.Add("glsl_vk_onedesc", CreateGLSL_VulkanOneDescCodeGen());
            }

            ~ShaderCompilerImpl()
            {
                compilerInstances--;
                if (compilerInstances == 0)
                {
                    BasicExpressionType::Finalize();
                    SpireStdLib::Finalize();
                }
            }

            virtual TranslationUnitResult PassThrough(
                CompileResult &			/*result*/,
                String const&			sourceText,
                String const&			sourcePath,
                const CompileOptions &	options,
                TranslationUnitOptions const& translationUnitOptions) override
            {
                ExtraContext extra;
                extra.options = &options;
                extra.translationUnitOptions = &translationUnitOptions;
                extra.sourcePath = sourcePath;
                extra.sourceText = sourceText;

                return DoNewEmitLogic(extra);
            }

        };

        ShaderCompiler * CreateShaderCompiler()
        {
            return new ShaderCompilerImpl();
        }

    }
}
