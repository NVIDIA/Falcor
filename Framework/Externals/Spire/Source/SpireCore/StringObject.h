#ifndef SPIRE_STRING_OBJECT_H
#define SPIRE_STRING_OBJECT_H

#include "../CoreLib/Basic.h"

namespace Spire
{
    namespace Compiler
    {
        class StringObject : public CoreLib::Object
        {
        public:
            CoreLib::String Content;
            StringObject() {}
            StringObject(const CoreLib::String & str)
                : Content(str)
            {}
        };
    }
}

#endif