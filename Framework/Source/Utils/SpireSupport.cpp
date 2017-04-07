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

#include "Framework.h"
#include "SpireSupport.h"
#include "API/VertexLayout.h"
#include <map>
#include <sstream>

#include "Externals/Spire/SpireAllSource.h"

namespace Falcor
{
	class ShaderRepositoryImpl
	{
    // Note(tfoley): `private` only exists to waste people's time. These members are de facto file-local anyway...
	public:
		SpireCompilationContext * context;
		SpireCompilationEnvironment * libEnv;
		SpireDiagnosticSink * sink;
		std::map<std::string, SpireModule*> vertexModules;
        std::map<SpireModule*, ProgramReflection::ComponentClassReflection::SharedPtr> componentClasses;

		void ReportErrors(SpireDiagnosticSink* sink)
		{
			int size = spGetDiagnosticOutput(sink, nullptr, 0);
			std::vector<char> buffer;
			buffer.resize(size);
			spGetDiagnosticOutput(sink, buffer.data(), size);
			logError(std::string("Spire compilation failed:\n") + buffer.data(), true);
		}
		void ReportErrors()
        {
            ReportErrors(sink);
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

			const char * libFiles[] = { "StandardPipeline.spire", "BSDFs.spire", "Helpers.spire", "SharedLib.spire" };
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
		SpireModule* CreateLibraryModuleFromSource(const char * source, const char * moduleName)
		{
			spEnvLoadModuleLibraryFromSource(libEnv, source, moduleName, sink);
			if (spDiagnosticSinkHasAnyErrors(sink))
			{
				ReportErrors();
				return nullptr;
			}
			else
			{
				return spEnvFindModule(libEnv, moduleName);
			}
		}
		SpireModule* GetVertexModule(VertexLayout* vertLayout)
		{
			auto bc = vertLayout->getBufferCount();
			std::string hashStr;
			for (auto i = 0u; i < bc; i++)
			{
				auto bufLayout = vertLayout->getBufferLayout(i);
				for (auto j = 0u; j < bufLayout->getElementCount(); j++)
				{
					hashStr += bufLayout->getElementName(j);
					hashStr += ",";
				}
			}
			auto module = vertexModules.find(hashStr);
			if (module != vertexModules.end())
			{
				return module->second;
			}
			auto index = vertexModules.size();
			std::stringstream moduleSrc;
			moduleSrc << "module VertexFormat" << index << " implements IVertexAttribs\n{\n";
			bool hasBitangent = false;
			bool hasColor = false;
            bool hasTexCoord = false;
			for (auto i = 0u; i < bc; i++)
			{
				auto bufLayout = vertLayout->getBufferLayout(i);
				for (auto j = 0u; j < bufLayout->getElementCount(); j++)
				{
					moduleSrc << "\tpublic @MeshVertex ";
					auto format = bufLayout->getElementFormat(j);
					switch (format)
					{
					case ResourceFormat::Alpha32Float:
					case ResourceFormat::Alpha8Unorm:
					case ResourceFormat::R8Snorm:
					case ResourceFormat::R8Unorm:
					case ResourceFormat::R16Float:
					case ResourceFormat::R16Unorm:
					case ResourceFormat::R16Snorm:
					case ResourceFormat::R32Float:
						moduleSrc << "float";
						break;
					case ResourceFormat::RG8Snorm:
					case ResourceFormat::RG8Unorm:
					case ResourceFormat::RG16Float:
					case ResourceFormat::RG16Unorm:
					case ResourceFormat::RG16Snorm:
					case ResourceFormat::RG32Float:
						moduleSrc << "float2";
						break;
					case ResourceFormat::R5G6B5Unorm:
					case ResourceFormat::RGB16Float:
					case ResourceFormat::RGB16Snorm:
					case ResourceFormat::RGB16Unorm:
					case ResourceFormat::RGB32Float:
						moduleSrc << "float3";
						break;
					case ResourceFormat::RGBA16Float:
					case ResourceFormat::RGBA32Float:
					case ResourceFormat::RGBA16Unorm:
					case ResourceFormat::RGBA8Snorm:
					case ResourceFormat::RGBA8Unorm:
						moduleSrc << "float4";
						break;
					case ResourceFormat::R8Int:
					case ResourceFormat::R16Int:
					case ResourceFormat::R32Int:
						moduleSrc << "int";
						break;
					case ResourceFormat::RG8Int:
					case ResourceFormat::RG16Int:
					case ResourceFormat::RG32Int:
						moduleSrc << "int2";
						break;
					case ResourceFormat::RG8Uint:
					case ResourceFormat::RG16Uint:
					case ResourceFormat::RG32Uint:
						moduleSrc << "uint2";
						break;
					case ResourceFormat::RGB16Int:
					case ResourceFormat::RGB32Int:
						moduleSrc << "int3";
						break;
					case ResourceFormat::RGB16Uint:
					case ResourceFormat::RGB32Uint:
						moduleSrc << "uint3";
						break;
					case ResourceFormat::RGBA8Int:
					case ResourceFormat::RGBA16Int:
					case ResourceFormat::RGBA32Int:
						moduleSrc << "int4";
						break;
					case ResourceFormat::RGBA8Uint:
					case ResourceFormat::RGBA16Uint:
					case ResourceFormat::RGBA32Uint:
						moduleSrc << "uint4";
						break;
					}
					moduleSrc << " " << bufLayout->getElementName(j) << ";\n";
                    if( bufLayout->getElementName(j) == VERTEX_TEXCOORD_NAME )
                    {
						moduleSrc << "\tpublic vec2 vertUV = " << VERTEX_TEXCOORD_NAME << ".xy;\n";
                        hasTexCoord = true;
                    }
					else if (bufLayout->getElementName(j) == VERTEX_POSITION_NAME)
						moduleSrc << "\tpublic vec3 vertPos = " << VERTEX_POSITION_NAME << ";\n";
					else if (bufLayout->getElementName(j) == VERTEX_NORMAL_NAME)
						moduleSrc << "\tpublic vec3 vertNormal = " << VERTEX_NORMAL_NAME << ";\n";
					else if (bufLayout->getElementName(j) == VERTEX_BITANGENT_NAME)
					{
						moduleSrc << "\tpublic vec3 vertBitangent = " << VERTEX_BITANGENT_NAME << ";\n";
						hasBitangent = true;
					}
					else if (bufLayout->getElementName(j) == VERTEX_DIFFUSE_COLOR_NAME)
					{
						moduleSrc << "\tpublic vec3 vertColor = " << VERTEX_DIFFUSE_COLOR_NAME << ";\n";
						hasColor = true;
					}
				}
			}
			if (!hasBitangent)
				moduleSrc << "\tpublic vec3 vertBitangent = vec3(1.0f, 0.0f, 0.0f);\n";
			if (!hasColor)
				moduleSrc << "\tpublic vec3 vertColor = vec3(1.0, 0.0, 0.0);\n";
			if (!hasTexCoord)
				moduleSrc << "\tpublic vec2 vertUV = vec2(0.0);\n";
			moduleSrc << "}";
			spEnvLoadModuleLibraryFromSource(libEnv, moduleSrc.str().c_str(), "VertexModule", sink);
			auto moduleHandle = spEnvFindModule(libEnv, (std::string("VertexFormat") + std::to_string(index)).c_str());
			vertexModules[hashStr] = moduleHandle;
			return moduleHandle;
		}
		SpireCompilationEnvironment* LoadSource(const char * source, const char * fileName, SpireDiagnosticSink * sink)
		{
			spPushContext(context);
			spLoadModuleLibraryFromSource(context, source, fileName, sink);
			if (spDiagnosticSinkHasAnyErrors(sink))
			{
				spPopContext(context);
				ReportErrors(sink);
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

	SpireModule * ShaderRepository::CreateLibraryModuleFromSource(const char * source, const char * moduleName)
	{
		return impl->CreateLibraryModuleFromSource(source, moduleName);
	}

	SpireCompilationEnvironment * ShaderRepository::LoadSource(const char * source, const char * fileName, SpireDiagnosticSink * sink)
	{
		return impl->LoadSource(source, fileName, sink);
	}

	SpireModule * ShaderRepository::GetVertexModule(VertexLayout* vertexLayout)
	{
		return impl->GetVertexModule(vertexLayout);
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

    //

    ProgramReflection::ComponentClassReflection::SharedPtr ShaderRepository::findComponentClass(SpireModule* spireComponentClass)
    {
        auto iter = impl->componentClasses.find(spireComponentClass);
        if( iter != impl->componentClasses.end() )
            return iter->second;

        auto componentClass = ProgramReflection::ComponentClassReflection::create(spireComponentClass);
        impl->componentClasses.insert(std::make_pair(spireComponentClass, componentClass));
        return componentClass;
    }


    ProgramReflection::ComponentClassReflection::SharedPtr ShaderRepository::findComponentClass(char const* name)
    {
        SpireModule* spireComponentClass = spFindModule(impl->context, name);
        if(!spireComponentClass)
            return nullptr;

        return findComponentClass(spireComponentClass);
    }

}


