#include "CascadedShadowMapping.h"

Wolf::CascadedShadowMapping::CascadedShadowMapping(Wolf::WolfInstance* engineInstance, Wolf::Scene* scene, Model* model, float cameraNear, float cameraFar, float shadowFar, 
	float cameraFOV, VkExtent2D extent, Image* depth, glm::mat4 projection)
{
	m_engineInstance = engineInstance;
	m_scene = scene;
	m_cameraNear = cameraNear;
	m_cameraFOV = cameraFOV;
	m_cameraFar = shadowFar;
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
		
		m_depthPasses[i] = std::make_unique<DepthPass>(engineInstance, scene, m_cascadeCommandBuffers[i], false, m_shadowMapExtents[i], VK_SAMPLE_COUNT_1_BIT, model, glm::mat4(1.0f), true);
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
	commandBufferCreateInfo.finalPipelineStage = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
	commandBufferCreateInfo.commandType = Scene::CommandType::COMPUTE;
	m_shadowMaskCommandBufferID = scene->addCommandBuffer(commandBufferCreateInfo);

	Scene::ComputePassCreateInfo computePassCreateInfo;
	computePassCreateInfo.extent = engineInstance->getWindowSize();
	computePassCreateInfo.dispatchGroups = { 16, 16, 1 };
	computePassCreateInfo.computeShaderPath = "Shaders/CSM/comp.spv";
	computePassCreateInfo.commandBufferID = m_shadowMaskCommandBufferID;

	ImageLayout depthLayout{};
	depthLayout.accessibility = VK_SHADER_STAGE_COMPUTE_BIT;
	depthLayout.binding = 0;

	std::array<ImageLayout, CASCADE_COUNT> cascadeLayouts{};
	for(int i(0); i < CASCADE_COUNT; ++i)
	{
		cascadeLayouts[i].accessibility = VK_SHADER_STAGE_COMPUTE_BIT;
		cascadeLayouts[i].binding = i + 1;
	}

	ImageLayout shadowMaskOutputLayout{};
	shadowMaskOutputLayout.accessibility = VK_SHADER_STAGE_COMPUTE_BIT;
	shadowMaskOutputLayout.binding = CASCADE_COUNT + 1;

	m_shadowMaskOutputTexture = engineInstance->createTexture();
	m_shadowMaskOutputTexture->create(engineInstance->getWindowSize(), VK_IMAGE_USAGE_STORAGE_BIT, VK_FORMAT_R32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	m_shadowMaskOutputTexture->setImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);
	
	ImageLayout volumetricLightOutput{};
	volumetricLightOutput.accessibility = VK_SHADER_STAGE_COMPUTE_BIT;
	volumetricLightOutput.binding = CASCADE_COUNT + 2;

	m_volumetricLightOutputTexture = engineInstance->createTexture();
	m_volumetricLightOutputTexture->create(engineInstance->getWindowSize(), VK_IMAGE_USAGE_STORAGE_BIT, VK_FORMAT_R32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	m_volumetricLightOutputTexture->setImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

	computePassCreateInfo.images = { { depth, depthLayout } };
	for(int i(0); i < CASCADE_COUNT; ++i)
	{	
		computePassCreateInfo.images.emplace_back(m_depthPasses[i]->getResult(), cascadeLayouts[i]);
	}

	computePassCreateInfo.images.emplace_back(m_shadowMaskOutputTexture->getImage(), shadowMaskOutputLayout);
	computePassCreateInfo.images.emplace_back(m_volumetricLightOutputTexture->getImage(), volumetricLightOutput);

	UniformBufferObjectLayout uboLayout{};
	uboLayout.accessibility = VK_SHADER_STAGE_COMPUTE_BIT;
	uboLayout.binding = CASCADE_COUNT + 3;

	m_ubo = engineInstance->createUniformBufferObject();
	m_uboData.cascadeSplits = glm::vec4(m_cascadeSplits[0], m_cascadeSplits[1], m_cascadeSplits[2], m_cascadeSplits[3]);
	m_uboData.invProjection = glm::inverse(projection);
	m_uboData.projectionParams.x = cameraFar / (cameraFar - cameraNear);
	m_uboData.projectionParams.y = (-cameraFar * cameraNear) / (cameraFar - cameraNear);
	m_ubo->initializeData(&m_uboData, sizeof(ShadowMaskUBO));

	computePassCreateInfo.ubos = { { m_ubo, uboLayout } };

	m_shadowMaskComputePassID = scene->addComputePass(computePassCreateInfo);

	m_blur = std::make_unique<Blur>(engineInstance, scene, m_shadowMaskCommandBufferID, m_volumetricLightOutputTexture->getImage(), nullptr);
}

void Wolf::CascadedShadowMapping::updateMatrices(glm::vec3 lightDir,
	glm::vec3 cameraPosition, glm::vec3 cameraOrientation, glm::mat4 model, glm::mat4 invModelView)
{	
	float lastSplitDist = m_cameraNear;
	for (int cascade(0); cascade < CASCADE_COUNT; ++cascade)
	{
		const float startCascade = lastSplitDist;
		const float endCascade = m_cascadeSplits[cascade];

		float radius = (endCascade - startCascade) / 2.0f;

		const float ar = m_ratio;
		const float cosHalfHFOV = static_cast<float>(glm::cos((m_cameraFOV * (1.0f / ar)) / 2.0f));
		const float b = endCascade / cosHalfHFOV;
		radius = glm::sqrt(b * b + (startCascade + radius) * (startCascade + radius) - 2.0f * b * startCascade * cosHalfHFOV);

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
		m_uboData.lightSpaceMatrices[cascade] = m_lightSpaceMatrices[cascade];
		m_depthPasses[cascade]->update(m_lightSpaceMatrices[cascade]);

		lastSplitDist += m_cascadeSplits[cascade];
	}

	m_uboData.invModelView = invModelView;
	m_ubo->updateData(&m_uboData);
}