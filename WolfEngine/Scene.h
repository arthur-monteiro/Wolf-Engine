#pragma once

#include <cstdint>
#include <utility>

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
#include "ComputePass.h"

namespace Wolf
{
	
	class Scene : public VulkanElement
	{
	public:
		enum class CommandType { GRAPHICS, COMPUTE, TRANSFER };
		struct	SceneCreateInfo
		{
			CommandType swapChainCommandType = CommandType::GRAPHICS;
		};
		
		Scene(SceneCreateInfo createInfo, VkDevice device, VkPhysicalDevice physicalDevice, std::vector<Image*> swapChainImages, VkCommandPool graphicsCommandPool, VkCommandPool computeCommandPool);
		Scene(SceneCreateInfo createInfo, VkDevice device, VkPhysicalDevice physicalDevice, std::vector<Image*> ovrSwapChainImages, std::vector<Image*> windowSwapChainImages, VkCommandPool graphicsCommandPool, VkCommandPool computeCommandPool);

		struct RenderPassOutput
		{
			VkClearValue clearValue;
			Attachment attachment;
		};

		struct RenderPassCreateInfo
		{
			int commandBufferID;
			bool outputIsSwapChain = false;

			// Output
			std::vector<RenderPassOutput> outputs;
		};
		int addRenderPass(RenderPassCreateInfo createInfo);

		struct ComputePassCreateInfo
		{
			int commandBufferID;
			bool outputIsSwapChain = false;
			uint32_t outputBinding = 0;
			VkExtent2D extent;
			VkExtent3D dispatchGroups;
			
			std::string computeShaderPath;

			std::vector<std::pair<UniformBufferObject*, UniformBufferObjectLayout>> ubos;
			//std::vector<std::pair<Texture*, TextureLayout>> textures;
			std::vector<std::pair<Image*, ImageLayout>> images;
			std::vector<std::pair<Sampler*, SamplerLayout>> samplers;
			std::vector<std::pair<VkBuffer, BufferLayout>> buffers;

			std::function<void(void*, VkCommandBuffer)> beforeRecord = nullptr; void* dataForBeforeRecordCallback = nullptr;
			std::function<void(void*, VkCommandBuffer)> afterRecord = nullptr; void* dataForAfterRecordCallback = nullptr;
		};
		int addComputePass(ComputePassCreateInfo createInfo);

		struct CommandBufferCreateInfo
		{
			CommandType commandType = CommandType::COMPUTE;
			VkPipelineStageFlags finalPipelineStage;
		};
		int addCommandBuffer(CommandBufferCreateInfo createInfo);

		struct RendererCreateInfo
		{
			int renderPassID;

			std::string vertexShaderPath;
			std::string fragmentShaderPath;

			InputVertexTemplate inputVerticesTemplate = InputVertexTemplate::NO;
			InstanceTemplate instanceTemplate = InstanceTemplate::NO;
			std::vector<VkVertexInputAttributeDescription> inputAttributeDescriptions;
			std::vector<VkVertexInputBindingDescription> inputBindingDescriptions;
			std::vector<bool> alphaBlending = { true };

			// Descriptor set layout
			std::vector<UniformBufferObjectLayout> uboLayouts;
			std::vector<TextureLayout> textureLayouts;
			std::vector<ImageLayout> imageLayouts;
			std::vector<SamplerLayout> samplerLayouts;
			std::vector<BufferLayout> bufferLayouts;

			// Viewport
			VkExtent2D extent = { 0, 0 };
			std::array<float, 2> viewportScale = { 1.0f, 1.0f };
			std::array<float, 2> viewportOffset = { 0.0f, 0.0f };
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
		
		void frame(Queue graphicsQueue, Queue computeQueue, uint32_t swapChainImageIndex, Semaphore* imageAvailableSemaphore, std::vector<int> commandBufferIDs,
		           const std::vector<std::pair<int, int>>&);

		VkSemaphore getSwapChainSemaphore() const { return m_swapChainCompleteSemaphore->getSemaphore(); }
		Image* getRenderPassOutput(int renderPassID, int textureID) { return m_sceneRenderPasses[renderPassID].renderPass->getImages(0)[textureID]; }

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

		// VR
		std::vector<Image*> m_windowSwapChainImages; // mirror images

		// CommandBuffer
		struct SceneCommandBuffer
		{
			std::unique_ptr<CommandBuffer> commandBuffer;
			std::unique_ptr<Semaphore> semaphore;
			CommandType type;

			SceneCommandBuffer(CommandType type)
			{
				this->type = type;
			}
		};
		std::vector<SceneCommandBuffer> m_sceneCommandBuffers;

		// RenderPasses
		struct SceneRenderPass
		{
			std::unique_ptr<RenderPass> renderPass;
			int commandBufferID;

			// Output
			std::vector<RenderPassOutput> outputs;
			bool outputIsSwapChain = false;

			std::vector<std::unique_ptr<Renderer>> renderers;

			SceneRenderPass(int commandBufferID, std::vector<RenderPassOutput> outputs, bool outputIsSwapChain)
			{
				this->outputs = std::move(outputs);
				this->outputIsSwapChain = outputIsSwapChain;
				this->commandBufferID = commandBufferID;
			}
		};
		std::vector<SceneRenderPass> m_sceneRenderPasses;

		struct SceneComputePass
		{
			std::vector<std::unique_ptr<ComputePass>> computePasses;
			int commandBufferID;
			
			bool outputIsSwapChain = false;
			uint32_t outputBinding = 0;
			VkExtent2D extent;
			VkExtent3D dispatchGroups;

			std::function<void(void*, VkCommandBuffer)> beforeRecord = nullptr; void* dataForBeforeRecordCallback = nullptr;
			std::function<void(void*, VkCommandBuffer)> afterRecord = nullptr; void* dataForAfterRecordCallback = nullptr;

			SceneComputePass(int commandBufferID, bool outputIsSwapChain)
			{
				this->outputIsSwapChain = outputIsSwapChain;
				this->commandBufferID = commandBufferID;
			}
		};
		std::vector<SceneComputePass> m_sceneComputePasses;
		
		// VR
		bool m_useOVR = false;

	private:
		inline void recordRenderPass(SceneRenderPass& sceneRenderPasse);
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
