#include "IL.h"
#include "../CoreLib/Tokenizer.h"

namespace Spire
{
    namespace Compiler
    {
        RefPtr<KeyHoleNode> ParseInternal(CoreLib::Text::TokenReader & parser)
        {
            RefPtr<KeyHoleNode> result = new KeyHoleNode();
            result->NodeType = parser.ReadWord();
            if (parser.LookAhead("<"))
            {
                parser.ReadToken();
                result->CaptureId = parser.ReadInt();
                parser.ReadToken();
            }
            if (parser.LookAhead("("))
            {
                while (!parser.LookAhead(")"))
                {
                    result->Children.Add(ParseInternal(parser));
                    if (parser.LookAhead(","))
                        parser.ReadToken();
                    else
                    {
                        break;
                    }
                }
                parser.Read(")");
            }
            return result;
        }

        RefPtr<KeyHoleNode> KeyHoleNode::Parse(String format)
        {
            CoreLib::Text::TokenReader parser(format);
            return ParseInternal(parser);
        }

        bool KeyHoleNode::Match(List<ILOperand*> & matchResult, ILOperand * instr)
        {
            bool matches = false;
            if (NodeType == "store")
                matches = dynamic_cast<StoreInstruction*>(instr) != nullptr;
            else if (NodeType == "op")
                matches = true;
            else if (NodeType == "load")
                matches = dynamic_cast<LoadInstruction*>(instr) != nullptr;
            else if (NodeType == "add")
                matches = dynamic_cast<AddInstruction*>(instr) != nullptr;
            else if (NodeType == "mu")
                matches = dynamic_cast<MulInstruction*>(instr) != nullptr;
            else if (NodeType == "sub")
                matches = dynamic_cast<SubInstruction*>(instr) != nullptr;
            else if (NodeType == "cal")
                matches = dynamic_cast<CallInstruction*>(instr) != nullptr;
            else if (NodeType == "switch")
                matches = dynamic_cast<SwitchInstruction*>(instr) != nullptr;
            if (matches)
            {
                if (Children.Count() > 0)
                {
                    ILInstruction * cinstr = dynamic_cast<ILInstruction*>(instr);
                    if (cinstr != nullptr)
                    {
                        int opId = 0;
                        for (auto & op : *cinstr)
                        {
                            if (opId >= Children.Count())
                            {
                                matches = false;
                                break;
                            }
                            matches = matches && Children[opId]->Match(matchResult, &op);
                            opId++;
                        }
                        if (opId != Children.Count())
                            matches = false;
                    }
                    else
                        matches = false;
                }
            }
            if (matches && CaptureId != -1)
            {
                matchResult.SetSize(Math::Max(matchResult.Count(), CaptureId + 1));
                matchResult[CaptureId] = instr;
            }
            return matches;
        }
    }
}