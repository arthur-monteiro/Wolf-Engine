#include "SSAO.h"
#include <random>

Wolf::SSAO::SSAO(Wolf::WolfInstance* engineInstance, Wolf::Scene* scene, int commandBufferID, VkExtent2D extent,
	glm::mat4 projection, Image* depth, Image* normal, float near, float far)
{
	// Data
	const std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between 0.0 - 1.0
	std::random_device rd;
	std::default_random_engine generator(rd());
	std::array<glm::vec4, 16> ssaoKernel;
	for (unsigned int i = 0; i < 16; ++i)
	{
		glm::vec3 sample(
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator) // normal is (0, 0, 1)
		);
		sample = glm::normalize(sample); // fit point into hemisphere (r = 1)
		sample *= randomFloats(generator);
		float scale = static_cast<float>(i) / 64.0f;
		scale = glm::mix(0.1f, 1.0f, scale * scale);
		sample *= scale;
		ssaoKernel[i] = glm::vec4(sample, 0.0f);
	}

	std::array<glm::vec4, 16> ssaoNoise;
	for (unsigned int i = 0; i < 16; i++)
	{
		glm::vec3 noise(
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator) * 2.0 - 1.0,
			0.0f);
		ssaoNoise[i] = glm::vec4(noise, 0.0f);
	}

	m_uboData.samples = ssaoKernel;
	m_uboData.noise = ssaoNoise;
	m_uboData.projection = projection;
	m_uboData.invProjection = glm::inverse(projection);
	m_uboData.projParams.x = far / (far - near);
	m_uboData.projParams.y = (-far * near) / (far - near);

	m_ubo = engineInstance->createUniformBufferObject();
	m_ubo->initializeData(&m_uboData, sizeof(UBOData));
	
	Scene::ComputePassCreateInfo computePassCreateInfo;
	computePassCreateInfo.extent = engineInstance->getWindowSize();
	computePassCreateInfo.dispatchGroups = { 16, 16, 1 };
	computePassCreateInfo.computeShaderPath = "Shaders/SSAO/comp.spv";
	computePassCreateInfo.commandBufferID = commandBufferID;

	ImageLayout depthLayout{};
	depthLayout.accessibility = VK_SHADER_STAGE_COMPUTE_BIT;
	depthLayout.binding = 0;

	ImageLayout normalLayout{};
	normalLayout.accessibility = VK_SHADER_STAGE_COMPUTE_BIT;
	normalLayout.binding = 1;

	ImageLayout outputLayout{};
	outputLayout.accessibility = VK_SHADER_STAGE_COMPUTE_BIT;
	outputLayout.binding = 2;

	m_outputTexture = engineInstance->createTexture();
	m_outputTexture->create(engineInstance->getWindowSize(), VK_IMAGE_USAGE_STORAGE_BIT, VK_FORMAT_R32_SFLOAT, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_ASPECT_COLOR_BIT);
	m_outputTexture->setImageLayout(VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT);

	computePassCreateInfo.images = { { depth, depthLayout }, { normal, normalLayout }, { m_outputTexture->getImage(), outputLayout} };

	UniformBufferObjectLayout uboLayout{};
	uboLayout.accessibility = VK_SHADER_STAGE_COMPUTE_BIT;
	uboLayout.binding = 3;
	
	computePassCreateInfo.ubos = { { m_ubo, uboLayout } };
	
	m_computePassID = scene->addComputePass(computePassCreateInfo);

	m_blur = std::make_unique<Blur>(engineInstance, scene, commandBufferID, m_outputTexture->getImage(), nullptr);
}
