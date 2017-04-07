#ifndef SPIRE_SUPPORT_H
#define SPIRE_SUPPORT_H

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
#pragma once

#define FALCOR_SPIRE_SUPPORTED 1 

// A helper file just so that other code is insulated from the path where
// Spire is installed

#include "../API/ProgramReflection.h"

#include "Externals/Spire/Spire.h"


namespace Falcor
{
	class ShaderRepositoryImpl;
	class VertexLayout;

	class ShaderRepository
	{
	private:
		static ShaderRepositoryImpl * impl;
		ShaderRepository() = default;
	public:
		SpireCompilationContext * GetContext();
		SpireModule* LoadLibraryModule(const char * name);
		SpireModule* CreateLibraryModuleFromSource(const char * source, const char * fileName);
		SpireCompilationEnvironment* LoadSource(const char * source, const char * fileName, SpireDiagnosticSink * sink);
		SpireModule* GetVertexModule(VertexLayout* vertexLayout);
		void UnloadSource(SpireCompilationEnvironment * env);
		static ShaderRepository Instance();
		static void Close();

        ProgramReflection::ComponentClassReflection::SharedPtr findComponentClass(SpireModule* spireComponentClass);
        ProgramReflection::ComponentClassReflection::SharedPtr findComponentClass(char const* name);
	};
}

#endif