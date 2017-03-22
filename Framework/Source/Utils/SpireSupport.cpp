/***************************************************************************
# Copyright (c) 2015, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************/

// This file only exists to compile the Spire source code into the library.

#define NOMINMAX

#include "Externals/Spire/SpireAllSource.h"
#include "SpireSupport.h"
#include "Logger.h"
#include "OS.h"

namespace Falcor
{
	class ShaderRepositoryImpl
	{
	private:
		SpireCompilationContext * context;
		SpireCompilationEnvironment * libEnv;
		SpireDiagnosticSink * sink;
		void ReportErrors()
		{
			int size = spGetDiagnosticOutput(sink, nullptr, 0);
			std::vector<char> buffer;
			buffer.resize(size);
			spGetDiagnosticOutput(sink, buffer.data(), size);
			logError(buffer.data(), true);
		}
	public:
		ShaderRepositoryImpl()
		{
			context = spCreateCompilationContext(nullptr);
			sink = spCreateDiagnosticSink(context);

			spSetCodeGenTarget(context, SPIRE_HLSL);

			for (auto p : getDataDirectoriesList())
			{
				spAddSearchPath(context, p.c_str());
			}

			const char * libFiles[] = { "StandardPipeline.spire", "SharedLib.spire" };
			for (auto file : libFiles)
			{
				std::string libPath;
				if (!findFileInDataDirectories(file, libPath))
				{
					std::string err = std::string("Can't find shader file ") + file;
					logError(err, true);
					break;
				}
				spLoadModuleLibrary(context, libPath.c_str(), sink);
			}

			if (spDiagnosticSinkHasAnyErrors(sink))
			{
				ReportErrors();
			}

			libEnv = spGetCurrentEnvironment(context);
		}
		~ShaderRepositoryImpl()
		{
			spReleaseEnvironment(libEnv);
			spDestroyDiagnosticSink(sink);
			spDestroyCompilationContext(context);
		}
		SpireCompilationContext * GetContext()
		{
			return context;
		}
		SpireModule* LoadLibraryModule(const char * name)
		{
			return spEnvFindModule(libEnv, name);
		}
		SpireCompilationEnvironment* LoadSource(const char * source, const char * fileName, SpireDiagnosticSink * sink)
		{
			spPushContext(context);
			spLoadModuleLibraryFromSource(context, source, fileName, sink);
			if (spDiagnosticSinkHasAnyErrors(sink))
			{
				spPopContext(context);
				ReportErrors();
				return nullptr;
			}
			auto rs = spGetCurrentEnvironment(context);
			spPopContext(context);
			return rs;
		}
		void UnloadSource(SpireCompilationEnvironment * env)
		{
			spReleaseEnvironment(env);
		}
	};
	
	ShaderRepositoryImpl * ShaderRepository::impl = nullptr;

	SpireCompilationContext * ShaderRepository::GetContext()
	{
		return impl->GetContext();
	}

	SpireModule * ShaderRepository::LoadLibraryModule(const char * name)
	{
		return impl->LoadLibraryModule(name);
	}

	SpireCompilationEnvironment * ShaderRepository::LoadSource(const char * source, const char * fileName, SpireDiagnosticSink * sink)
	{
		return impl->LoadSource(source, fileName, sink);
	}

	void ShaderRepository::UnloadSource(SpireCompilationEnvironment * env)
	{
		impl->UnloadSource(env);
	}

	ShaderRepository ShaderRepository::Instance()
	{
		if (!impl)
			impl = new ShaderRepositoryImpl();
		return ShaderRepository();
	}

	void ShaderRepository::Close()
	{
		delete impl;
		impl = nullptr;
	}
}


