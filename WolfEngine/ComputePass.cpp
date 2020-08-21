#include "ComputePass.h"

#include <utility>

#include "Debug.h"

Wolf::ComputePass::ComputePass(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, std::string computeShader,
                               DescriptorSetCreateInfo descriptorSetCreateInfo)
{
	m_device = device;
	m_physicalDevice = physicalDevice;
	m_commandPool = commandPool;

	m_descriptorSetCreateInfo = descriptorSetCreateInfo;

	// Global bindings
	std::vector<VkDescriptorSetLayoutBinding> bindings;

	for (auto descriptorLayout : descriptorSetCreateInfo.descriptorBuffers)
	{
		VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
		descriptorSetLayoutBinding.binding = descriptorLayout.second.binding;
		descriptorSetLayoutBinding.descriptorType = descriptorLayout.second.descriptorType;
		descriptorSetLayoutBinding.descriptorCount = descriptorLayout.second.count;
		descriptorSetLayoutBinding.stageFlags = descriptorLayout.second.accessibility;
		descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

		bindings.push_back(descriptorSetLayoutBinding);
	}

	for (auto descriptorLayout : descriptorSetCreateInfo.descriptorImages)
	{
		VkDescriptorSetLayoutBinding descriptorSetLayoutBinding = {};
		descriptorSetLayoutBinding.binding = descriptorLayout.second.binding;
		descriptorSetLayoutBinding.descriptorType = descriptorLayout.second.descriptorType;
		descriptorSetLayoutBinding.descriptorCount = descriptorLayout.second.count;
		descriptorSetLayoutBinding.stageFlags = descriptorLayout.second.accessibility;
		descriptorSetLayoutBinding.pImmutableSamplers = nullptr;

		bindings.push_back(descriptorSetLayoutBinding);
	}

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
		Debug::sendError("Error : create descriptor set layout");
	
	/* Create pipeline */
	m_pipeline = std::make_unique<Pipeline>(device, std::move(computeShader), &m_descriptorSetLayout);
}

void Wolf::ComputePass::create(VkDescriptorPool descriptorPool)
{
	m_descriptorSet = createDescriptorSet(m_device, m_descriptorSetLayout, descriptorPool, m_descriptorSetCreateInfo);
}

void Wolf::ComputePass::record(VkCommandBuffer commandBuffer, VkExtent2D extent, VkExtent3D dispatchGroups)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline->getPipeline());
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline->getPipelineLayout(), 0, 1, &m_descriptorSet, 0, 0);
	uint32_t groupSizeX = extent.width % dispatchGroups.width != 0 ? extent.width / dispatchGroups.width + 1 : extent.width / dispatchGroups.width;
	uint32_t groupSizeY = extent.height % dispatchGroups.height != 0 ? extent.height / dispatchGroups.height + 1 : extent.height / dispatchGroups.height;
	vkCmdDispatch(commandBuffer, groupSizeX, groupSizeY, dispatchGroups.depth);
}
