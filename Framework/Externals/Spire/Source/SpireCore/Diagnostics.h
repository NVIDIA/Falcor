#ifndef RASTER_RENDERER_COMPILE_ERROR_H
#define RASTER_RENDERER_COMPILE_ERROR_H

#include "../CoreLib/Basic.h"
#include "../CoreLib/Tokenizer.h"

#include "../../Spire.h"

namespace Spire
{
    namespace Compiler
    {
        using namespace CoreLib::Basic;
        using namespace CoreLib::Text;

        enum class Severity
        {
            Note,
            Warning,
            Error,
            Fatal,
            Internal,
        };

        // TODO(tfoley): move this into a source file...
        inline const char* getSeverityName(Severity severity)
        {
            switch (severity)
            {
            case Severity::Note:        return "note";
            case Severity::Warning:     return "warning";
            case Severity::Error:       return "error";
            case Severity::Fatal:       return "fatal error";
            case Severity::Internal:    return "internal error";
            default:                    return "unknown error";
            }
        }

        // A structure to be used in static data describing different
        // diagnostic messages.
        struct DiagnosticInfo
        {
            int id;
            Severity severity;
            char const* messageFormat;
        };

        class Diagnostic
        {
        public:
            String Message;
            CodePosition Position;
            int ErrorID;
            Severity severity;

            Diagnostic()
            {
                ErrorID = -1;
            }
            Diagnostic(
                const String & msg,
                int id,
                const CodePosition & pos,
                Severity severity)
                : severity(severity)
            {
                Message = msg;
                ErrorID = id;
                Position = pos;
            }
        };

        class Decl;
        class Type;
        class ExpressionType;
        class ILType;
        class StageAttribute;
        struct TypeExp;
        struct QualType;

        void printDiagnosticArg(StringBuilder& sb, char const* str);
        void printDiagnosticArg(StringBuilder& sb, int val);
        void printDiagnosticArg(StringBuilder& sb, CoreLib::Basic::String const& str);
        void printDiagnosticArg(StringBuilder& sb, Decl* decl);
        void printDiagnosticArg(StringBuilder& sb, Type* type);
        void printDiagnosticArg(StringBuilder& sb, ExpressionType* type);
        void printDiagnosticArg(StringBuilder& sb, TypeExp const& type);
        void printDiagnosticArg(StringBuilder& sb, QualType const& type);
        void printDiagnosticArg(StringBuilder& sb, ILType* type);
        void printDiagnosticArg(StringBuilder& sb, CoreLib::Text::TokenType tokenType);
        void printDiagnosticArg(StringBuilder& sb, Token const& token);

        template<typename T>
        void printDiagnosticArg(StringBuilder& sb, RefPtr<T> ptr)
        {
            printDiagnosticArg(sb, ptr.Ptr());
        }

        inline CodePosition const& getDiagnosticPos(CodePosition const& pos) { return pos;  }

        class SyntaxNode;
        class ShaderClosure;
        CodePosition const& getDiagnosticPos(SyntaxNode const* syntax);
        CodePosition const& getDiagnosticPos(CoreLib::Text::Token const& token);
        CodePosition const& getDiagnosticPos(TypeExp const& typeExp);

        template<typename T>
        CodePosition getDiagnosticPos(RefPtr<T> const& ptr)
        {
            return getDiagnosticPos(ptr.Ptr());
        }

        struct DiagnosticArg
        {
            void* data;
            void (*printFunc)(StringBuilder&, void*);

            template<typename T>
            struct Helper
            {
                static void printFunc(StringBuilder& sb, void* data) { printDiagnosticArg(sb, *(T*)data); }
            };

            template<typename T>
            DiagnosticArg(T const& arg)
                : data((void*)&arg)
                , printFunc(&Helper<T>::printFunc)
            {}
        };

        class DiagnosticSink
        {
        public:
            StringBuilder outputBuffer;
//            List<Diagnostic> diagnostics;
            int errorCount = 0;

            SpireDiagnosticCallback callback            = nullptr;
            void*                   callbackUserData    = nullptr;

/*
            void Error(int id, const String & msg, const CodePosition & pos)
            {
                diagnostics.Add(Diagnostic(msg, id, pos, Severity::Error));
                errorCount++;
            }

            void Warning(int id, const String & msg, const CodePosition & pos)
            {
                diagnostics.Add(Diagnostic(msg, id, pos, Severity::Warning));
            }
*/
            int GetErrorCount() { return errorCount; }

            void diagnoseDispatch(CodePosition const& pos, DiagnosticInfo const& info)
            {
                diagnoseImpl(pos, info, 0, NULL);
            }

            void diagnoseDispatch(CodePosition const& pos, DiagnosticInfo const& info, DiagnosticArg const& arg0)
            {
                DiagnosticArg const* args[] = { &arg0 };
                diagnoseImpl(pos, info, 1, args);
            }

            void diagnoseDispatch(CodePosition const& pos, DiagnosticInfo const& info, DiagnosticArg const& arg0, DiagnosticArg const& arg1)
            {
                DiagnosticArg const* args[] = { &arg0, &arg1 };
                diagnoseImpl(pos, info, 2, args);
            }

            void diagnoseDispatch(CodePosition const& pos, DiagnosticInfo const& info, DiagnosticArg const& arg0, DiagnosticArg const& arg1, DiagnosticArg const& arg2)
            {
                DiagnosticArg const* args[] = { &arg0, &arg1, &arg2 };
                diagnoseImpl(pos, info, 3, args);
            }

            void diagnoseDispatch(CodePosition const& pos, DiagnosticInfo const& info, DiagnosticArg const& arg0, DiagnosticArg const& arg1, DiagnosticArg const& arg2, DiagnosticArg const& arg3)
            {
                DiagnosticArg const* args[] = { &arg0, &arg1, &arg2, &arg3 };
                diagnoseImpl(pos, info, 4, args);
            }

            template<typename P, typename... Args>
            void diagnose(P const& pos, DiagnosticInfo const& info, Args const&... args )
            {
                diagnoseDispatch(getDiagnosticPos(pos), info, args...);
            }

            void diagnoseImpl(CodePosition const& pos, DiagnosticInfo const& info, int argCount, DiagnosticArg const* const* args);

            // A stored state for the sink, which can be used to restore it later.
            struct State
            {
                int diagnosticCount;
                int errorCount;
            };

#if 0
            // Save the state of the sink so that it can be restored later.
            State saveState()
            {
                State state;
                state.diagnosticCount = diagnostics.Count();
                state.errorCount = errorCount;
                return state;
            }

            // Restore the state of the sink to a saved state.
            void restoreState(State state)
            {
                errorCount = state.errorCount;
                diagnostics.SetSize(state.diagnosticCount);
            }
#endif
        };

        namespace Diagnostics
        {
#define DIAGNOSTIC(id, severity, name, messageFormat) extern const DiagnosticInfo name;
#include "DiagnosticDefs.h"
        }
    }
}

#ifdef _DEBUG
#define SPIRE_INTERNAL_ERROR(sink, pos) \
    (sink)->diagnose(Spire::Compiler::CodePosition(__LINE__, 0, 0, __FILE__), Spire::Compiler::Diagnostics::internalCompilerError)
#define SPIRE_UNIMPLEMENTED(sink, pos, what) \
    (sink)->diagnose(Spire::Compiler::CodePosition(__LINE__, 0, 0, __FILE__), Spire::Compiler::Diagnostics::unimplemented, what)

#define SPIRE_UNREACHABLE(msg) do { assert(!"ureachable code:" msg); exit(1); } while(0)
#else
#define SPIRE_INTERNAL_ERROR(sink, pos) \
    (sink)->diagnose(pos, Spire::Compiler::Diagnostics::internalCompilerError)
#define SPIRE_UNIMPLEMENTED(sink, pos, what) \
    (sink)->diagnose(pos, Spire::Compiler::Diagnostics::unimplemented, what)

// TODO: find something that will perform better
#define SPIRE_UNREACHABLE(msg) exit(1)
#endif

#endif
