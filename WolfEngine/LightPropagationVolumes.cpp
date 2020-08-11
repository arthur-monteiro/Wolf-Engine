#include "LightPropagationVolumes.h"

Wolf::LightPropagationVolumes::LightPropagationVolumes(Wolf::WolfInstance* engineInstance, Wolf::Scene* scene, Model* model, glm::mat4 projection, glm::mat4 modelMat, glm::vec3 lightDir,
	glm::vec4 cascadeSplits, std::array<Image*, 4> depthTextures)
{
	m_engineInstance = engineInstance;
	m_scene = scene;

	m_voxelTexture = engineInstance->createTexture();
	m_voxelTexture->create({ VOXEL_SIZE, VOXEL_SIZE, VOXEL_SIZE }, VK_IMAGE_USAGE_STORAGE_BIT, VK_FORMAT_R8_UNORM, VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT);
	m_voxelTexture->setImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

	m_uboVoxelization = engineInstance->createUniformBufferObject();
	//modelMat = glm::mat4(1.0f);
	// X
	m_projections[0] = glm::ortho(-32.0f, 32.0f, -4.0f, 16.0f, 0.0f, 64.0f) *
		glm::lookAt(glm::vec3(-32.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
		modelMat;

	// Y
	glm::mat4 YLookAtMatrix = glm::mat4(1.0f);
	glm::vec3 f(0.0f, 1.0f, 0.0f);
	glm::vec3 s(1.0f, 0.0f, 0.0f);
	glm::vec3 u(0.0f, 0.0f, 1.0f);

	glm::vec3 eye(0.0f, -4.0f, 0.0f);
	
	YLookAtMatrix[0][0] = s.x;
	YLookAtMatrix[1][0] = s.y;
	YLookAtMatrix[2][0] = s.z;
	YLookAtMatrix[0][1] = u.x;
	YLookAtMatrix[1][1] = u.y;
	YLookAtMatrix[2][1] = u.z;
	YLookAtMatrix[0][2] = -f.x;
	YLookAtMatrix[1][2] = -f.y;
	YLookAtMatrix[2][2] = -f.z;
	YLookAtMatrix[3][0] = -dot(s, eye);
	YLookAtMatrix[3][1] = -dot(u, eye);
	YLookAtMatrix[3][2] = dot(f, eye);
	
	m_projections[1] = glm::ortho(-32.0f, 32.0f, -32.0f, 32.0f, 0.0f, 20.0f) * YLookAtMatrix *
		/*glm::lookAt(glm::vec3(0.0f, -4.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)) **/
		modelMat;

	// Z	
	m_projections[2] = glm::ortho(-32.0f, 32.0f, -4.0f, 16.0f, 0.0f, 64.0f) *
		glm::lookAt(glm::vec3(0.0f, 0.0f, -32.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
		modelMat;
	m_uboVoxelization->initializeData(&m_projections, 3 * sizeof(glm::mat4));

	glm::vec4 t = glm::vec4(-32.0f, 16.0f, -32.0f, 1.0f);
	glm::vec4 tX = m_projections[0] * t;
	glm::vec4 tY = m_projections[1] * t;
	glm::vec4 tZ = m_projections[2] * t;

	Scene::CommandBufferCreateInfo commandBufferCreateInfo;
	commandBufferCreateInfo.finalPipelineStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	commandBufferCreateInfo.commandType = Scene::CommandType::GRAPHICS;
	m_commandBufferID = scene->addCommandBuffer(commandBufferCreateInfo);

	Scene::RenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.commandBufferID = m_commandBufferID;
	renderPassCreateInfo.outputIsSwapChain = false;
	renderPassCreateInfo.extent = { VOXEL_SIZE, VOXEL_SIZE };

	m_attachments.resize(1);
	m_attachments[0] = Attachment({ VOXEL_SIZE, VOXEL_SIZE }, VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
	m_clearValues.resize(1);
	m_clearValues[0] = { 0.5f, 0.0f, 0.5f, 1.0f };

	int i(0);
	for (auto& attachment : m_attachments)
	{
		Scene::RenderPassOutput renderPassOutput;
		renderPassOutput.attachment = attachment;
		renderPassOutput.clearValue = m_clearValues[i++];

		renderPassCreateInfo.outputs.push_back(renderPassOutput);
	}

	m_renderPassID = m_scene->addRenderPass(renderPassCreateInfo);

	Scene::RendererCreateInfo rendererCreateInfo;
	rendererCreateInfo.vertexShaderPath = "Shaders/LightPropagationVolumes/vert.spv";
	rendererCreateInfo.fragmentShaderPath = "Shaders/LightPropagationVolumes/frag.spv";
	rendererCreateInfo.inputVerticesTemplate = InputVertexTemplate::FULL_3D_MATERIAL;
	rendererCreateInfo.instanceTemplate = InstanceTemplate::NO;
	rendererCreateInfo.renderPassID = m_renderPassID;
	rendererCreateInfo.extent = { VOXEL_SIZE, 128 };
	rendererCreateInfo.enableDepthTesting = false;
	rendererCreateInfo.enableConservativeRasterization = false;

	ImageLayout imageLayout{};
	imageLayout.accessibility = VK_SHADER_STAGE_FRAGMENT_BIT;
	imageLayout.binding = 1;
	imageLayout.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	rendererCreateInfo.imageLayouts.push_back(imageLayout);

	UniformBufferObjectLayout uboLayout{};
	uboLayout.accessibility = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayout.binding = 0;
	rendererCreateInfo.uboLayouts.push_back(uboLayout);

	m_voxelisationRendererID = m_scene->addRenderer(rendererCreateInfo);

	Scene::AddModelInfo addModelInfo{};
	addModelInfo.model = model;
	addModelInfo.renderPassID = m_renderPassID;
	addModelInfo.rendererID = m_voxelisationRendererID;

	addModelInfo.images.emplace_back(m_voxelTexture->getImage(), imageLayout);

	addModelInfo.ubos.emplace_back(m_uboVoxelization, uboLayout);

	m_scene->addModel(addModelInfo);

	buildInjection(model, cascadeSplits, depthTextures);
	buildPropagation();

	// Voxel viewer
	commandBufferCreateInfo.finalPipelineStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	commandBufferCreateInfo.commandType = Scene::CommandType::COMPUTE;
	m_viewerBufferID = scene->addCommandBuffer(commandBufferCreateInfo);

	Scene::ComputePassCreateInfo voxelViewerComputePassCreateInfo;
	voxelViewerComputePassCreateInfo.extent = engineInstance->getWindowSize();
	voxelViewerComputePassCreateInfo.computeShaderPath = "Shaders/LightPropagationVolumes/comp.spv";
	voxelViewerComputePassCreateInfo.outputIsSwapChain = false;
	voxelViewerComputePassCreateInfo.commandBufferID = m_viewerBufferID;
	voxelViewerComputePassCreateInfo.dispatchGroups = { 16, 16, 1 };

	ImageLayout voxelTextureRLayout{};
	voxelTextureRLayout.accessibility = VK_SHADER_STAGE_COMPUTE_BIT;
	voxelTextureRLayout.binding = 0;

	ImageLayout voxelTextureGLayout{};
	voxelTextureGLayout.accessibility = VK_SHADER_STAGE_COMPUTE_BIT;
	voxelTextureGLayout.binding = 1;
	
	ImageLayout voxelTextureBLayout{};
	voxelTextureBLayout.accessibility = VK_SHADER_STAGE_COMPUTE_BIT;
	voxelTextureBLayout.binding = 2;

	ImageLayout voxelCountTextureLayout{};
	voxelCountTextureLayout.accessibility = VK_SHADER_STAGE_COMPUTE_BIT;
	voxelCountTextureLayout.binding = 3;

	ImageLayout outputTextureLayout{};
	outputTextureLayout.accessibility = VK_SHADER_STAGE_COMPUTE_BIT;
	outputTextureLayout.binding = 4;

	m_viewerOutput = engineInstance->createTexture();
	m_viewerOutput->create({ engineInstance->getWindowSize().width, engineInstance->getWindowSize().height, 1 }, VK_IMAGE_USAGE_STORAGE_BIT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT);
	m_viewerOutput->setImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

	voxelViewerComputePassCreateInfo.images = { {m_injectionTextures[0]->getImage(), voxelTextureRLayout},
		{m_injectionTextures[1]->getImage(), voxelTextureGLayout}, {m_injectionTextures[2]->getImage(), voxelTextureBLayout},
		{m_injectionTextures.back()->getImage(), voxelCountTextureLayout}, { m_viewerOutput->getImage(), outputTextureLayout }};

	m_uboVoxelViewer = engineInstance->createUniformBufferObject();
	m_voxelViewerMatrices[2] = glm::ortho(-32.0f, 32.0f, -4.0f, 16.0f, 0.0f, 64.0f) *
		glm::lookAt(glm::vec3(0.0f, 0.0f, -32.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	m_voxelViewerMatrices[1] = glm::inverse(projection);
	m_uboVoxelViewer->initializeData(&m_voxelViewerMatrices, 3 * sizeof(glm::mat4));

	UniformBufferObjectLayout uboMatricesLayout{};
	uboMatricesLayout.accessibility = VK_SHADER_STAGE_COMPUTE_BIT;
	uboMatricesLayout.binding = 5;

	voxelViewerComputePassCreateInfo.ubos = { { m_uboVoxelViewer, uboMatricesLayout } };

	m_viewerComputePassID = scene->addComputePass(voxelViewerComputePassCreateInfo);

	/* Clear */
	commandBufferCreateInfo.finalPipelineStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	commandBufferCreateInfo.commandType = Scene::CommandType::COMPUTE;
	m_clearCommandBufferID = scene->addCommandBuffer(commandBufferCreateInfo);

	Scene::ComputePassCreateInfo clearComputePassCreateInfo;
	clearComputePassCreateInfo.extent = { VOXEL_SIZE, VOXEL_SIZE };
	clearComputePassCreateInfo.computeShaderPath = "Shaders/LightPropagationVolumes/clear.spv";
	clearComputePassCreateInfo.outputIsSwapChain = false;
	clearComputePassCreateInfo.commandBufferID = m_clearCommandBufferID;
	clearComputePassCreateInfo.dispatchGroups = { 8, 8, VOXEL_SIZE / 8 };

	for (int i(0); i < m_injectionTextures.size(); ++i)
	{
		ImageLayout imageLayout{};
		imageLayout.accessibility = VK_SHADER_STAGE_COMPUTE_BIT;
		imageLayout.binding = i;

		clearComputePassCreateInfo.images.emplace_back(m_injectionTextures[i]->getImage(), imageLayout);
	}

	m_clearComputePassID = scene->addComputePass(clearComputePassCreateInfo);
}

void Wolf::LightPropagationVolumes::update(glm::mat4 view, std::array<glm::mat4, 4> lightSpaceMatrices, glm::mat4 modelMat)
{
	m_uboInjectionData.lightSpaceMatrices = std::move(lightSpaceMatrices);
	m_uboInjectionData.modelView = view * modelMat;
	m_uboInjection->updateData(&m_uboInjectionData);
	
	m_voxelViewerMatrices[0] = glm::inverse(view);
	m_uboVoxelViewer->updateData(&m_voxelViewerMatrices);
}

void Wolf::LightPropagationVolumes::buildInjection(Model* model, glm::vec4 cascadeSplits, std::array<Image*, 4> depthTextures)
{
	// Data
	for(int i(0); i < m_injectionTextures.size(); ++i)
	{
		m_injectionTextures[i] = m_engineInstance->createTexture();
		m_injectionTextures[i]->create({ VOXEL_SIZE, VOXEL_SIZE, VOXEL_SIZE }, VK_IMAGE_USAGE_STORAGE_BIT, VK_FORMAT_R32_UINT, VK_SAMPLE_COUNT_1_BIT,
			VK_IMAGE_ASPECT_COLOR_BIT);
		m_injectionTextures[i]->setImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	}
	
	m_uboInjection = m_engineInstance->createUniformBufferObject();

	m_uboInjectionData.projectionX = m_projections[0];
	m_uboInjectionData.projectionY = m_projections[1];
	m_uboInjectionData.projectionZ = m_projections[2];
	m_uboInjectionData.cascadeSplits = cascadeSplits;
	
	m_uboInjection->initializeData(&m_uboInjectionData, sizeof(m_uboInjectionData));

	// Command Buffer
	Scene::CommandBufferCreateInfo commandBufferCreateInfo;
	commandBufferCreateInfo.finalPipelineStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	commandBufferCreateInfo.commandType = Scene::CommandType::GRAPHICS;
	m_injectionCommandBufferID = m_scene->addCommandBuffer(commandBufferCreateInfo);

	// Render Pass
	Scene::RenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.commandBufferID = m_injectionCommandBufferID;
	renderPassCreateInfo.outputIsSwapChain = false;
	renderPassCreateInfo.extent = { VOXEL_SIZE, VOXEL_SIZE };

	m_injectionAttachments.resize(1);
	m_injectionAttachments[0] = Attachment({ VOXEL_SIZE, VOXEL_SIZE }, VK_FORMAT_R8G8B8A8_UNORM, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_GENERAL, VK_ATTACHMENT_STORE_OP_STORE, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT);
	m_injectionClearValues.resize(1);
	m_injectionClearValues[0] = { 0.5f, 0.0f, 0.5f, 1.0f };

	int i(0);
	for (auto& attachment : m_injectionAttachments)
	{
		Scene::RenderPassOutput renderPassOutput;
		renderPassOutput.attachment = attachment;
		renderPassOutput.clearValue = m_injectionClearValues[i++];

		renderPassCreateInfo.outputs.push_back(renderPassOutput);
	}

	m_injectionRenderPassID = m_scene->addRenderPass(renderPassCreateInfo);

	// Renderer
	Scene::RendererCreateInfo rendererCreateInfo;
	rendererCreateInfo.vertexShaderPath = "Shaders/LightPropagationVolumes/injectionVert.spv";
	rendererCreateInfo.fragmentShaderPath = "Shaders/LightPropagationVolumes/injectionFrag.spv";
	rendererCreateInfo.inputVerticesTemplate = InputVertexTemplate::FULL_3D_MATERIAL;
	rendererCreateInfo.instanceTemplate = InstanceTemplate::NO;
	rendererCreateInfo.renderPassID = m_injectionRenderPassID;
	rendererCreateInfo.extent = { VOXEL_SIZE, VOXEL_SIZE };
	rendererCreateInfo.enableDepthTesting = false;
	rendererCreateInfo.enableConservativeRasterization = false;

	for (int i(0); i < m_injectionTextures.size(); ++i)
	{
		ImageLayout voxelDataImageLayout{};
		voxelDataImageLayout.accessibility = VK_SHADER_STAGE_FRAGMENT_BIT;
		voxelDataImageLayout.binding = i + 2;
		voxelDataImageLayout.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		rendererCreateInfo.imageLayouts.push_back(voxelDataImageLayout);
	}

	SamplerLayout samplerLayout{};
	samplerLayout.accessibility = VK_SHADER_STAGE_FRAGMENT_BIT;
	samplerLayout.binding = 1;
	rendererCreateInfo.samplerLayouts.push_back(samplerLayout);

	for (size_t j(0); j < model->getNumberOfImages() + 4 /* depth textures */; ++j)
	{
		ImageLayout imageLayout{};
		imageLayout.accessibility = VK_SHADER_STAGE_FRAGMENT_BIT;
		imageLayout.binding = static_cast<uint32_t>(j + 9);
		rendererCreateInfo.imageLayouts.push_back(imageLayout);
	}

	UniformBufferObjectLayout uboLayout{};
	uboLayout.accessibility = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayout.binding = 0;
	rendererCreateInfo.uboLayouts.push_back(uboLayout);

	m_voxelisationRendererID = m_scene->addRenderer(rendererCreateInfo);

	// Add Model
	Scene::AddModelInfo addModelInfo{};
	addModelInfo.model = model;
	addModelInfo.renderPassID = m_injectionRenderPassID;
	addModelInfo.rendererID = m_voxelisationRendererID;

	for(int j(0); j < m_injectionTextures.size(); ++j)
	{
		ImageLayout imageLayout{};
		imageLayout.accessibility = VK_SHADER_STAGE_FRAGMENT_BIT;
		imageLayout.binding = static_cast<uint32_t>(j + 2);
		imageLayout.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		addModelInfo.images.emplace_back(m_injectionTextures[j]->getImage(), imageLayout);
	}
	
	for(int j(0); j < 4; ++j)
	{
		ImageLayout imageLayout{};
		imageLayout.accessibility = VK_SHADER_STAGE_FRAGMENT_BIT;
		imageLayout.binding = static_cast<uint32_t>(j + 9);

		addModelInfo.images.emplace_back(depthTextures[j], imageLayout);
	}
	std::vector<Image*> images = model->getImages();
	for (size_t j(0); j < images.size(); ++j)
	{
		ImageLayout imageLayout{};
		imageLayout.accessibility = VK_SHADER_STAGE_FRAGMENT_BIT;
		imageLayout.binding = static_cast<uint32_t>(j + 13);

		addModelInfo.images.emplace_back(images[j], imageLayout);
	}

	addModelInfo.ubos.emplace_back(m_uboInjection, uboLayout);
	addModelInfo.samplers.emplace_back(model->getSampler(), samplerLayout);

	m_scene->addModel(addModelInfo);
}

void Wolf::LightPropagationVolumes::buildPropagation()
{
	// Data
	m_lightVolumesPropagationTexture = m_engineInstance->createTexture();
	m_lightVolumesPropagationTexture->create({ VOXEL_SIZE, VOXEL_SIZE, VOXEL_SIZE }, VK_IMAGE_USAGE_STORAGE_BIT, VK_FORMAT_R32G32B32A32_SFLOAT, VK_SAMPLE_COUNT_1_BIT,
		VK_IMAGE_ASPECT_COLOR_BIT);
	m_lightVolumesPropagationTexture->setImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

	// Command Buffer
	Scene::CommandBufferCreateInfo commandBufferCreateInfo;
	commandBufferCreateInfo.finalPipelineStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	commandBufferCreateInfo.commandType = Scene::CommandType::COMPUTE;
	m_propagationCommandBufferID = m_scene->addCommandBuffer(commandBufferCreateInfo);

	// Compute pass  
	Scene::ComputePassCreateInfo propagationComputePassCreateInfo;
	propagationComputePassCreateInfo.extent = { VOXEL_SIZE, VOXEL_SIZE };
	propagationComputePassCreateInfo.computeShaderPath = "Shaders/LightPropagationVolumes/propagation.spv";
	propagationComputePassCreateInfo.outputIsSwapChain = false;
	propagationComputePassCreateInfo.commandBufferID = m_propagationCommandBufferID;
	propagationComputePassCreateInfo.dispatchGroups = { 8, 8, VOXEL_SIZE / 8 };

	for(int i(0);  i < m_injectionTextures.size(); ++i)
	{
		ImageLayout imageLayout{};
		imageLayout.accessibility = VK_SHADER_STAGE_COMPUTE_BIT;
		imageLayout.binding = i;

		propagationComputePassCreateInfo.images.emplace_back(m_injectionTextures[i]->getImage(), imageLayout);
	}
	
	ImageLayout outputLayout{};
	outputLayout.accessibility = VK_SHADER_STAGE_COMPUTE_BIT;
	outputLayout.binding = m_injectionTextures.size();

	propagationComputePassCreateInfo.images.emplace_back(m_lightVolumesPropagationTexture->getImage(), outputLayout);

	m_propagationComputePassID = m_scene->addComputePass(propagationComputePassCreateInfo);
}
