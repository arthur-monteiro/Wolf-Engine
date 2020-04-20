#include "Renderer.h"

#include <utility>

Wolf::Renderer::~Renderer()
= default;

Wolf::Renderer::Renderer(VkDevice device, std::string vertexShader, std::string fragmentShader,
	std::vector<VkVertexInputBindingDescription> vertexInputDescription,
	std::vector<VkVertexInputAttributeDescription> attributeInputDescription,
	std::vector<UniformBufferObjectLayout> uniformBufferObjectLayouts, std::vector<TextureLayout> textureLayouts,
	std::vector<ImageLayout> imageLayouts, std::vector<SamplerLayout> samplerLayouts,
	std::vector<BufferLayout> bufferLayouts, std::vector<bool> alphaBlending)
{
	m_vertexShader = std::move(vertexShader);
	m_geometryShader = "";
	m_fragmentShader = std::move(fragmentShader);
	m_vertexInputDescription = std::move(vertexInputDescription);
	m_attributeInputDescription = std::move(attributeInputDescription);
	m_alphaBlending = std::move(alphaBlending);

	createDescriptorSetLayout(device, std::move(uniformBufferObjectLayouts), std::move(textureLayouts), std::move(imageLayouts), std::move(samplerLayouts),
		std::move(bufferLayouts));
}

Wolf::Renderer::Renderer(VkDevice device, std::string vertexShader, std::string geometryShader,
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

	createDescriptorSetLayout(device, std::move(uniformBufferObjectLayouts), std::move(textureLayouts), std::move(imageLayouts), std::move(samplerLayouts),
		std::move(bufferLayouts));
}

Wolf::Renderer::Renderer(VkDevice device, std::string vertexShader,
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

	createDescriptorSetLayout(device, std::move(uniformBufferObjectLayouts), std::move(textureLayouts), {}, {}, {}); // !! change definition to allow image and sampler separated
}

void Wolf::Renderer::create(VkDevice device, VkRenderPass renderPass, VkExtent2D extent, VkSampleCountFlagBits msaa, VkDescriptorPool descriptorPool)
{
	if (!m_pipelineCreated)
	{
		m_pipeline.initialize(device, renderPass, m_vertexShader, m_geometryShader, m_fragmentShader, m_vertexInputDescription, m_attributeInputDescription, extent, msaa, m_alphaBlending, &m_descriptorSetLayout);
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

	if (!imageLayouts.empty())
	{
		VkDescriptorSetLayoutBinding imageLayoutBinding = {};
		imageLayoutBinding.binding = imageLayouts[0].binding;
		imageLayoutBinding.descriptorCount = static_cast<uint32_t>(imageLayouts.size());
		imageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
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

	std::vector<VkDescriptorImageInfo> imageInfo(images.size());
	for (int i(0); i < images.size(); ++i)
	{
		imageInfo[i].imageLayout = images[i].first->getImageLayout();
		imageInfo[i].imageView = images[i].first->getImageView();
	}

	if (!images.empty())
	{
		VkWriteDescriptorSet descriptorWrite;
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = descriptorSet;
		descriptorWrite.dstBinding = images[0].second.binding;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		descriptorWrite.descriptorCount = static_cast<uint32_t>(imageInfo.size());
		descriptorWrite.pImageInfo = imageInfo.data();
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
