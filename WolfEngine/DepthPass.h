#pragma once

#include "WolfEngine.h"
#include "RenderPass.h"
#include "UniformBufferObject.h"
#include "Model.h"

namespace Wolf
{
	class DepthPass
	{
	public:
		DepthPass(Wolf::WolfInstance* engineInstance, Wolf::Scene* scene, int commandBufferID, bool outputIsSwapChain, VkExtent2D extent, VkSampleCountFlagBits sampleCount,
			Model* model, glm::mat4 mvp, bool useAsStorage);
		~DepthPass() = default;

		void update(glm::mat4 mvp);

	private:
		Wolf::WolfInstance* m_engineInstance;
		Wolf::Scene* m_scene;

		int m_renderPassID = -1;
		
		Attachment m_attachment;
		std::vector<VkClearValue> m_clearValues;

		UniformBufferObject* m_uboMVP;
		glm::mat4 m_mvp;
		int m_rendererID;

		VkSampleCountFlagBits m_sampleCount = VK_SAMPLE_COUNT_1_BIT;
	};
}
