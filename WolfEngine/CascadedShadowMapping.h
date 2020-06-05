#pragma once

#include "DepthPass.h"
//#include "Blur.h"
#include "Model.h"

namespace Wolf
{
	constexpr int CASCADE_COUNT = 4;
	
	class CascadedShadowMapping
	{
	public:
		CascadedShadowMapping(Wolf::WolfInstance* engineInstance, Wolf::Scene* scene, Model* model, float cameraNear, float cameraFar, float cameraFOV, VkExtent2D extent);
		~CascadedShadowMapping();

		void updateMatrices(glm::vec3 lightDir, glm::vec3 cameraPosition, glm::vec3 cameraOrientation, glm::mat4 model);
		
		/*void submit(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkPhysicalDevice physicalDevice, VkDescriptorPool descriptorPool, glm::mat4 view, glm::mat4 model, glm::mat4 projection, float cameraNear, float cameraFOV, glm::vec3 lightDir,
			glm::vec3 cameraPosition, glm::vec3 cameraOrientation);
		void cleanup(VkDevice device, VkCommandPool commandPool, VkDescriptorPool descriptorPool);

		void setSoftShadowsOption(glm::uint softShadowsOption);
		void setSSIterations(glm::uint nIterations);
		void setSamplingDivisor(float divisor);
		void setBlurAmount(int blurAmount);*/

		// Getters
	public:
		/*Semaphore* getSemaphore() { return m_blurAmount > 0 ? m_blur.getSemaphore() : m_renderPass.getRenderCompleteSemaphore(); }
		Texture* getOutputTexture() { return &m_outputTexture; }*/

	private:
		Wolf::WolfInstance* m_engineInstance;
		Wolf::Scene* m_scene;
	
		std::array<std::unique_ptr<DepthPass>, CASCADE_COUNT> m_depthPasses;
		std::array<int, CASCADE_COUNT> m_cascadeCommandBuffers;
		std::array<glm::mat4, CASCADE_COUNT> m_lightSpaceMatrices;

		/* Camera params */
		float m_cameraNear;
		float m_cameraFar;
		float m_cameraFOV;
		float m_ratio;
		VkExtent2D m_extent;

		/* Shadow Mask*/
		int m_shadowMaskRenderPass = -1;
		Attachment m_shadowMaskAttachment;
		int m_shadowMaskCommandBuffer = -2;
		int m_shadowMaskRenderer = -1;

		struct ShadowMaskUBO
		{
			glm::mat4 mvp;
			std::array<glm::mat4, CASCADE_COUNT> lightSpaceMatrices;
			glm::vec4 cascadeSplits;
			//glm::ivec4 softShadowsOption = glm::ivec4(0, 8, 700, 0);
		};

		/*RenderPass m_renderPass;
		std::vector<Attachment> m_attachments;
		std::vector<VkClearValue> m_clearValues;
		Renderer m_renderer;
		Texture m_outputTexture;*/

		//Blur m_blur;
		//int m_blurAmount = 0;
		//int m_updateBlurAmount = -1;

		///*struct CascadedShadowMappingUBO
		//{
		//	glm::mat4 mvp;
		//	std::array<glm::mat4, CASCADE_COUNT> lightSpaceMatrices;
		//	glm::vec4 cascadeSplits;
		//	glm::ivec4 softShadowsOption = glm::ivec4(0, 8, 700, 0);
		//};
		//CascadedShadowMappingUBO m_uboData;
		//UniformBufferObject* m_ubo;*/

		std::vector<float> m_cascadeSplits;
		std::vector<VkExtent2D> m_shadowMapExtents;
	};
}

