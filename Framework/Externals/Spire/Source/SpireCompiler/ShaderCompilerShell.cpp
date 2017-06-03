#include "CoreLib/LibIO.h"
#include "SpireLib.h"

using namespace CoreLib::Basic;
using namespace CoreLib::IO;
using namespace Spire::Compiler;

// Try to read an argument for a command-line option.
wchar_t const* tryReadCommandLineArgumentRaw(wchar_t const* option, wchar_t***ioCursor, wchar_t**end)
{
    wchar_t**& cursor = *ioCursor;
    if (cursor == end)
    {
        fprintf(stderr, "expected an argument for command-line option '%S'", option);
        exit(1);
    }
    else
    {
        return *cursor++;
    }
}

String tryReadCommandLineArgument(wchar_t const* option, wchar_t***ioCursor, wchar_t**end)
{
    return String::FromWString(tryReadCommandLineArgumentRaw(option, ioCursor, end));
}

Profile TranslateProfileName(char const* name)
{
#define PROFILE(TAG, NAME, STAGE, VERSION)	if(strcmp(name, #NAME) == 0) return Profile::TAG;
#define PROFILE_ALIAS(TAG, NAME)			if(strcmp(name, #NAME) == 0) return Profile::TAG;
#include "SpireCore/ProfileDefs.h"

    return Profile::Unknown;
}

struct OptionsParser
{
    SpireSession*           session = nullptr;
    SpireCompileRequest*    compileRequest = nullptr;

    struct RawEntryPoint
    {
        String          name;
        SpireProfileID  profileID = SPIRE_PROFILE_UNKNOWN;
        int             translationUnitIndex = -1;
    };

    // Collect entry point names, so that we can associate them
    // with entry points later...
    List<RawEntryPoint> rawEntryPoints;

    // The number of input files that have been specified
    int inputPathCount = 0;

    // If we already have a translation unit for Spire code, then this will give its index.
    // If not, it will be `-1`.
    int spireTranslationUnit = -1;

    int translationUnitCount = 0;
    int currentTranslationUnitIndex = -1;

    SpireProfileID currentProfileID = SPIRE_PROFILE_UNKNOWN;

    SpireCompileFlags flags = 0;

    void addInputSpirePath(
        String const& path)
    {
        // All of the input .spire files will be grouped into a single logical translation unit,
        // which we create lazily when the first .spire file is encountered.
        if( spireTranslationUnit == -1 )
        {
            translationUnitCount++;
            spireTranslationUnit = spAddTranslationUnit(
                compileRequest,
                SPIRE_SOURCE_LANGUAGE_SPIRE,
                nullptr);
        }

        spAddTranslationUnitSourceFile(
            compileRequest,
            spireTranslationUnit,
            path.begin());

        // Set the translation unit to be used by subsequent entry points
        currentTranslationUnitIndex = spireTranslationUnit;
    }

    void addInputForeignShaderPath(
        String const&           path,
        SpireSourceLanguage     language)
    {
        translationUnitCount++;
        currentTranslationUnitIndex = spAddTranslationUnit(
                compileRequest,
                language,
                nullptr);

        spAddTranslationUnitSourceFile(
            compileRequest,
            currentTranslationUnitIndex,
            path.begin());
    }

    void addInputPath(
        wchar_t const*  inPath)
    {
        inputPathCount++;

        // look at the extension on the file name to determine
        // how we should handle it.
        String path = String::FromWString(inPath);

        if( path.EndsWith(".spire") )
        {
            // Plain old spire code
            addInputSpirePath(path);
        }
        else if( path.EndsWith(".hlsl") )
        {
            // HLSL for rewriting
            addInputForeignShaderPath(path, SPIRE_SOURCE_LANGUAGE_HLSL);
        }
        else if( path.EndsWith(".glsl") )
        {
            // HLSL for rewriting
            addInputForeignShaderPath(path, SPIRE_SOURCE_LANGUAGE_GLSL);
        }
        else
        {
            fprintf(stderr, "error: can't deduce language for input file '%S'\n", inPath);
            exit(1);
        }
    }

    void parse(
        int             argc,
        wchar_t**       argv)
    {
        wchar_t** argCursor = &argv[1];
        wchar_t** argEnd = &argv[argc];
        while (argCursor != argEnd)
        {
            wchar_t const* arg = *argCursor++;
            if (arg[0] == '-')
            {
                String argStr = String::FromWString(arg);

                // The argument looks like an option, so try to parse it.
//                if (argStr == "-outdir")
//                    outputDir = tryReadCommandLineArgument(arg, &argCursor, argEnd);
//                if (argStr == "-out")
//                    options.outputName = tryReadCommandLineArgument(arg, &argCursor, argEnd);
//                else if (argStr == "-symbo")
//                    options.SymbolToCompile = tryReadCommandLineArgument(arg, &argCursor, argEnd);
                //else
                if (argStr == "-no-checking")
                    flags |= SPIRE_COMPILE_FLAG_NO_CHECKING;
                else if (argStr == "-backend" || argStr == "-target")
                {
                    String name = tryReadCommandLineArgument(arg, &argCursor, argEnd);
                    SpireCompileTarget target = SPIRE_TARGET_UNKNOWN;

                    if (name == "glsl")
                    {
                        target = SPIRE_GLSL;
                    }
                    else if (name == "glsl_vk")
                    {
                        target = SPIRE_GLSL_VULKAN;
                    }
//                    else if (name == "glsl_vk_onedesc")
//                    {
//                        options.Target = CodeGenTarget::GLSL_Vulkan_OneDesc;
//                    }
                    else if (name == "hlsl")
                    {
                        target = SPIRE_HLSL;
                    }
                    else if (name == "spriv")
                    {
                        target = SPIRE_SPIRV;
                    }
                    else if (name == "dxbc")
                    {
                        target = SPIRE_DXBC;
                    }
                    else if (name == "dxbc-assembly")
                    {
                        target = SPIRE_DXBC_ASM;
                    }
                    else if (name == "reflection-json")
                    {
                        target = SPIRE_REFLECTION_JSON;
                    }
                    else
                    {
                        fprintf(stderr, "unknown code generation target '%S'\n", name.ToWString());
                        exit(1);
                    }

                    spSetCodeGenTarget(compileRequest, target);
                }
                // A "profile" specifies both a specific target stage and a general level
                // of capability required by the program.
                else if (argStr == "-profile")
                {
                    String name = tryReadCommandLineArgument(arg, &argCursor, argEnd);

                    SpireProfileID profileID = spFindProfile(session, name.begin());
                    if( profileID == SPIRE_PROFILE_UNKNOWN )
                    {
                        fprintf(stderr, "unknown profile '%s'\n", name.begin());
                    }
                    else
                    {
                        currentProfileID = profileID;
                    }
                }
                else if (argStr == "-entry")
                {
                    String name = tryReadCommandLineArgument(arg, &argCursor, argEnd);

                    RawEntryPoint entry;
                    entry.name = name;
                    entry.translationUnitIndex = currentTranslationUnitIndex;

                    // TODO(tfoley): Allow user to fold a specification of a profile into the entry-point name,
                    // for the case where they might be compiling multiple entry points in one invocation...
                    //
                    // For now, just use the last profile set on the command-line to specify this

                    entry.profileID = currentProfileID;

                    rawEntryPoints.Add(entry);
                }
#if 0
                else if (argStr == "-stage")
                {
                    String name = tryReadCommandLineArgument(arg, &argCursor, argEnd);
                    StageTarget stage = StageTarget::Unknown;
                    if (name == "vertex") { stage = StageTarget::VertexShader; }
                    else if (name == "fragment") { stage = StageTarget::FragmentShader; }
                    else if (name == "hull") { stage = StageTarget::HullShader; }
                    else if (name == "domain") { stage = StageTarget::DomainShader; }
                    else if (name == "compute") { stage = StageTarget::ComputeShader; }
                    else
                    {
                        fprintf(stderr, "unknown stage '%S'\n", name.ToWString());
                    }
                    options.stage = stage;
                }
#endif
                else if (argStr == "-pass-through")
                {
                    String name = tryReadCommandLineArgument(arg, &argCursor, argEnd);
                    SpirePassThrough passThrough = SPIRE_PASS_THROUGH_NONE;
                    if (name == "fxc") { passThrough = SPIRE_PASS_THROUGH_FXC; }
                    else
                    {
                        fprintf(stderr, "unknown pass-through target '%S'\n", name.ToWString());
                        exit(1);
                    }

                    spSetPassThrough(
                        compileRequest,
                        passThrough);
                }
//                else if (argStr == "-genchoice")
//                    options.Mode = CompilerMode::GenerateChoice;
                else if (argStr[1] == 'D')
                {
                    // The value to be defined might be part of the same option, as in:
                    //     -DFOO
                    // or it might come separately, as in:
                    //     -D FOO
                    wchar_t const* defineStr = arg + 2;
                    if (defineStr[0] == 0)
                    {
                        // Need to read another argument from the command line
                        defineStr = tryReadCommandLineArgumentRaw(arg, &argCursor, argEnd);
                    }
                    // The string that sets up the define can have an `=` between
                    // the name to be defined and its value, so we search for one.
                    wchar_t const* eqPos = nullptr;
                    for(wchar_t const* dd = defineStr; *dd; ++dd)
                    {
                        if (*dd == '=')
                        {
                            eqPos = dd;
                            break;
                        }
                    }

                    // Now set the preprocessor define
                    //
                    if (eqPos)
                    {
                        // If we found an `=`, we split the string...

                        spAddPreprocessorDefine(
                            compileRequest,
                            String::FromWString(defineStr, eqPos).begin(),
                            String::FromWString(eqPos+1).begin());
                    }
                    else
                    {
                        // If there was no `=`, then just #define it to an empty string

                        spAddPreprocessorDefine(
                            compileRequest,
                            String::FromWString(defineStr).begin(),
                            "");
                    }
                }
                else if (argStr[1] == 'I')
                {
                    // The value to be defined might be part of the same option, as in:
                    //     -IFOO
                    // or it might come separately, as in:
                    //     -I FOO
                    // (see handling of `-D` above)
                    wchar_t const* includeDirStr = arg + 2;
                    if (includeDirStr[0] == 0)
                    {
                        // Need to read another argument from the command line
                        includeDirStr = tryReadCommandLineArgumentRaw(arg, &argCursor, argEnd);
                    }

                    spAddSearchPath(
                        compileRequest,
                        String::FromWString(includeDirStr).begin());
                }
                else if (argStr == "--")
                {
                    // The `--` option causes us to stop trying to parse options,
                    // and treat the rest of the command line as input file names:
                    while (argCursor != argEnd)
                    {
                        addInputPath(*argCursor++);
                    }
                    break;
                }
                else
                {
                    fprintf(stderr, "unknown command-line option '%S'\n", argStr.ToWString());
                    // TODO: print a usage message
                    exit(1);
                }
            }
            else
            {
                addInputPath(arg);
            }
        }

        spSetCompileFlags(compileRequest, flags);

        if (inputPathCount == 0)
        {
            fprintf(stderr, "error: no input file specified\n");
            exit(1);
        }

        // No point in moving forward if there is nothing to compile
        if( translationUnitCount == 0 )
        {
            fprintf(stderr, "error: no compilation requested\n");
            exit(1);
        }

        // For any entry points that were given without an explicit profile, we can now apply
        // the profile that was given to them.
        if( rawEntryPoints.Count() != 0 )
        {
            if( currentProfileID == SPIRE_PROFILE_UNKNOWN )
            {
                fprintf(stderr, "error: no profile specified; use the '-profile <profile name>' option");
                exit(1);
            }
            // TODO: issue an error if we have multiple `-profile` options *and*
            // there are entry points that didn't get a profile.
            else
            {
                for( auto& e : rawEntryPoints )
                {
                    if( e.profileID == SPIRE_PROFILE_UNKNOWN )
                    {
                        e.profileID = currentProfileID;
                    }
                }
            }
        }

        // Next, we want to make sure that entry points get attached to the appropriate translation
        // unit that will provide them.
        {
            bool anyEntryPointWithoutTranslationUnit = false;
            for( auto& entryPoint : rawEntryPoints )
            {
                // Skip entry points that are already associated with a translation unit...
                if( entryPoint.translationUnitIndex != -1 )
                    continue;

                anyEntryPointWithoutTranslationUnit = true;
                entryPoint.translationUnitIndex = 0;
            }

            if( anyEntryPointWithoutTranslationUnit && translationUnitCount != 1 )
            {
                fprintf(stderr, "error: when using multiple translation units, entry points must be specified after their translation unit file(s)");
                exit(1);
            }

            // Now place all those entry points where they belong
            for( auto& entryPoint : rawEntryPoints )
            {
                spAddTranslationUnitEntryPoint(
                    compileRequest,
                    entryPoint.translationUnitIndex,
                    entryPoint.name.begin(),
                    entryPoint.profileID);
            }
        }

#if 0
        // Automatically derive an output directory based on the first file specified.
        //
        // TODO: require manual specification if there are multiple input files, in different directories
        String fileName = options.translationUnits[0].sourceFilePaths[0];
        if (outputDir.Length() == 0)
        {
            outputDir = Path::GetDirectoryName(fileName);
        }
#endif
    }

};


void parseOptions(
    SpireCompileRequest*    compileRequest,
    int                     argc,
    wchar_t**               argv)
{
    OptionsParser parser;
    parser.compileRequest = compileRequest;
    parser.parse(argc, argv);
}

static void diagnosticCallback(
    char const* message,
    void*       userData)
{
    fputs(message, stderr);
    fflush(stderr);
}

int wmain(int argc, wchar_t* argv[])
{
    // Parse any command-line options

    SpireSession* session = spCreateSession(nullptr);
    SpireCompileRequest* compileRequest = spCreateCompileRequest(session);

    parseOptions(compileRequest, argc, argv);

    spSetDiagnosticCallback(
        compileRequest,
        &diagnosticCallback,
        nullptr);

    // Invoke the compiler

#ifndef _DEBUG
    try
#endif
    {
        int result = spCompile(compileRequest);
        if( result != 0 )
        {
            // TODO: emit the diagnostics here!!!

            exit(-1);
        }

        // TODO: generate all the required outputs here!!!


        spDestroyCompileRequest(compileRequest);
        spDestroySession(session);
    }
#ifndef _DEBUG
    catch (Exception & e)
    {
        printf("internal compiler error: %S\n", e.Message.ToWString());
        return 1;
    }
#endif

#if 0
        int returnValue = -1;
    {


        auto sourceDir = Path::GetDirectoryName(fileName);
        CompileResult result;
        try
        {
            auto files = SpireLib::CompileShaderSourceFromFile(result, fileName, options);
            for (auto & f : files)
            {
                try
                {
                    f.SaveToFile(Path::Combine(outputDir, f.MetaData.ShaderName + ".cse"));
                }
                catch (Exception &)
                {
                    result.GetErrorWriter()->diagnose(CodePosition(0, 0, 0, ""), Diagnostics::cannotWriteOutputFile, Path::Combine(outputDir, f.MetaData.ShaderName + ".cse"));
                }
            }
        }
        catch (Exception & e)
        {
            printf("internal compiler error: %S\n", e.Message.ToWString());
        }
        result.PrintDiagnostics();
        if (result.GetErrorCount() == 0)
            returnValue = 0;
    }
#endif

#ifdef _MSC_VER
    _CrtDumpMemoryLeaks();
#endif
    return 0;
}