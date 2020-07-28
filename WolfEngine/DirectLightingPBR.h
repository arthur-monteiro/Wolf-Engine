#pragma once

#include "WolfEngine.h"
#include "Blur.h"

namespace Wolf
{
	class DirectLightingPBR
	{
	public:
		DirectLightingPBR(Wolf::WolfInstance* engineInstance, Wolf::Scene* scene, int commandBufferID,
			VkExtent2D extent, Image* depth, Image* albedoImage, Image* normalRoughnessMetal, Image* shadowMask, Image* volumetricLight, Image* aoMaskImage,
			glm::mat4 projection, float near, float far);

		void update(glm::vec3 lightDirectionInViewPosSpace);

		Texture* getOutputTexture() { return m_outputTexture; }

	private:
		int m_computePassID;
		Texture* m_outputTexture;

		struct UBOData
		{
			glm::mat4 invProjection;
			glm::vec4 projParams;
			glm::vec4 directionDirectionalLight;
			glm::vec4 colorDirectionalLight;
		};
		UBOData m_uboData;
		UniformBufferObject* m_ubo;
	};
}
