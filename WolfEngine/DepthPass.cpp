#include "DepthPass.h"

Wolf::DepthPass::DepthPass(Wolf::WolfInstance* engineInstance, Wolf::Scene* scene, int commandBufferID, bool outputIsSwapChain, VkExtent2D extent, VkSampleCountFlagBits sampleCount,
	Model* model, glm::mat4 mvp, bool useAsStorage, bool useAsSampled)
{
	m_engineInstance = engineInstance;
	m_scene = scene;

	// Render Pass Creation
	m_sampleCount = sampleCount;
	
	Scene::RenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.commandBufferID = commandBufferID;
	renderPassCreateInfo.outputIsSwapChain = outputIsSwapChain; // should be equal to "no"

	VkImageUsageFlags usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
	if (useAsStorage)
		usage |= VK_IMAGE_USAGE_STORAGE_BIT;
	if(useAsSampled)
		usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
	VkImageLayout layout;
	if (useAsStorage)
		layout = VK_IMAGE_LAYOUT_GENERAL;
	else
		layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	m_attachment = Attachment(extent, VK_FORMAT_D32_SFLOAT, m_sampleCount, layout, VK_ATTACHMENT_STORE_OP_STORE, usage);
	Scene::RenderPassOutput renderPassOutput;
	renderPassOutput.attachment = m_attachment;
	renderPassOutput.clearValue = { 1.0f };
	renderPassCreateInfo.outputs = { renderPassOutput };
	
	m_renderPassID = m_scene->addRenderPass(renderPassCreateInfo);

	// Renderer
	Scene::RendererCreateInfo rendererCreateInfo;
	rendererCreateInfo.vertexShaderPath = "Shaders/depthPass/vert.spv";
	rendererCreateInfo.fragmentShaderPath = ""; // no fragment shader
	rendererCreateInfo.inputVerticesTemplate = InputVertexTemplate::FULL_3D_MATERIAL;
	rendererCreateInfo.instanceTemplate = InstanceTemplate::NO;
	rendererCreateInfo.renderPassID = m_renderPassID;
	rendererCreateInfo.extent = extent;

	UniformBufferObjectLayout mvpLayout{};
	mvpLayout.accessibility = VK_SHADER_STAGE_VERTEX_BIT;
	mvpLayout.binding = 0;
	rendererCreateInfo.uboLayouts.push_back(mvpLayout);
	
	m_rendererID = scene->addRenderer(rendererCreateInfo);

	Scene::AddModelInfo addModelInfo{};
	addModelInfo.model = model;
	addModelInfo.renderPassID = m_renderPassID;
	addModelInfo.rendererID = m_rendererID;

	m_uboMVP = engineInstance->createUniformBufferObject();
	m_mvp = glm::mat4(1.0f);
	m_uboMVP->initializeData(&m_mvp, sizeof(glm::mat4));
	addModelInfo.ubos.emplace_back(m_uboMVP, mvpLayout);

	m_scene->addModel(addModelInfo);
}

void Wolf::DepthPass::update(glm::mat4 mvp)
{
	m_mvp = mvp;
	m_uboMVP->updateData(&m_mvp);
}
