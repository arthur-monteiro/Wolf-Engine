#pragma once

#include "WolfEngine.h"
#include "RenderPass.h"
#include "UniformBufferObject.h"
#include "Model.h"

namespace Wolf
{
	class GBuffer
	{
	public:
		GBuffer(Wolf::WolfInstance* engineInstance, Wolf::Scene* scene, int commandBufferID, VkExtent2D extent, VkSampleCountFlagBits sampleCount,
			Model* model, glm::mat4 mvp, bool useDepthAsStorage);

		void updateMVPMatrix(glm::mat4 m, glm::mat4 v, glm::mat4 p);
		
	private:
		Wolf::WolfInstance* m_engineInstance;
		Wolf::Scene* m_scene;

		int m_renderPassID = -1;

		std::vector<Attachment> m_attachments;
		std::vector<VkClearValue> m_clearValues;

		UniformBufferObject* m_uboMVP;
		std::array<glm::mat4, 3> m_mvp;
		int m_rendererID;

		VkSampleCountFlagBits m_sampleCount = VK_SAMPLE_COUNT_1_BIT;
	};
}

