#pragma once

#include "DepthPass.h"
#include "Blur.h"
#include "Model.h"

namespace Wolf
{
	constexpr int CASCADE_COUNT = 4;
	
	class CascadedShadowMapping
	{
	public:
		CascadedShadowMapping(Wolf::WolfInstance* engineInstance, Wolf::Scene* scene, Model* model, float cameraNear, float cameraFar, float shadowFar, float cameraFOV, VkExtent2D extent,
			Image* depth, glm::mat4 projection);

		void updateMatrices(glm::vec3 lightDir, glm::vec3 cameraPosition, glm::vec3 cameraOrientation, glm::mat4 model, glm::mat4 invModelView);
		
		/*void submit(VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool, VkPhysicalDevice physicalDevice, VkDescriptorPool descriptorPool, glm::mat4 view, glm::mat4 model, glm::mat4 projection, float cameraNear, float cameraFOV, glm::vec3 m_lightDir,
			glm::vec3 cameraPosition, glm::vec3 cameraOrientation);
		void cleanup(VkDevice device, VkCommandPool commandPool, VkDescriptorPool descriptorPool);

		void setSoftShadowsOption(glm::uint softShadowsOption);
		void setSSIterations(glm::uint nIterations);
		void setSamplingDivisor(float divisor);
		void setBlurAmount(int blurAmount);*/

		// Getters
	public:
		/*Semaphore* getSemaphore() { return m_blurAmount > 0 ? m_blur.getSemaphore() : m_renderPass.getRenderCompleteSemaphore(); }
		Texture* getOutputShadowMaskTexture() { return &m_outputTexture; }*/
		std::vector<int> getCascadeCommandBuffers()
		{
			std::vector<int> r(m_cascadeCommandBuffers.begin(), m_cascadeCommandBuffers.end());
			r.push_back(m_shadowMaskCommandBufferID);
			std::vector<int> blurCommandBuffers = m_blur->getCommandBufferIDs();
			for (auto& commandBuffer : blurCommandBuffers)
				r.push_back(commandBuffer);
			return r;
		}
		std::vector<std::pair<int, int>> getCommandBufferSynchronisation()
		{
			std::vector<std::pair<int, int>> r;
			for(auto& commandBuffer : m_cascadeCommandBuffers)
			{
				r.emplace_back(commandBuffer, m_shadowMaskCommandBufferID);
			}

			std::vector<std::pair<int, int>> blurSync = m_blur->getCommandBufferSynchronisation();
			r.emplace_back(m_shadowMaskCommandBufferID, blurSync[0].first);
			for (auto& sync : blurSync)
				r.push_back(sync);

			return r;
		}

		Texture* getOutputShadowMaskTexture() { return m_shadowMaskOutputTexture; }
		Texture* getOutputVolumetricLightMaskTexture() { return m_blur->getTextureOutput(); }

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
		int m_shadowMaskComputePassID;
		Texture* m_shadowMaskOutputTexture;
		Texture* m_volumetricLightOutputTexture;
		int m_shadowMaskCommandBufferID = -2;

		struct ShadowMaskUBO
		{
			glm::mat4 invModelView;
			glm::mat4 invProjection;
			glm::vec4 projectionParams;
			std::array<glm::mat4, CASCADE_COUNT> lightSpaceMatrices;
			glm::vec4 cascadeSplits;
		};
		ShadowMaskUBO m_uboData;
		UniformBufferObject* m_ubo;
		
		std::vector<float> m_cascadeSplits;
		std::vector<VkExtent2D> m_shadowMapExtents;

		std::unique_ptr<Blur> m_blur;
	};
}

