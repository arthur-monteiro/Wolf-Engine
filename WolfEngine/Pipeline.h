#pragma once

#include "VulkanHelper.h"

namespace Wolf
{
	class Pipeline
	{
	public:
		Pipeline() = default;
		~Pipeline();

		void initialize(VkDevice device, VkRenderPass renderPass, std::string vertexShader, std::string geometryShader, std::string fragmentShader, std::vector<VkVertexInputBindingDescription> vertexInputDescription,
			std::vector<VkVertexInputAttributeDescription> attributeInputDescription, VkExtent2D extent, VkSampleCountFlagBits msaaSamples, std::vector<bool> alphaBlending,
			VkDescriptorSetLayout* descriptorSetLayout, VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VkBool32 enableDepthTesting = VK_TRUE, bool addColors = false);
		void initialize(VkDevice device, std::string computeShader, VkDescriptorSetLayout* descriptorSetLayout);

		void cleanup(VkDevice device);

		VkPipeline getPipeline() const { return m_pipeline; }
		VkPipelineLayout getPipelineLayout() const { return m_pipelineLayout; }

	private:
		VkPipelineLayout m_pipelineLayout;
		VkPipeline m_pipeline;

	public:
		static std::vector<char> readFile(const std::string& filename);
		static VkShaderModule createShaderModule(const std::vector<char>& code, VkDevice device);
	};


}
