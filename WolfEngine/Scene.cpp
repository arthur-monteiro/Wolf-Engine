#include "Scene.h"
#include "InputVertexTemplate.h"
#include "Debug.h"

Wolf::Scene::Scene(SceneCreateInfo createInfo, VkDevice device, VkPhysicalDevice physicalDevice, std::vector<Image*> swapChainImages, VkCommandPool graphicsCommandPool, VkCommandPool computeCommandPool)
{
	m_device = device;
	m_physicalDevice = physicalDevice;
	m_swapChainImages = std::move(swapChainImages);
	m_swapChainCommandType = createInfo.swapChainCommandType;

	m_graphicsCommandPool = graphicsCommandPool;
	m_computeCommandPool = computeCommandPool;
}

Wolf::Scene::Scene(SceneCreateInfo createInfo, VkDevice device, VkPhysicalDevice physicalDevice,
	std::vector<Image*> ovrSwapChainImages, std::vector<Image*> windowSwapChainImages,
	VkCommandPool graphicsCommandPool, VkCommandPool computeCommandPool)
{
	m_useOVR = true;

	m_device = device;
	m_physicalDevice = physicalDevice;
	m_swapChainImages = std::move(ovrSwapChainImages);
	m_swapChainCommandType = createInfo.swapChainCommandType;

	m_graphicsCommandPool = graphicsCommandPool;
	m_computeCommandPool = computeCommandPool;
	m_windowSwapChainImages = std::move(windowSwapChainImages);
}

int Wolf::Scene::addRenderPass(Wolf::Scene::RenderPassCreateInfo createInfo)
{
	/*std::vector<VkClearValue> clearValues;
	VkExtent2D extent;
	if(createInfo.commandBufferID == -1)
	{
		clearValues.resize(2);
		clearValues[0] = { 1.0f };
		clearValues[1] = { 1.0f, 0.0f, 0.0f, 1.0f };

		extent = m_swapChainImages[0]->getExtent();
	}
	else
	{
		extent = createInfo.extent;
	}*/

	if(createInfo.outputIsSwapChain)
	{
		createInfo.outputs.resize(2);
		
		createInfo.outputs[0].clearValue = { 1.0f };
		createInfo.outputs[0].attachment = Attachment(m_swapChainImages[0]->getExtent(), findDepthFormat(m_physicalDevice), VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

		createInfo.outputs[1].clearValue = { 1.0f, 0.0f, 0.0f, 1.0f };
		createInfo.outputs[1].attachment = Attachment(m_swapChainImages[0]->getExtent(), m_swapChainImages[0]->getFormat(), VK_SAMPLE_COUNT_1_BIT, m_useOVR ? VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
	}
	else
	{
		if (createInfo.outputs.empty())
		{
			Debug::sendError("RenderPass creation must include output");
			return -1;
		}

		bool depthAttachmentPresent = false;
		for (RenderPassOutput& output : createInfo.outputs)
		{
			if (output.attachment.usageType & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
				depthAttachmentPresent = true;

			if (output.attachment.extent.width == 0 || output.attachment.extent.height == 0)
				output.attachment.extent = m_swapChainImages[0]->getExtent();

		}
		if (!depthAttachmentPresent)
		{
			RenderPassOutput depthOutput;
			depthOutput.clearValue = { 1.0f };
			depthOutput.attachment = Attachment(createInfo.outputs[0].attachment.extent, findDepthFormat(m_physicalDevice), VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_ATTACHMENT_STORE_OP_DONT_CARE, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
			createInfo.outputs.push_back(depthOutput);
		}
	}
	
	m_sceneRenderPasses.emplace_back(createInfo.commandBufferID, createInfo.outputs, createInfo.outputIsSwapChain);

	std::vector<Attachment> attachments(0);
	for (RenderPassOutput& output : m_sceneRenderPasses[m_sceneRenderPasses.size() - 1].outputs)
		attachments.push_back(output.attachment);

	// Render pass creation
	if (m_sceneRenderPasses[m_sceneRenderPasses.size() - 1].outputIsSwapChain)
		m_sceneRenderPasses[m_sceneRenderPasses.size() - 1].renderPass = std::make_unique<RenderPass>(m_device, m_physicalDevice, m_graphicsCommandPool, attachments, m_swapChainImages);
	else
		m_sceneRenderPasses[m_sceneRenderPasses.size() - 1].renderPass = std::make_unique<RenderPass>(m_device, m_physicalDevice, m_graphicsCommandPool, attachments, std::vector<VkExtent2D>(1, attachments[0].extent));

	return static_cast<int>(m_sceneRenderPasses.size() - 1);
}

int Wolf::Scene::addRenderer(RendererCreateInfo createInfo)
{
#ifdef _DEBUG
	if(createInfo.renderPassID < 0 || createInfo.renderPassID > m_sceneRenderPasses.size() - 1)
	{
		Debug::sendError("Invalid render pass ID. ID sent = " + std::to_string(createInfo.renderPassID) + ", last render pass ID = " + std::to_string(m_sceneRenderPasses.size() - 1));
		return -1;
	}
#endif

	// Set input attribut and binding descriptions from template
	switch (createInfo.inputVerticesTemplate)
	{
	case InputVertexTemplate::POSITION_2D :
		createInfo.inputAttributeDescriptions = Vertex2D::getAttributeDescriptions(0);
		createInfo.inputBindingDescriptions = { Vertex2D::getBindingDescription(0) };
		break;
	case InputVertexTemplate::POSITION_TEXTURECOORD_2D:
		createInfo.inputAttributeDescriptions = Vertex2DTextured::getAttributeDescriptions(0);
		createInfo.inputBindingDescriptions = { Vertex2DTextured::getBindingDescription(0) };
		break;
	case InputVertexTemplate::POSITION_TEXTURECOORD_ID_2D:
		createInfo.inputAttributeDescriptions = Vertex2DTexturedWithMaterial::getAttributeDescriptions(0);
		createInfo.inputBindingDescriptions = { Vertex2DTexturedWithMaterial::getBindingDescription(0) };
		break;
	case InputVertexTemplate::FULL_3D_MATERIAL:
		createInfo.inputAttributeDescriptions = Vertex3D::getAttributeDescriptions(0);
		createInfo.inputBindingDescriptions = { Vertex3D::getBindingDescription(0) };
		break;		
	case InputVertexTemplate::NO:
		break;
	default:
		Debug::sendError("Unknown inputVerticesTemplate while creating renderer");
		break;
	}

	switch (createInfo.instanceTemplate)
	{
	case InstanceTemplate::SINGLE_ID:
		std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions = InstanceSingleID::getAttributeDescriptions(1, 2);
		std::vector<VkVertexInputBindingDescription> inputBindingDescriptions = { InstanceSingleID::getBindingDescription(1) };

		for (VkVertexInputAttributeDescription& inputAttributeDescription : inputAttributeDescriptions)
			createInfo.inputAttributeDescriptions.push_back(inputAttributeDescription);
		for (VkVertexInputBindingDescription& inputBindingDescription : inputBindingDescriptions)
			createInfo.inputBindingDescriptions.push_back(inputBindingDescription);
		break;
	}
	
	const auto r = new Renderer(m_device, createInfo.vertexShaderPath, createInfo.fragmentShaderPath, createInfo.inputBindingDescriptions, 
	                            std::move(createInfo.inputAttributeDescriptions), std::move(createInfo.uboLayouts), std::move(createInfo.textureLayouts), std::move(createInfo.imageLayouts), 
								std::move(createInfo.samplerLayouts), std::move(createInfo.bufferLayouts), { true });
	
	m_sceneRenderPasses[createInfo.renderPassID].renderers.push_back(std::unique_ptr<Renderer>(r));
	m_sceneRenderPasses[createInfo.renderPassID].renderers.back()->setViewport(createInfo.viewportScale, createInfo.viewportOffset);

	return static_cast<int>(m_sceneRenderPasses[createInfo.renderPassID].renderers.size() - 1);
}

void Wolf::Scene::addModel(AddModelInfo addModelInfo)
{
	m_descriptorPool.addUniformBuffer(static_cast<unsigned int>(addModelInfo.ubos.size()));
	m_descriptorPool.addCombinedImageSampler(static_cast<unsigned int>(addModelInfo.textures.size()));
	m_descriptorPool.addSampledImage(static_cast<unsigned int>(addModelInfo.images.size()));
	m_descriptorPool.addSampler(static_cast<unsigned int>(addModelInfo.samplers.size()));
	m_descriptorPool.addStorageBuffer(static_cast<unsigned int>(addModelInfo.buffers.size()));
	
	std::vector<VertexBuffer> vertexBuffers = addModelInfo.model->getVertexBuffers();
	for(VertexBuffer& vertexBuffer : vertexBuffers)
	{		
		m_sceneRenderPasses[addModelInfo.renderPassID].renderers[addModelInfo.rendererID]->addMesh(vertexBuffer, std::move(addModelInfo.ubos), std::move(addModelInfo.textures), std::move(addModelInfo.images), std::move(addModelInfo.samplers), 
			std::move(addModelInfo.buffers));
	}
}

void Wolf::Scene::addText(AddTextInfo addTextInfo)
{	
	// Build text
	const VkExtent2D outputExtent = m_sceneRenderPasses[addTextInfo.renderPassID].outputs[0].attachment.extent;
	addTextInfo.text->build(outputExtent, addTextInfo.font, addTextInfo.size);

	// Uniform Buffer Objects
	UniformBufferObjectLayout uboLayout{};
	uboLayout.accessibility = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayout.binding = 0;
	std::vector<std::pair<UniformBufferObject*, UniformBufferObjectLayout>> ubos = { {addTextInfo.text->getUBO(), uboLayout} };
	for (std::pair<UniformBufferObject*, UniformBufferObjectLayout>& ubo : addTextInfo.ubos)
		ubos.push_back(ubo);

	// Images
	std::vector<Image*> images = addTextInfo.font->getImages();
	std::vector<ImageLayout> imageLayouts(images.size());
	for (int i(0); i < imageLayouts.size(); ++i)
	{
		imageLayouts[i].accessibility = VK_SHADER_STAGE_FRAGMENT_BIT;
		imageLayouts[i].binding = i + 2;
	}
	std::vector<std::pair<Image*, ImageLayout>> rendererImages(images.size());
	for (int j(0); j < rendererImages.size(); ++j)
	{
		rendererImages[j].first = images[j];
		rendererImages[j].second = imageLayouts[j];
	}
	for (std::pair<Image*, ImageLayout>& image : addTextInfo.images)
		rendererImages.push_back(image);

	// Samplers
	Sampler* sampler = addTextInfo.font->getSampler();
	SamplerLayout samplerLayout;
	samplerLayout.accessibility = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayout.binding = 1;
	std::vector<std::pair<Sampler*, SamplerLayout>> samplers = { { sampler, samplerLayout }};
	for (std::pair<Sampler*, SamplerLayout>& sampler : addTextInfo.samplers)
		samplers.push_back(sampler);

	m_sceneRenderPasses[addTextInfo.renderPassID].renderers[addTextInfo.rendererID]->addMesh(addTextInfo.text->getVertexBuffer(), std::move(ubos), std::move(addTextInfo.textures), std::move(rendererImages), std::move(samplers),
		std::move(addTextInfo.buffers));

	// Update descriptor pools needs
	m_descriptorPool.addUniformBuffer(static_cast<unsigned int>(addTextInfo.ubos.size()) + 1);
	m_descriptorPool.addCombinedImageSampler(static_cast<unsigned int>(addTextInfo.textures.size()));
	m_descriptorPool.addSampledImage(static_cast<unsigned int>(addTextInfo.images.size() + images.size()));
	m_descriptorPool.addSampler(static_cast<unsigned int>(addTextInfo.samplers.size()) + 1);
	m_descriptorPool.addStorageBuffer(static_cast<unsigned int>(addTextInfo.buffers.size()));
}

void Wolf::Scene::record()
{
	m_descriptorPool.allocate(m_device);
	
	for(SceneRenderPass& sceneRenderPass : m_sceneRenderPasses)
	{
		// Renderers creation
		for (std::unique_ptr<Renderer>& renderer : sceneRenderPass.renderers)
			renderer->create(m_device, sceneRenderPass.renderPass->getRenderPass(), m_swapChainImages[0]->getExtent(), VK_SAMPLE_COUNT_1_BIT, m_descriptorPool.getDescriptorPool());
	}
	
	// As a scene is designed to be renderer on a screen, we need to create a command buffer for each swapchain image
	m_swapChainCommandBuffers.resize(m_swapChainImages.size());
	for (size_t i(0); i < m_swapChainImages.size(); ++i)
	{
		if(m_swapChainCommandType == CommandType::GRAPHICS)
			m_swapChainCommandBuffers[i] = std::make_unique<CommandBuffer>(m_device, m_graphicsCommandPool);
		else 
			m_swapChainCommandBuffers[i] = std::make_unique<CommandBuffer>(m_device, m_computeCommandPool);
		
		m_swapChainCommandBuffers[i]->beginCommandBuffer();

		for(size_t j(0); j < m_sceneRenderPasses.size(); ++j)
		{
			if(m_sceneRenderPasses[j].commandBufferID == -1)
			{
				std::vector<VkClearValue> clearValues(0);
				for (RenderPassOutput& output : m_sceneRenderPasses[j].outputs)
					clearValues.push_back(output.clearValue);

				if(m_sceneRenderPasses[j].outputIsSwapChain)
					m_sceneRenderPasses[j].renderPass->beginRenderPass(i, clearValues, m_swapChainCommandBuffers[i]->getCommandBuffer());
				else
					m_sceneRenderPasses[j].renderPass->beginRenderPass(0, clearValues, m_swapChainCommandBuffers[i]->getCommandBuffer());

				for(std::unique_ptr<Renderer>& renderer : m_sceneRenderPasses[j].renderers)
				{
					vkCmdBindPipeline(m_swapChainCommandBuffers[i]->getCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, renderer->getPipeline());
					const VkDeviceSize offsets[1] = { 0 };

					/*VkViewport viewport;
					viewport.x = 0;
					viewport.y = 0;
					viewport.height = 100;
					viewport.width = 100;
					viewport.minDepth = 0.0f;
					viewport.maxDepth = 1.0f;
					vkCmdSetViewport(m_swapChainCommandBuffers[i]->getCommandBuffer(), 0, 1, &viewport);*/

					std::vector<std::tuple<VertexBuffer, InstanceBuffer, VkDescriptorSet>> meshesToRender = renderer->getMeshes();
					for(std::tuple<VertexBuffer, InstanceBuffer, VkDescriptorSet>& mesh : meshesToRender)
					{
						bool isInstancied = std::get<1>(mesh).nInstances > 0 && std::get<1>(mesh).instanceBuffer;
						
						vkCmdBindVertexBuffers(m_swapChainCommandBuffers[i]->getCommandBuffer(), 0, 1, &std::get<0>(mesh).vertexBuffer, offsets);
						vkCmdBindIndexBuffer(m_swapChainCommandBuffers[i]->getCommandBuffer(), std::get<0>(mesh).indexBuffer, 0, VK_INDEX_TYPE_UINT32);

						if(isInstancied)
							vkCmdBindVertexBuffers(m_swapChainCommandBuffers[i]->getCommandBuffer(), 1, 1, &std::get<1>(mesh).instanceBuffer, offsets);

						if(std::get<2>(mesh) != VK_NULL_HANDLE) // render can be done without descriptor set
							vkCmdBindDescriptorSets(m_swapChainCommandBuffers[i]->getCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS,
								renderer->getPipelineLayout(), 0, 1, &std::get<2>(mesh), 0, nullptr);

						if(!isInstancied)
							vkCmdDrawIndexed(m_swapChainCommandBuffers[i]->getCommandBuffer(), std::get<0>(mesh).nbIndices, 1, 0, 0, 0);
						else
							vkCmdDrawIndexed(m_swapChainCommandBuffers[i]->getCommandBuffer(), std::get<0>(mesh).nbIndices, std::get<1>(meshesToRender[j]).nInstances, 0, 0, 0);
					}
				}
				
				m_sceneRenderPasses[j].renderPass->endRenderPass(m_swapChainCommandBuffers[i]->getCommandBuffer());

				// Copy result to mirror
				if(m_useOVR)
				{
					Image::transitionImageLayoutUsingCommandBuffer(m_swapChainCommandBuffers[i]->getCommandBuffer(), m_windowSwapChainImages[i]->getImage(), VK_FORMAT_R8G8B8A8_UNORM /* just no depth */, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						1, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0);

					VkImageBlit region = {};
					region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					region.srcSubresource.mipLevel = 0;
					region.srcSubresource.baseArrayLayer = 0;
					region.srcSubresource.layerCount = 1;
					region.srcOffsets[0] = { 0, 0, 0 };
					region.srcOffsets[1] = { static_cast<int32_t>(m_swapChainImages[0]->getExtent().width), static_cast<int32_t>(m_swapChainImages[0]->getExtent().height), 1 };
					region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					region.dstSubresource.mipLevel = 0;
					region.dstSubresource.baseArrayLayer = 0;
					region.dstSubresource.layerCount = 1;
					region.dstOffsets[0] = { 0, 0, 0 };
					region.dstOffsets[1] = { static_cast<int32_t>(m_windowSwapChainImages[i]->getExtent().width),  static_cast<int32_t>(m_windowSwapChainImages[i]->getExtent().height), 1 };
					vkCmdBlitImage(m_swapChainCommandBuffers[i]->getCommandBuffer(), m_swapChainImages[i]->getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
						m_windowSwapChainImages[i]->getImage(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_LINEAR);

					Image::transitionImageLayoutUsingCommandBuffer(m_swapChainCommandBuffers[i]->getCommandBuffer(), m_windowSwapChainImages[i]->getImage(), VK_FORMAT_R8G8B8A8_UNORM /* just no depth */, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
						1, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0);
				}
			}
		}
		
		m_swapChainCommandBuffers[i]->endCommandBuffer();
	}
	if (m_swapChainCommandType == CommandType::GRAPHICS)
	{
		m_swapChainCompleteSemaphore = std::make_unique<Semaphore>();
		m_swapChainCompleteSemaphore->initialize(m_device);
	}
}

void Wolf::Scene::frame(Queue graphicsQueue, uint32_t swapChainImageIndex, Semaphore* imageAvailableSemaphore)
{
	if(imageAvailableSemaphore)
		m_swapChainCommandBuffers[swapChainImageIndex]->submit(m_device, graphicsQueue, { imageAvailableSemaphore }, { m_swapChainCompleteSemaphore->getSemaphore() });
	else
		m_swapChainCommandBuffers[swapChainImageIndex]->submit(m_device, graphicsQueue, { }, { });
}
