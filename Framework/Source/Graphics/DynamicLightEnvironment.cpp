#include "DynamicLightEnvironment.h"

namespace Falcor
{
	DynamicLightEnvironment::DynamicLightEnvironment()
	{
		auto compClass = ShaderRepository::Instance().findComponentClass("DynamicLighting");
		componentInstance = ComponentInstance::create(compClass);
	}

	void DynamicLightEnvironment::setAmbient(float3 ambient)
	{
		componentInstance->setVariable("ambient", ambient);
	}

	void DynamicLightEnvironment::setLightList(std::vector<Light::SharedPtr> & lights)
	{
		int lightDataSize = sizeof(LightData) - sizeof(MaterialData);
		componentInstance->setVariable("lightCount", (int)lights.size());
		buffer.resize(lightDataSize * lights.size());
		for (auto i = 0u; i < lights.size(); i++)
			memcpy(&buffer[0] + lightDataSize * i, &lights[i]->getData(), lightDataSize);
		componentInstance->setVariableBlob("lights", &buffer[0], buffer.size());
	}
}

