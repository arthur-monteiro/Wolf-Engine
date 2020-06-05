#include "CascadedShadowMapping.h"

Wolf::CascadedShadowMapping::CascadedShadowMapping(Wolf::WolfInstance* engineInstance, Wolf::Scene* scene, Model* model, float cameraNear, float cameraFar, float cameraFOV, VkExtent2D extent)
{
	m_engineInstance = engineInstance;
	m_scene = scene;
	m_cameraNear = cameraNear;
	m_cameraFOV = cameraFOV;
	m_cameraFar = cameraFar;
	m_ratio = static_cast<float>(extent.height) / static_cast<float>(extent.width);
	m_extent = extent;

	m_shadowMapExtents = { { 2048, 2048 }, { 2048, 2048 }, { 1024, 1024 }, { 1024, 1024 } };

	for(int i(0); i < m_depthPasses.size(); ++i)
	{
		// We use separate command buffers because we want to update cascade separately -> Crytek paper
		Scene::CommandBufferCreateInfo commandBufferCreateInfo;
		commandBufferCreateInfo.commandType = Scene::CommandType::GRAPHICS;
		commandBufferCreateInfo.finalPipelineStage = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
		m_cascadeCommandBuffers[i] = scene->addCommandBuffer(commandBufferCreateInfo);
		
		m_depthPasses[i] = std::make_unique<DepthPass>(engineInstance, scene, m_cascadeCommandBuffers[i], false, m_shadowMapExtents[i], VK_SAMPLE_COUNT_1_BIT, model, glm::mat4(1.0f), false);
	}

	// Cascade splits
	float near = m_cameraNear;
	float far = m_cameraFar; // we don't render shadows on all the range
	for (float i(1.0f / CASCADE_COUNT); i <= 1.0f; i += 1.0f / CASCADE_COUNT)
	{
		float d_uni = glm::mix(near, far, i);
		float d_log = near * glm::pow((far / near), i);

		m_cascadeSplits.push_back(glm::mix(d_uni, d_log, 0.5f));
	}

	// Shadow Mask
	Scene::CommandBufferCreateInfo commandBufferCreateInfo;
	commandBufferCreateInfo.commandType = Scene::CommandType::GRAPHICS;
	commandBufferCreateInfo.finalPipelineStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	m_shadowMaskCommandBuffer = scene->addCommandBuffer(commandBufferCreateInfo);
	
	Scene::RenderPassCreateInfo renderPassCreateInfo{};
	
	m_shadowMaskAttachment = Attachment(m_extent, VK_FORMAT_R8_UNORM, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ATTACHMENT_STORE_OP_STORE,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
	Scene::RenderPassOutput renderPassOutput;
	renderPassOutput.attachment = m_shadowMaskAttachment;
	renderPassOutput.clearValue = { 1.0f };
	renderPassCreateInfo.outputs.push_back(renderPassOutput);

	renderPassCreateInfo.outputIsSwapChain = false;
	renderPassCreateInfo.commandBufferID = m_shadowMaskCommandBuffer;
	
	m_shadowMaskRenderPass = m_scene->addRenderPass(renderPassCreateInfo);

	Scene::RendererCreateInfo rendererCreateInfo;
	rendererCreateInfo.vertexShaderPath = "Shaders/CSM/vertMask.spv";
	rendererCreateInfo.fragmentShaderPath = "Shaders/CSM/fragMask.spv";
	rendererCreateInfo.inputVerticesTemplate = InputVertexTemplate::FULL_3D_MATERIAL;
	rendererCreateInfo.instanceTemplate = InstanceTemplate::NO;
	rendererCreateInfo.renderPassID = m_shadowMaskRenderPass;
	rendererCreateInfo.extent = m_extent;

	UniformBufferObjectLayout mvpLayout{};
	mvpLayout.accessibility = VK_SHADER_STAGE_VERTEX_BIT;
	mvpLayout.binding = 0;
	rendererCreateInfo.uboLayouts.push_back(mvpLayout);
}

Wolf::CascadedShadowMapping::~CascadedShadowMapping()
{
}

void Wolf::CascadedShadowMapping::updateMatrices(glm::vec3 lightDir,
	glm::vec3 cameraPosition, glm::vec3 cameraOrientation, glm::mat4 model)
{	
	float lastSplitDist = m_cameraNear;
	for (int cascade(0); cascade < CASCADE_COUNT; ++cascade)
	{
		const float startCascade = lastSplitDist;
		const float endCascade = m_cascadeSplits[cascade];

		float radius = (endCascade - startCascade) / 2.0f;

		const float ar = m_ratio;
		const float cosHalfHFOV = static_cast<float>(glm::cos(static_cast<double>((m_cameraFOV * (1.0f / ar)) / 2.0f)));
		const double b = endCascade / cosHalfHFOV;
		radius = glm::sqrt(b * b + (startCascade + radius) * (startCascade + radius) - 2 * b * startCascade * cosHalfHFOV);

		const float texelPerUnit = static_cast<float>(m_shadowMapExtents[cascade].width) / (radius * 2.0f);
		glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(texelPerUnit));
		glm::mat4 lookAt = scaleMat * glm::lookAt(glm::vec3(0.0f), -lightDir, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 lookAtInv = glm::inverse(lookAt);

		glm::vec3 frustumCenter = cameraPosition + cameraOrientation * startCascade + cameraOrientation * (endCascade / 2.0f);
		frustumCenter = lookAt * glm::vec4(frustumCenter, 1.0f);
		frustumCenter.x = static_cast<float>(floor(frustumCenter.x));
		frustumCenter.y = static_cast<float>(floor(frustumCenter.y));
		frustumCenter = lookAtInv * glm::vec4(frustumCenter, 1.0f);

		glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - glm::normalize(lightDir), frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));

		glm::mat4 proj = glm::ortho(-radius, radius, -radius, radius, -30.0f * 6.0f, 30.0f * 6.0f);
		m_lightSpaceMatrices[cascade] = proj * lightViewMatrix * model;
		m_depthPasses[cascade]->update(m_lightSpaceMatrices[cascade]);

		lastSplitDist += m_cascadeSplits[cascade];
	}
}
