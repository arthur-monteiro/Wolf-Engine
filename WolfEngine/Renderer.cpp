#include "Renderer.h"

#include <utility>

Wolf::Renderer::~Renderer()
= default;

Wolf::Renderer::Renderer(VkDevice device, VkExtent2D extent, std::string vertexShader, std::string fragmentShader,
	std::vector<VkVertexInputBindingDescription> vertexInputDescription,
	std::vector<VkVertexInputAttributeDescription> attributeInputDescription,
	std::vector<UniformBufferObjectLayout> uniformBufferObjectLayouts, std::vector<TextureLayout> textureLayouts,
	std::vector<ImageLayout> imageLayouts, std::vector<SamplerLayout> samplerLayouts,
	std::vector<BufferLayout> bufferLayouts, std::vector<bool> alphaBlending, bool enableDepthTesting, bool enableConservativeRasterization)
{
	m_vertexShader = std::move(vertexShader);
	m_geometryShader = "";
	m_fragmentShader = std::move(fragmentShader);
	m_vertexInputDescription = std::move(vertexInputDescription);
	m_attributeInputDescription = std::move(attributeInputDescription);
	m_alphaBlending = std::move(alphaBlending);
	m_extent = extent;
	m_primitiveTopoly = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	m_enableDepthTesting = enableDepthTesting;
	m_enableConservativeRasterization = enableConservativeRasterization;

	createDescriptorSetLayout(device, std::move(uniformBufferObjectLayouts), std::move(textureLayouts), std::move(imageLayouts), std::move(samplerLayouts),
		std::move(bufferLayouts));
}

Wolf::Renderer::Renderer(VkDevice device, VkExtent2D extent, std::string vertexShader, std::string geometryShader,
	std::string fragmentShader, std::vector<VkVertexInputBindingDescription> vertexInputDescription,
	std::vector<VkVertexInputAttributeDescription> attributeInputDescription,
	std::vector<UniformBufferObjectLayout> uniformBufferObjectLayouts, std::vector<TextureLayout> textureLayouts,
	std::vector<ImageLayout> imageLayouts, std::vector<SamplerLayout> samplerLayouts,
	std::vector<BufferLayout> bufferLayouts, std::vector<bool> alphaBlending)
{
	m_vertexShader = std::move(vertexShader);
	m_geometryShader = std::move(geometryShader);
	m_fragmentShader = std::move(fragmentShader);
	m_vertexInputDescription = std::move(vertexInputDescription);
	m_attributeInputDescription = std::move(attributeInputDescription);
	m_alphaBlending = std::move(alphaBlending);
	m_extent = extent;
	m_primitiveTopoly = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	m_enableDepthTesting = true;
	m_enableConservativeRasterization = false;
	
	createDescriptorSetLayout(device, std::move(uniformBufferObjectLayouts), std::move(textureLayouts), std::move(imageLayouts), std::move(samplerLayouts),
		std::move(bufferLayouts));
}

Wolf::Renderer::Renderer(VkDevice device, VkExtent2D extent, std::string vertexShader,
	std::vector<VkVertexInputBindingDescription> vertexInputDescription,
	std::vector<VkVertexInputAttributeDescription> attributeInputDescription,
	std::vector<UniformBufferObjectLayout> uniformBufferObjectLayouts, std::vector<TextureLayout> textureLayouts,
	std::vector<bool> alphaBlending)
{
	m_vertexShader = std::move(vertexShader);
	m_geometryShader = "";
	m_fragmentShader = "";
	m_vertexInputDescription = std::move(vertexInputDescription);
	m_attributeInputDescription = std::move(attributeInputDescription);
	m_alphaBlending = std::move(alphaBlending);
	m_extent = extent;
	m_primitiveTopoly = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	m_enableDepthTesting = true;
	m_enableConservativeRasterization = false;

	createDescriptorSetLayout(device, std::move(uniformBufferObjectLayouts), std::move(textureLayouts), {}, {}, {}); // !! change definition to allow image and sampler separated
}

void Wolf::Renderer::create(VkDevice device, VkRenderPass renderPass, VkSampleCountFlagBits msaa, VkDescriptorPool descriptorPool)
{
	if (!m_pipelineCreated)
	{
		m_pipeline.initialize(device, renderPass, m_vertexShader, m_geometryShader, m_fragmentShader, m_vertexInputDescription, m_attributeInputDescription, 
			m_extent, msaa, m_alphaBlending, &m_descriptorSetLayout, m_viewportScale, m_viewportOffset, m_primitiveTopoly, m_enableDepthTesting, false, m_enableConservativeRasterization);
		m_pipelineCreated = true;
	}

	for(size_t i(0); i < m_meshes.size(); ++i)
	{
		if(m_meshes[i].descriptorSet == VK_NULL_HANDLE && m_meshes[i].needDescriptorSet())
		{
			m_meshes[i].descriptorSet = createDescriptorSet(device, descriptorPool, m_meshes[i].ubos, m_meshes[i].textures, m_meshes[i].images, m_meshes[i].samplers, m_meshes[i].buffers);
		}
	}
}

void Wolf::Renderer::destroyPipeline(VkDevice device)
{
	if (!m_pipelineCreated)
		return;

	m_pipeline.cleanup(device);
	m_pipelineCreated = false;
}

void Wolf::Renderer::setViewport(std::array<float, 2> viewportScale, std::array<float, 2> viewportOffset)
{
	m_viewportScale = viewportScale;
	m_viewportOffset = viewportOffset;
}

int Wolf::Renderer::addMesh(VertexBuffer vertexBuffer,
                            std::vector<std::pair<UniformBufferObject*, UniformBufferObjectLayout>> ubos, std::vector<std::pair<Texture*, TextureLayout>> textures,
                            std::vector<std::pair<Image*, ImageLayout>> images, std::vector<std::pair<Sampler*, SamplerLayout>> samplers, std::vector<std::pair<VkBuffer, BufferLayout>> buffers)
{
	m_meshes.emplace_back(vertexBuffer, ubos, textures, images, samplers, buffers);

	return static_cast<int>(m_meshes.size() - 1);
}

int Wolf::Renderer::addMeshInstancied(VertexBuffer vertexBuffer,
                                      InstanceBuffer instanceBuffer, std::vector<std::pair<UniformBufferObject*, UniformBufferObjectLayout>> ubos,
                                      std::vector<std::pair<Texture*, TextureLayout>> textures, std::vector<std::pair<Image*, ImageLayout>> images, std::vector<std::pair<Sampler*, SamplerLayout>> samplers, std::vector<std::pair<VkBuffer, BufferLayout>> buffers)
{
	m_meshes.emplace_back(vertexBuffer, ubos, textures, images, samplers, buffers, instanceBuffer);

	return static_cast<int>(m_meshes.size() - 1);
}

void Wolf::Renderer::reloadMeshVertexBuffer(VkDevice device, VertexBuffer vertexBuffer, int meshID)
{
	//m_meshes[meshID].first = vertexBuffer;
}

void Wolf::Renderer::cleanup(VkDevice device, VkDescriptorPool descriptorPool)
{
	for (size_t i(0); i < m_meshes.size(); ++i)
		vkFreeDescriptorSets(device, descriptorPool, 1, &m_meshes[i].descriptorSet);
	vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
	destroyPipeline(device);

	m_meshes.clear();
	m_meshesInstancied.clear();
}

std::vector<std::tuple<Wolf::VertexBuffer, Wolf::InstanceBuffer, VkDescriptorSet>> Wolf::Renderer::getMeshes()
{
	std::vector<std::tuple<Wolf::VertexBuffer, Wolf::InstanceBuffer, VkDescriptorSet>> r(m_meshes.size());
	for(size_t i(0); i < m_meshes.size(); ++i)
	{
		r[i] = std::make_tuple(m_meshes[i].vertexBuffer, m_meshes[i].instanceBuffer, m_meshes[i].descriptorSet);
	}

	return r;
}

void Wolf::Renderer::createDescriptorSetLayout(VkDevice device, std::vector<UniformBufferObjectLayout> uniformBufferObjectLayouts, std::vector<TextureLayout> textureLayouts,
                                               std::vector<ImageLayout> imageLayouts, std::vector<SamplerLayout> samplerLayouts, std::vector<BufferLayout> bufferLayouts)
{
	std::vector<VkDescriptorSetLayoutBinding> bindings;
	for (auto& uniformBufferObjectLayout : uniformBufferObjectLayouts)
	{
		VkDescriptorSetLayoutBinding uboLayoutBinding = {};
		uboLayoutBinding.binding = uniformBufferObjectLayout.binding;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = uniformBufferObjectLayout.accessibility;
		uboLayoutBinding.pImmutableSamplers = nullptr;

		bindings.push_back(uboLayoutBinding);
	}

	for (auto& textureLayout : textureLayouts)
	{
		VkDescriptorSetLayoutBinding imageSamplerLayoutBinding = {};
		imageSamplerLayoutBinding.binding = textureLayout.binding;
		imageSamplerLayoutBinding.descriptorCount = static_cast<uint32_t>(1);
		imageSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		imageSamplerLayoutBinding.stageFlags = textureLayout.accessibility;
		imageSamplerLayoutBinding.pImmutableSamplers = nullptr;

		bindings.push_back(imageSamplerLayoutBinding);
	}

	int sampledImageBinding = -1;
	int nbSampledImage = 0;
	for (auto& imageLayout : imageLayouts)
	{
		if (imageLayout.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
		{
			if(sampledImageBinding == -1)
				sampledImageBinding = imageLayout.binding;
			nbSampledImage++;
		}
	}
	if (sampledImageBinding >= 0)
	{
		VkDescriptorSetLayoutBinding imageLayoutBinding = {};
		imageLayoutBinding.binding = sampledImageBinding;
		imageLayoutBinding.descriptorCount = static_cast<uint32_t>(nbSampledImage);
		imageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		imageLayoutBinding.stageFlags = imageLayouts[0].accessibility;
		imageLayoutBinding.pImmutableSamplers = nullptr;

		bindings.push_back(imageLayoutBinding);
	}

	int storageImageBinding = -1;
	int nbStorageImage = 0;
	for (auto& imageLayout : imageLayouts)
	{
		if (imageLayout.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		{
			if (storageImageBinding == -1)
				storageImageBinding = imageLayout.binding;
			nbStorageImage++;
		}
	}
	if (storageImageBinding >= 0)
	{
		VkDescriptorSetLayoutBinding imageLayoutBinding = {};
		imageLayoutBinding.binding = storageImageBinding;
		imageLayoutBinding.descriptorCount = static_cast<uint32_t>(nbStorageImage);
		imageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		imageLayoutBinding.stageFlags = imageLayouts[0].accessibility;
		imageLayoutBinding.pImmutableSamplers = nullptr;

		bindings.push_back(imageLayoutBinding);
	}

	if (!samplerLayouts.empty())
	{
		VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
		samplerLayoutBinding.binding = samplerLayouts[0].binding;
		samplerLayoutBinding.descriptorCount = static_cast<uint32_t>(samplerLayouts.size());
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		samplerLayoutBinding.stageFlags = samplerLayouts[0].accessibility;
		samplerLayoutBinding.pImmutableSamplers = nullptr;

		bindings.push_back(samplerLayoutBinding);
	}

	for (auto& bufferLayout : bufferLayouts)
	{
		VkDescriptorSetLayoutBinding bufferLayoutBinding = {};
		bufferLayoutBinding.binding = bufferLayout.binding;
		bufferLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		bufferLayoutBinding.descriptorCount = 1;
		bufferLayoutBinding.stageFlags = bufferLayout.accessibility;
		bufferLayoutBinding.pImmutableSamplers = nullptr;

		bindings.push_back(bufferLayoutBinding);
	}

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m_descriptorSetLayout) != VK_SUCCESS)
		throw std::runtime_error("Error : create descriptor set layout");
}

VkDescriptorSet Wolf::Renderer::createDescriptorSet(VkDevice device, VkDescriptorPool descriptorPool,
                                                    std::vector<std::pair<UniformBufferObject*, UniformBufferObjectLayout>> ubos, std::vector<std::pair<Texture*, TextureLayout>> textures,
                                                    std::vector<std::pair<Image*, ImageLayout>> images, std::vector<std::pair<Sampler*, SamplerLayout>> samplers, std::vector<std::pair<VkBuffer, BufferLayout>> buffers)
{
	VkDescriptorSetLayout layouts[] = { m_descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	VkDescriptorSet descriptorSet;
	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS)
		throw std::runtime_error("Error : allocate descriptor set");

	std::vector<VkWriteDescriptorSet> descriptorWrites;

	std::vector<VkDescriptorBufferInfo> uniformBufferInfo(ubos.size());
	for (int i(0); i < ubos.size(); ++i)
	{
		uniformBufferInfo[i].buffer = ubos[i].first->getUniformBuffer();
		uniformBufferInfo[i].offset = 0;
		uniformBufferInfo[i].range = ubos[i].first->getSize();

		VkWriteDescriptorSet descriptorWrite;
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = ubos[i].second.binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &uniformBufferInfo[i];
		descriptorWrite.pNext = NULL;

		descriptorWrites.push_back(descriptorWrite);
	}

	std::vector<VkDescriptorImageInfo> imageSamplerInfo(textures.size());
	for (int i(0); i < textures.size(); ++i)
	{
		imageSamplerInfo[i].imageLayout = textures[i].first->getImageLayout();
		imageSamplerInfo[i].imageView = textures[i].first->getImageView();
		imageSamplerInfo[i].sampler = textures[i].first->getSampler();
	}

	if (!textures.empty())
	{
		VkWriteDescriptorSet descriptorWrite;
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = textures[0].second.binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrite.descriptorCount = static_cast<uint32_t>(imageSamplerInfo.size());
		descriptorWrite.pImageInfo = imageSamplerInfo.data();
		descriptorWrite.pNext = NULL;

		descriptorWrites.push_back(descriptorWrite);
	}

	// Sampled Images
	int sampledImageBinding = -1;
	int nbSampledImage = 0;
	for (auto& imageLayout : images)
	{
		if (imageLayout.second.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
		{
			if (sampledImageBinding == -1)
				sampledImageBinding = imageLayout.second.binding;
			nbSampledImage++;
		}
	}
	std::vector<VkDescriptorImageInfo> imageInfo(nbSampledImage);
	int index = 0;
	for (int i(0); i < images.size(); ++i)
	{
		if(images[i].second.descriptorType != VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE)
			continue;
		imageInfo[index].imageLayout = images[i].first->getImageLayout();
		imageInfo[index].imageView = images[i].first->getImageView();

		index++;
	}

	if (nbSampledImage > 0)
	{
		VkWriteDescriptorSet descriptorWrite;
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = sampledImageBinding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		descriptorWrite.descriptorCount = static_cast<uint32_t>(imageInfo.size());
		descriptorWrite.pImageInfo = imageInfo.data();
		descriptorWrite.pNext = NULL;

		descriptorWrites.push_back(descriptorWrite);
	}

	// Storage images
	int storageImageBinding = -1;
	int nbStorageImage = 0;
	for (auto& imageLayout : images)
	{
		if (imageLayout.second.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		{
			if (storageImageBinding == -1)
				storageImageBinding = imageLayout.second.binding;
			nbStorageImage++;
		}
	}
	index = 0;
	std::vector<VkDescriptorImageInfo> storageImageInfo(nbStorageImage);
	for (int i(0); i < images.size(); ++i)
	{
		if (images[i].second.descriptorType != VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
			continue;
		storageImageInfo[index].imageLayout = images[i].first->getImageLayout();
		storageImageInfo[index].imageView = images[i].first->getImageView();

		index++;
	}

	if (nbStorageImage > 0)
	{
		VkWriteDescriptorSet descriptorWrite;
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = storageImageBinding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		descriptorWrite.descriptorCount = static_cast<uint32_t>(storageImageInfo.size());
		descriptorWrite.pImageInfo = storageImageInfo.data();
		descriptorWrite.pNext = NULL;

		descriptorWrites.push_back(descriptorWrite);
	}

	std::vector<VkDescriptorImageInfo> samplerInfo(samplers.size());
	for (int i(0); i < samplers.size(); ++i)
	{
		samplerInfo[i].sampler = samplers[i].first->getSampler();
	}

	if (!samplers.empty())
	{
		VkWriteDescriptorSet descriptorWrite;
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = samplers[0].second.binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
		descriptorWrite.descriptorCount = static_cast<uint32_t>(samplerInfo.size());
		descriptorWrite.pImageInfo = samplerInfo.data();
		descriptorWrite.pNext = NULL;

		descriptorWrites.push_back(descriptorWrite);
	}

	std::vector<VkDescriptorBufferInfo> bufferInfo(buffers.size());
	for (int i(0); i < buffers.size(); ++i)
	{
		bufferInfo[i].buffer = buffers[i].first;
		bufferInfo[i].offset = 0;
		bufferInfo[i].range = buffers[i].second.size;

		VkWriteDescriptorSet descriptorWrite;
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = buffers[i].second.binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo[i];
		descriptorWrite.pNext = NULL;

		descriptorWrites.push_back(descriptorWrite);
	}

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

	return descriptorSet;
}
