#include "ComputePass.h"

#include <utility>

Wolf::ComputePass::ComputePass(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, std::string computeShader,
                               std::vector<std::pair<UniformBufferObject*, UniformBufferObjectLayout>> ubos, std::vector<std::pair<Image*, ImageLayout>> images)
{
	m_device = device;
	m_physicalDevice = physicalDevice;
	m_commandPool = commandPool;

	m_ubos = std::move(ubos);
	m_images = std::move(images);
	
	/* Create pipeline */
	std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayouts;
	for (int i(0); i < m_ubos.size(); ++i)
	{
		VkDescriptorSetLayoutBinding descriptorSetLayout = {};
		descriptorSetLayout.binding = m_ubos[i].second.binding;
		descriptorSetLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorSetLayout.descriptorCount = 1;
		descriptorSetLayout.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		descriptorSetLayout.pImmutableSamplers = nullptr;

		descriptorSetLayouts.push_back(descriptorSetLayout);
	}

	for (int i(0); i < m_images.size(); ++i)
	{
		VkDescriptorSetLayoutBinding descriptorSetLayout = {};
		descriptorSetLayout.binding = m_images[i].second.binding;
		descriptorSetLayout.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		descriptorSetLayout.descriptorCount = 1;
		descriptorSetLayout.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		descriptorSetLayout.pImmutableSamplers = nullptr;

		descriptorSetLayouts.push_back(descriptorSetLayout);
	}

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	layoutInfo.pBindings = descriptorSetLayouts.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
		throw std::runtime_error("Error : create descriptor set layout");

	m_pipeline.initialize(device, std::move(computeShader), &m_descriptorSetLayout);
}

void Wolf::ComputePass::create(VkDescriptorPool descriptorPool)
{
	VkDescriptorSetLayout layouts[] = { m_descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	VkResult res = vkAllocateDescriptorSets(m_device, &allocInfo, &m_descriptorSet);
	if (res != VK_SUCCESS)
		throw std::runtime_error("Error : allocate descriptor set");

	// Descriptor Set
	std::vector<VkWriteDescriptorSet> descriptorWrites;

	std::vector<VkDescriptorBufferInfo> bufferInfo(m_ubos.size());
	for (int i(0); i < m_ubos.size(); ++i)
	{
		bufferInfo[i].buffer = m_ubos[i].first->getUniformBuffer();
		bufferInfo[i].offset = 0;
		bufferInfo[i].range = m_ubos[i].first->getSize();

		VkWriteDescriptorSet descriptorWrite;
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_descriptorSet;
		descriptorWrite.dstBinding = m_ubos[i].second.binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo[i];
		descriptorWrite.pNext = NULL;

		descriptorWrites.push_back(descriptorWrite);
	}

	std::vector<VkDescriptorImageInfo> imageInfo(m_images.size());
	for (int i(0); i < m_images.size(); ++i)
	{
		imageInfo[i].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
		imageInfo[i].imageView = m_images[i].first->getImageView();
		//imageInfo[i].sampler = m_textures[i].first->getSampler();

		VkWriteDescriptorSet descriptorWrite;
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_descriptorSet;
		descriptorWrite.dstBinding = m_images[i].second.binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pImageInfo = &imageInfo[i];
		descriptorWrite.pNext = NULL;

		descriptorWrites.push_back(descriptorWrite);
	}

	vkUpdateDescriptorSets(m_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void Wolf::ComputePass::record(VkCommandBuffer commandBuffer, VkExtent2D extent, VkExtent3D dispatchGroups)
{
	vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.getPipeline());
	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipeline.getPipelineLayout(), 0, 1, &m_descriptorSet, 0, 0);
	uint32_t groupSizeX = extent.width % dispatchGroups.width != 0 ? extent.width / dispatchGroups.width + 1 : extent.width / dispatchGroups.width;
	uint32_t groupSizeY = extent.height % dispatchGroups.height != 0 ? extent.height / dispatchGroups.height + 1 : extent.height / dispatchGroups.height;
	vkCmdDispatch(commandBuffer, groupSizeX, groupSizeY, dispatchGroups.depth);
}
