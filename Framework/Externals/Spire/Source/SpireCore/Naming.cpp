#include "Naming.h"

namespace Spire
{
    namespace Compiler
    {
        using namespace CoreLib;

        CoreLib::String EscapeCodeName(CoreLib::String str)
        {
            StringBuilder sb;
            bool isUnderScore = false;
            for (auto ch : str)
            {
                if (ch == '_' || !((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')))
                {
                    if (isUnderScore)
                        sb << "I_";
                    else
                        sb << "_";
                    isUnderScore = true;
                }
                else
                {
                    isUnderScore = false;
                    sb << ch;
                }
            }
            if (isUnderScore)
                sb << "I";
            return sb.ProduceString();
        }

    }
}