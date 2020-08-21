#include "DescriptorSet.h"

#include "Debug.h"

VkDescriptorSet Wolf::createDescriptorSet(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, VkDescriptorPool descriptorPool, DescriptorSetCreateInfo descriptorSetCreateInfo)
{
	VkDescriptorSetLayout layouts[] = { descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	VkDescriptorSet descriptorSet;
	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
		Debug::sendError("Error : allocate descriptor set");

	std::vector<VkWriteDescriptorSet> descriptorWrites;

	std::vector<std::vector<VkDescriptorBufferInfo>> descriptorBufferInfos(descriptorSetCreateInfo.descriptorBuffers.size());
	for (int i(0); i < descriptorBufferInfos.size(); ++i)
	{
		descriptorBufferInfos[i].resize(descriptorSetCreateInfo.descriptorBuffers[i].first.size());
		for (int j(0); j < descriptorBufferInfos[i].size(); ++j)
		{
			descriptorBufferInfos[i][j].buffer = descriptorSetCreateInfo.descriptorBuffers[i].first[j].buffer;
			descriptorBufferInfos[i][j].offset = 0;
			descriptorBufferInfos[i][j].range = descriptorSetCreateInfo.descriptorBuffers[i].first[j].size;
		}

		VkWriteDescriptorSet descriptorWrite;
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = descriptorSetCreateInfo.descriptorBuffers[i].second.binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = descriptorSetCreateInfo.descriptorBuffers[i].second.descriptorType;
		descriptorWrite.descriptorCount = static_cast<uint32_t>(descriptorBufferInfos[i].size());
		descriptorWrite.pBufferInfo = descriptorBufferInfos[i].data();
		descriptorWrite.pNext = NULL;

		descriptorWrites.push_back(descriptorWrite);
	}

	std::vector<std::vector<VkDescriptorImageInfo>> descriptorImageInfos(descriptorSetCreateInfo.descriptorImages.size());
	for (int i(0); i < descriptorImageInfos.size(); ++i)
	{
		descriptorImageInfos[i].resize(descriptorSetCreateInfo.descriptorImages[i].first.size());
		for (int j(0); j < descriptorImageInfos[i].size(); ++j)
		{
			if(descriptorSetCreateInfo.descriptorImages[i].first[j].image)
			{
				descriptorImageInfos[i][j].imageLayout = descriptorSetCreateInfo.descriptorImages[i].first[j].image->getImageLayout();
				descriptorImageInfos[i][j].imageView = descriptorSetCreateInfo.descriptorImages[i].first[j].image->getImageView();
			}
			if(descriptorSetCreateInfo.descriptorImages[i].first[j].sampler)
				descriptorImageInfos[i][j].sampler = descriptorSetCreateInfo.descriptorImages[i].first[j].sampler->getSampler();
		}

		VkWriteDescriptorSet descriptorWrite;
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = descriptorSetCreateInfo.descriptorImages[i].second.binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = descriptorSetCreateInfo.descriptorImages[i].second.descriptorType;
		descriptorWrite.descriptorCount = static_cast<uint32_t>(descriptorImageInfos[i].size());
		descriptorWrite.pImageInfo = descriptorImageInfos[i].data();
		descriptorWrite.pNext = NULL;

		descriptorWrites.push_back(descriptorWrite);
	}

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	return descriptorSet;
}

void Wolf::DescriptorSetGenerator::addUniformBuffer(UniformBuffer* ubo, VkShaderStageFlags accessibility,
	uint32_t binding)
{
	DescriptorSetCreateInfo::BufferData bufferData;
	bufferData.buffer = ubo->getUniformBuffer();
	bufferData.size = ubo->getSize();

	DescriptorLayout descriptorLayout;
	descriptorLayout.accessibility = accessibility;
	descriptorLayout.binding = binding;
	descriptorLayout.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	
	m_descriptorSetCreateInfo.descriptorBuffers.push_back({
			{ bufferData },
			descriptorLayout
		});
}

void Wolf::DescriptorSetGenerator::addImages(std::vector<Image*> images, VkDescriptorType descriptorType,
	VkShaderStageFlags accessibility, uint32_t binding)
{
	std::vector<DescriptorSetCreateInfo::ImageData> imageData(images.size());
	for(int i(0); i< images.size(); ++i)
	{
		imageData[i].image = images[i];
	}

	DescriptorLayout descriptorLayout;
	descriptorLayout.accessibility = accessibility;
	descriptorLayout.binding = binding;
	descriptorLayout.descriptorType = descriptorType;
	descriptorLayout.count = static_cast<uint32_t>(images.size());

	m_descriptorSetCreateInfo.descriptorImages.emplace_back(
		imageData,
		descriptorLayout
	);
}

void Wolf::DescriptorSetGenerator::addCombinedImageSampler(Image* image, Sampler* sampler,
	VkShaderStageFlags accessibility, uint32_t binding)
{
	DescriptorSetCreateInfo::ImageData imageData{};
	imageData.image = image;
	imageData.sampler = sampler;

	DescriptorLayout descriptorLayout;
	descriptorLayout.accessibility = accessibility;
	descriptorLayout.binding = binding;
	descriptorLayout.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	m_descriptorSetCreateInfo.descriptorImages.push_back({
			{ imageData },
			descriptorLayout
		});
}

void Wolf::DescriptorSetGenerator::addSampler(Sampler* sampler, VkShaderStageFlags accessibility, uint32_t binding)
{
	DescriptorSetCreateInfo::ImageData imageData{};
	imageData.sampler = sampler;

	DescriptorLayout descriptorLayout;
	descriptorLayout.accessibility = accessibility;
	descriptorLayout.binding = binding;
	descriptorLayout.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;

	m_descriptorSetCreateInfo.descriptorImages.push_back({
			{ imageData },
			descriptorLayout
		});
}

std::vector<Wolf::DescriptorLayout> Wolf::DescriptorSetGenerator::getDescriptorLayouts()
{
	std::vector<Wolf::DescriptorLayout> r(m_descriptorSetCreateInfo.descriptorImages.size() + m_descriptorSetCreateInfo.descriptorBuffers.size());
	
	int i = 0;
	for (auto& descriptorImage : m_descriptorSetCreateInfo.descriptorImages)
		r[i++] = descriptorImage.second;
	for (auto& descriptorBuffer : m_descriptorSetCreateInfo.descriptorBuffers)
		r[i++] = descriptorBuffer.second;

	return r;
}
