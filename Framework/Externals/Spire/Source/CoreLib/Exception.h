#ifndef CORE_LIB_EXCEPTION_H
#define CORE_LIB_EXCEPTION_H

#include "Common.h"
#include "LibString.h"

namespace CoreLib
{
	namespace Basic
	{
		class Exception : public Object
		{
		public:
			String Message;
			Exception()
			{}
			Exception(const String & message)
				: Message(message)
			{
			}
		};

		class IndexOutofRangeException : public Exception
		{
		public:
			IndexOutofRangeException()
			{}
			IndexOutofRangeException(const String & message)
				: Exception(message)
			{
			}

		};

		class InvalidOperationException : public Exception
		{
		public:
			InvalidOperationException()
			{}
			InvalidOperationException(const String & message)
				: Exception(message)
			{
			}

		};
		
		class ArgumentException : public Exception
		{
		public:
			ArgumentException()
			{}
			ArgumentException(const String & message)
				: Exception(message)
			{
			}

		};

		class KeyNotFoundException : public Exception
		{
		public:
			KeyNotFoundException()
			{}
			KeyNotFoundException(const String & message)
				: Exception(message)
			{
			}
		};
		class KeyExistsException : public Exception
		{
		public:
			KeyExistsException()
			{}
			KeyExistsException(const String & message)
				: Exception(message)
			{
			}
		};

		class NotSupportedException : public Exception
		{
		public:
			NotSupportedException()
			{}
			NotSupportedException(const String & message)
				: Exception(message)
			{
			}
		};

		class NotImplementedException : public Exception
		{
		public:
			NotImplementedException()
			{}
			NotImplementedException(const String & message)
				: Exception(message)
			{
			}
		};

		class InvalidProgramException : public Exception
		{
		public:
			InvalidProgramException()
			{}
			InvalidProgramException(const String & message)
				: Exception(message)
			{
			}
		};
	}
}

#endif