#pragma once

#include "VulkanHelper.h"
#include <array>

namespace Wolf
{
	class Pipeline
	{
	public:
		Pipeline() = default;
		~Pipeline();

		void initialize(VkDevice device, VkRenderPass renderPass, std::string vertexShader, std::string geometryShader, std::string fragmentShader, 
			std::string tessellationControlShader, std::string tessellationEvaluationShader, std::vector<VkVertexInputBindingDescription> vertexInputDescription,
			std::vector<VkVertexInputAttributeDescription> attributeInputDescription, VkExtent2D extent, VkSampleCountFlagBits msaaSamples, std::vector<bool> alphaBlending,
			VkDescriptorSetLayout* descriptorSetLayout, std::array<float, 2> viewportScale, std::array<float, 2> viewportOffset, VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 
			VkBool32 enableDepthTesting = VK_TRUE, bool addColors = false, bool enableConservativeRasterization = false, float maxExtraPrimitiveOverestimationSize = 0.75f,
			VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL);
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
