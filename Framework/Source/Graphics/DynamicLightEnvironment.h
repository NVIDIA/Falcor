#pragma once
#include <string>
#include <vector>
#include <map>
#include "Utils/SpireSupport.h"
#include "Graphics/Light.h"
#include "API/ConstantBuffer.h"

namespace Falcor
{
	class DynamicLightEnvironment
	{
	private:
		std::vector<unsigned char> buffer;
		ComponentInstance::SharedPtr componentInstance;
	public:
		DynamicLightEnvironment();
		ComponentInstance::SharedPtr getSpireComponentInstance() const
		{
			return componentInstance;
		}
		void setAmbient(float3 ambient);
		void setLightList(std::vector<Light::SharedPtr> & lights);
	};
}