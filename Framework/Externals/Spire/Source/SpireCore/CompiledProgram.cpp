#include "CompiledProgram.h"

namespace Spire
{
    namespace Compiler
    {
        void IndentString(StringBuilder & sb, String src)
        {
            int indent = 0;
            bool beginTrim = true;
            for (int c = 0; c < src.Length(); c++)
            {
                auto ch = src[c];
                if (ch == '\n')
                {
                    sb << "\n";

                    beginTrim = true;
                }
                else
                {
                    if (beginTrim)
                    {
                        while (c < src.Length() - 1 && (src[c] == '\t' || src[c] == '\n' || src[c] == '\r' || src[c] == ' '))
                        {
                            c++;
                            ch = src[c];
                        }
                        for (int i = 0; i < indent - 1; i++)
                            sb << '\t';
                        if (ch != '}' && indent > 0)
                            sb << '\t';
                        beginTrim = false;
                    }

                    if (ch == '{')
                        indent++;
                    else if (ch == '}')
                        indent--;
                    if (indent < 0)
                        indent = 0;

                    sb << ch;
                }
            }
        }
    }
}