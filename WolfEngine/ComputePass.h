#pragma once
#include "VulkanHelper.h"
#include "FrameBuffer.h"
#include "Attachment.h"
#include "Semaphore.h"
#include "Image.h"
#include "UniformBufferObject.h"
#include "Texture.h"
#include "Pipeline.h"
#include "VulkanElement.h"

namespace Wolf
{
	class ComputePass : public VulkanElement
	{
	public:
		ComputePass(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, std::string computeShader,
			std::vector<std::pair<UniformBufferObject*, UniformBufferObjectLayout>> ubos, std::vector<std::pair<Image*, ImageLayout>> images);

		void create(VkDescriptorPool descriptorPool);
		void record(VkCommandBuffer commandBuffer, VkExtent2D extent, VkExtent3D dispatchGroups);
		
	private:
		Pipeline m_pipeline;

		// Data
		std::vector<std::pair<UniformBufferObject*, UniformBufferObjectLayout>> m_ubos;
		std::vector<std::pair<Image*, ImageLayout>> m_images;

		VkDescriptorSet m_descriptorSet;
		VkDescriptorSetLayout m_descriptorSetLayout;
	};
}
