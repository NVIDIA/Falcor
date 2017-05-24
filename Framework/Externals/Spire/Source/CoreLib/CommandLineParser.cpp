#include "CommandLineParser.h"

namespace CoreLib
{
	namespace Text
	{
		CommandLineParser::CommandLineParser(const String & cmdLine)
		{
			stream = Split(cmdLine, L' ');
		}

		String CommandLineParser::GetFileName()
		{
			if (stream.Count())
				return stream.First();
			else
				return "";
		}

		bool CommandLineParser::OptionExists(const String & opt)
		{
			for (auto & token : stream)
			{
				if (token.Equals(opt, false))
				{
					return true;
				}
			}
			return false;
		}

		String CommandLineParser::GetOptionValue(const String & opt)
		{
			for (int i = 0; i < stream.Count(); i++)
			{
				if (stream[i].Equals(opt, false))
				{
					if (i < stream.Count() - 1)
						return stream[i+1];
					return "";
				}
			}
			return "";
		}

		String CommandLineParser::GetToken(int id)
		{
			return stream[id];
		}

		int CommandLineParser::GetTokenCount()
		{
			return stream.Count();
		}
	}
}