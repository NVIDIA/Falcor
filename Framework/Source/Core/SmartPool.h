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
#include <queue>
#include "GpuFence.h"

namespace Falcor
{
	template<typename ObjectType>
	class SmartPool : public std::enable_shared_from_this<SmartPool<ObjectType>>
	{
	public:
		using SharedPtr = std::shared_ptr<SmartPool<ObjectType>>;
		using SharedConstPtr = std::shared_ptr<const SmartPool<ObjectType>>;
		
		static SharedPtr create(GpuFence::SharedPtr& pFence) { return SharedPtr(new SmartPool(pFence)); }
		ObjectType allocate()
		{
			// The queue is sorted based on time. Check if the first object is free
			Data data = mQueue.front();
			if (data.timestamp < mpFence->getGpuValue())
			{
				mQueue.pop();
			}
			else
			{
				data.alloc = newObject();
			}

			data.timestamp = mpFence->getCpuValue();
			mQueue.push(data);
			return data.alloc;
		}

	private:
		SmartPool(GpuFence::SharedPtr& pFence) : mpFence(pFence) { mQueue.push({ newObject(), mpFence->getCpuValue()}); }
		GpuFence::SharedPtr mpFence;
		ObjectType newObject();

		struct Data
		{
			ObjectType alloc;
			uint64_t timestamp;
		};

		std::queue<Data> mQueue;
	};
}
