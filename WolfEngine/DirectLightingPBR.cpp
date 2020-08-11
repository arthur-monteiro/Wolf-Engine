#include "DirectLightingPBR.h"

Wolf::DirectLightingPBR::DirectLightingPBR(Wolf::WolfInstance* engineInstance, Wolf::Scene* scene, int commandBufferID,
	VkExtent2D extent, Image* depth, Image* albedoImage, Image* normalRoughnessMetal, Image* shadowMask, Image* volumetricLight, Image* aoMaskImage, Image* lightPropagationVolumes,
	glm::mat4 projection, float near, float far)
{
	Scene::ComputePassCreateInfo computePassCreateInfo;
	computePassCreateInfo.extent = engineInstance->getWindowSize();
	computePassCreateInfo.dispatchGroups = { 16, 16, 1 };
	computePassCreateInfo.computeShaderPath = "Shaders/directLighting/comp.spv";
	computePassCreateInfo.commandBufferID = commandBufferID;

	ImageLayout depthLayout{};
	depthLayout.accessibility = VK_SHADER_STAGE_COMPUTE_BIT;
	depthLayout.binding = 0;

	ImageLayout albedoImageLayout{};
	albedoImageLayout.accessibility = VK_SHADER_STAGE_COMPUTE_BIT;
	albedoImageLayout.binding = 1;

	ImageLayout normalRoughnessMetalImageLayout{};
	normalRoughnessMetalImageLayout.accessibility = VK_SHADER_STAGE_COMPUTE_BIT;
	normalRoughnessMetalImageLayout.binding = 2;

	ImageLayout shadowMaskImageLayout{};
	shadowMaskImageLayout.accessibility = VK_SHADER_STAGE_COMPUTE_BIT;
	shadowMaskImageLayout.binding = 3;

	ImageLayout volumetricLightImageLayout{};
	volumetricLightImageLayout.accessibility = VK_SHADER_STAGE_COMPUTE_BIT;
	volumetricLightImageLayout.binding = 4;

	ImageLayout aoImageLayout{};
	aoImageLayout.accessibility = VK_SHADER_STAGE_COMPUTE_BIT;
	aoImageLayout.binding = 5;

	ImageLayout lightPropagationVolumesImageLayout{};
	lightPropagationVolumesImageLayout.accessibility = VK_SHADER_STAGE_COMPUTE_BIT;
	lightPropagationVolumesImageLayout.binding = 6;

	ImageLayout outputImageLayout{};
	outputImageLayout.accessibility = VK_SHADER_STAGE_COMPUTE_BIT;
	outputImageLayout.binding = 7;

	m_outputTexture = engineInstance->createTexture();
	m_outputTexture->create({ engineInstance->getWindowSize().width, engineInstance->getWindowSize().height, 1 }, VK_IMAGE_USAGE_STORAGE_BIT, VK_FORMAT_R32G32B32A32_SFLOAT, 
		VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	m_outputTexture->setImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

	computePassCreateInfo.images = { { depth, depthLayout},
		{ albedoImage, albedoImageLayout },
		{ normalRoughnessMetal, normalRoughnessMetalImageLayout},
		{ shadowMask, shadowMaskImageLayout },
		{ volumetricLight, volumetricLightImageLayout },
		{ aoMaskImage, aoImageLayout },
		{ lightPropagationVolumes, lightPropagationVolumesImageLayout },
		{ m_outputTexture->getImage(), outputImageLayout} };

	m_uboData.directionDirectionalLight = glm::vec4(-1.0f, -5.0f, 0.0f, 1.0f);
	m_uboData.colorDirectionalLight = glm::vec4(10.0f, 9.0f, 6.0f, 1.0f);
	m_uboData.invProjection = glm::inverse(projection);
	m_uboData.projParams.x = far / (far - near);
	m_uboData.projParams.y = (-far * near) / (far - near);
	
	m_ubo = engineInstance->createUniformBufferObject();
	m_ubo->initializeData(&m_uboData, sizeof(UBOData));

	UniformBufferObjectLayout uboLayout{};
	uboLayout.accessibility = VK_SHADER_STAGE_COMPUTE_BIT;
	uboLayout.binding = 8;

	computePassCreateInfo.ubos = { { m_ubo, uboLayout } };

	m_computePassID = scene->addComputePass(computePassCreateInfo);
}

void Wolf::DirectLightingPBR::update(glm::vec3 lightDirectionInViewPosSpace, glm::mat4 voxelProjection)
{
	m_uboData.directionDirectionalLight = glm::vec4(lightDirectionInViewPosSpace, 1.0f);
	m_uboData.voxelProjection = voxelProjection;
	m_ubo->updateData(&m_uboData);
}
