#include "GBuffer.h"

Wolf::GBuffer::GBuffer(Wolf::WolfInstance* engineInstance, Wolf::Scene* scene, int commandBufferID, VkExtent2D extent,
	VkSampleCountFlagBits sampleCount, Model* model, glm::mat4 mvp, bool useDepthAsStorage)
{
	m_engineInstance = engineInstance;
	m_scene = scene;
	m_sampleCount = sampleCount;
	
	Scene::RenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.commandBufferID = commandBufferID;
	renderPassCreateInfo.outputIsSwapChain = false;

	// Attachments -> depth + view pos + albedo + normal + (rougness + metal + ao)
	m_attachments.resize(5);
	VkImageUsageFlags depthUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	if (useDepthAsStorage)
		depthUsage |= VK_IMAGE_USAGE_STORAGE_BIT;
	VkImageLayout depthFinalLayout;
	if (useDepthAsStorage)
		depthFinalLayout = VK_IMAGE_LAYOUT_GENERAL;
	else
		depthFinalLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL;
	VkAttachmentStoreOp depthStoreOp;
	if (useDepthAsStorage)
		depthStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
	else
		depthStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	m_attachments[0] = Attachment(extent, VK_FORMAT_D32_SFLOAT, m_sampleCount, depthFinalLayout, depthStoreOp, depthUsage);
	m_attachments[1] = Attachment(extent, VK_FORMAT_R32G32B32A32_SFLOAT, m_sampleCount, VK_IMAGE_LAYOUT_GENERAL, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
	m_attachments[2] = Attachment(extent, VK_FORMAT_R8G8B8A8_UNORM, m_sampleCount, VK_IMAGE_LAYOUT_GENERAL, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
	m_attachments[3] = Attachment(extent, VK_FORMAT_R32G32B32A32_SFLOAT, m_sampleCount, VK_IMAGE_LAYOUT_GENERAL, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
	m_attachments[4] = Attachment(extent, VK_FORMAT_R8G8B8A8_UNORM, m_sampleCount, VK_IMAGE_LAYOUT_GENERAL, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT);

	m_clearValues.resize(5);
	m_clearValues[0] = { 1.0f };
	m_clearValues[1] = { -10.0f, 0.0f, std::numeric_limits<float>::max(), 1.0f }; // set max to view pos depth
	m_clearValues[2] = { 1.0f, 0.0f, 0.0f, 1.0f };
	m_clearValues[3] = { -10.0f, 0.0f, 0.0f, 1.0f };
	m_clearValues[4] = { 1.0f, 0.0f, 0.0f, 1.0f };

	int i(0);
	for(auto& attachment : m_attachments)
	{
		Scene::RenderPassOutput renderPassOutput;
		renderPassOutput.attachment = attachment;
		renderPassOutput.clearValue = m_clearValues[i++];
		
		renderPassCreateInfo.outputs.push_back(renderPassOutput);
	}

	m_renderPassID = m_scene->addRenderPass(renderPassCreateInfo);

	// Renderer
	Scene::RendererCreateInfo rendererCreateInfo;
	rendererCreateInfo.vertexShaderPath = "Shaders/GBuffer/vert.spv";
	rendererCreateInfo.fragmentShaderPath = "Shaders/GBuffer/frag.spv";
	rendererCreateInfo.inputVerticesTemplate = InputVertexTemplate::FULL_3D_MATERIAL;
	rendererCreateInfo.instanceTemplate = InstanceTemplate::NO;
	rendererCreateInfo.renderPassID = m_renderPassID;
	rendererCreateInfo.extent = extent;

	UniformBufferObjectLayout mvpLayout{};
	mvpLayout.accessibility = VK_SHADER_STAGE_VERTEX_BIT;
	mvpLayout.binding = 0;
	rendererCreateInfo.uboLayouts.push_back(mvpLayout);

	SamplerLayout samplerLayout{};
	samplerLayout.accessibility = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayout.binding = 1;
	rendererCreateInfo.samplerLayouts.push_back(samplerLayout);

	for (size_t i(0); i < model->getNumberOfImages(); ++i)
	{
		ImageLayout imageLayout{};
		imageLayout.accessibility = VK_SHADER_STAGE_FRAGMENT_BIT;
		imageLayout.binding = static_cast<uint32_t>(i + 2);
		rendererCreateInfo.imageLayouts.push_back(imageLayout);
	}

	rendererCreateInfo.alphaBlending = { true, true, true, true };

	m_rendererID = m_scene->addRenderer(rendererCreateInfo);

	Scene::AddModelInfo addModelInfo{};
	addModelInfo.model = model;
	addModelInfo.renderPassID = m_renderPassID;
	addModelInfo.rendererID = m_rendererID;

	// UBO
	m_uboMVP = engineInstance->createUniformBufferObject();
	m_uboMVP->initializeData(&m_mvp, 3 * sizeof(glm::mat4));
	addModelInfo.ubos.emplace_back(m_uboMVP, mvpLayout);

	// Sampler
	addModelInfo.samplers.emplace_back(model->getSampler(), samplerLayout);

	// Images
	std::vector<Image*> images = model->getImages();
	for (size_t i(0); i < images.size(); ++i)
	{
		ImageLayout imageLayout{};
		imageLayout.accessibility = VK_SHADER_STAGE_FRAGMENT_BIT;
		imageLayout.binding = static_cast<uint32_t>(i + 2);

		addModelInfo.images.emplace_back(images[i], imageLayout);
	}

	m_scene->addModel(addModelInfo);
}

void Wolf::GBuffer::updateMVPMatrix(glm::mat4 m, glm::mat4 v, glm::mat4 p)
{
	m_mvp = { p, m, v};
	m_uboMVP->updateData(&m_mvp);
}
