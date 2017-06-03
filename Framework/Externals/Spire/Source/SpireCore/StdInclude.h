#ifndef SHADER_COMPILER_STD_LIB_H
#define SHADER_COMPILER_STD_LIB_H

#include "../CoreLib/Basic.h"

namespace Spire
{
    namespace Compiler
    {
        class SpireStdLib
        {
        private:
            static CoreLib::String code;
        public:
            static CoreLib::String GetCode();
            static void Finalize();
        };
    }
}

#endif