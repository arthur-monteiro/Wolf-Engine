#pragma once

#include <cstdint>

#include "VulkanElement.h"
#include "Image.h"
#include "CommandBuffer.h"
#include "CommandPool.h"
#include "RenderPass.h"
#include "Renderer.h"
#include "InputVertexTemplate.h"
#include "Model.h"
#include "DescriptorPool.h"
#include "Text.h"
#include "InstanceTemplate.h"

namespace Wolf
{
	
	class Scene : public VulkanElement
	{
	public:
		enum class CommandType { GRAPHICS, COMPUTE };
		struct	SceneCreateInfo
		{
			CommandType swapChainCommandType = CommandType::GRAPHICS;
		};
		
		Scene(SceneCreateInfo createInfo, VkDevice device, VkPhysicalDevice physicalDevice, std::vector<Image*> swapChainImages, VkCommandPool graphicsCommandPool, VkCommandPool computeCommandPool);

		struct RenderPassCreateInfo
		{
			int commandBufferID;
			VkExtent2D extent;
		};
		int addRenderPass(RenderPassCreateInfo createInfo);

		struct RendererCreateInfo
		{
			int renderPassID;

			std::string vertexShaderPath;
			std::string fragmentShaderPath;

			InputVertexTemplate inputVerticesTemplate = InputVertexTemplate::NO;
			InstanceTemplate instanceTemplate = InstanceTemplate::NO;
			std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions;
			std::vector<VkVertexInputBindingDescription> inputBindingDescriptions;

			// Descriptor set layout
			std::vector<UniformBufferObjectLayout> uboLayouts;
			std::vector<TextureLayout> textureLayouts;
			std::vector<ImageLayout> imageLayouts;
			std::vector<SamplerLayout> samplerLayouts;
			std::vector<BufferLayout> bufferLayouts;
		};
		int addRenderer(RendererCreateInfo createInfo);

		struct AddModelInfo
		{
			Model* model;
			int renderPassID;
			int rendererID;

			std::vector<std::pair<UniformBufferObject*, UniformBufferObjectLayout>> ubos;
			std::vector<std::pair<Texture*, TextureLayout>> textures;
			std::vector<std::pair<Image*, ImageLayout>> images;
			std::vector<std::pair<Sampler*, SamplerLayout>> samplers;
			std::vector<std::pair<VkBuffer, BufferLayout>> buffers;
		};
		void addModel(AddModelInfo addModelInfo);

		template<typename T>
		void addModelInstancied(AddModelInfo addModelInfo, Instance<T>* instanceBuffer);

		struct AddTextInfo
		{
			Text* text;
			Font* font;
			float size;
			int renderPassID;
			int rendererID;

			// Info to add
			std::vector<std::pair<UniformBufferObject*, UniformBufferObjectLayout>> ubos;
			std::vector<std::pair<Texture*, TextureLayout>> textures;
			std::vector<std::pair<Image*, ImageLayout>> images;
			std::vector<std::pair<Sampler*, SamplerLayout>> samplers;
			std::vector<std::pair<VkBuffer, BufferLayout>> buffers;
		};
		void addText(AddTextInfo addTextInfo);
		
		void record();
		
		void frame(VkQueue graphicsQueue, uint32_t swapChainImageIndex, Semaphore* imageAvailableSemaphore);

		VkSemaphore getSwapChainSemaphore() const { return m_swapChainCompleteSemaphore->getSemaphore(); }

	private:
		// Command Pools
		VkCommandPool m_graphicsCommandPool;
		VkCommandPool m_computeCommandPool;

		// Descriptor Pool
		DescriptorPool m_descriptorPool;
		
		// SwapChain
		std::vector<Image*> m_swapChainImages;
		std::vector<std::unique_ptr<CommandBuffer>> m_swapChainCommandBuffers;
		std::unique_ptr<Semaphore> m_swapChainCompleteSemaphore;
		CommandType m_swapChainCommandType = CommandType::GRAPHICS;

		// CommandBuffer
		struct SceneCommandBuffer
		{
			std::unique_ptr<CommandBuffer> commandBuffer;
			std::unique_ptr<Semaphore> semaphore;
			CommandType type;
		};
		std::vector<SceneCommandBuffer> m_sceneCommandBuffers;

		// RenderPasses
		struct SceneRenderPass
		{
			std::unique_ptr<RenderPass> renderPass;
			int commandBufferID;
			std::vector<Attachment> attachments;
			std::vector<VkClearValue> clearValues;
			VkExtent2D extent;

			std::vector<std::unique_ptr<Renderer>> renderers;

			SceneRenderPass(int commandBufferID, std::vector<VkClearValue> clearValues, VkExtent2D extent)
			{
				this->clearValues = clearValues;
				this->commandBufferID = commandBufferID;
				this->extent = extent;
			}
		};
		std::vector<SceneRenderPass> m_sceneRenderPasses;
	};

	template <typename T>
	void Scene::addModelInstancied(AddModelInfo addModelInfo, Instance<T>* instanceBuffer)
	{
		m_descriptorPool.addUniformBuffer(static_cast<unsigned int>(addModelInfo.ubos.size()));
		m_descriptorPool.addCombinedImageSampler(static_cast<unsigned int>(addModelInfo.textures.size()));
		m_descriptorPool.addSampledImage(static_cast<unsigned int>(addModelInfo.images.size()));
		m_descriptorPool.addSampler(static_cast<unsigned int>(addModelInfo.samplers.size()));
		m_descriptorPool.addStorageBuffer(static_cast<unsigned int>(addModelInfo.buffers.size()));

		std::vector<VertexBuffer> vertexBuffers = addModelInfo.model->getVertexBuffers();
		for (VertexBuffer& vertexBuffer : vertexBuffers)
		{
			m_sceneRenderPasses[addModelInfo.renderPassID].renderers[addModelInfo.rendererID]->addMeshInstancied(vertexBuffer, instanceBuffer->getInstanceBuffer(), std::move(addModelInfo.ubos), std::move(addModelInfo.textures), 
				std::move(addModelInfo.images), std::move(addModelInfo.samplers), std::move(addModelInfo.buffers));
		}
	}
}
