#pragma once

#include <utility>


#include "VulkanHelper.h"
#include "Image.h"
#include "Texture.h"
#include "UniformBufferObject.h"
#include "Instance.h"
#include "Pipeline.h"
#include "Mesh.h"

namespace Wolf
{
	struct BufferLayout
	{
		VkShaderStageFlags accessibility;
		uint32_t binding;
		VkDeviceSize size;
	};
	
	class Renderer
	{
	public:
		~Renderer();

		Renderer(VkDevice device, VkExtent2D extent, std::string vertexShader, std::string fragmentShader, std::vector<VkVertexInputBindingDescription> vertexInputDescription,
			std::vector<VkVertexInputAttributeDescription> attributeInputDescription, std::vector<UniformBufferObjectLayout> uniformBufferObjectLayouts,
			std::vector<TextureLayout> textureLayouts, std::vector<ImageLayout> imageLayouts, std::vector<SamplerLayout> samplerLayouts, std::vector<BufferLayout> bufferLayouts,
			std::vector<bool> alphaBlending, bool enableDepthTesting, bool enableConservativeRasterization, VkPolygonMode polygonMode);
		Renderer(VkDevice device, VkExtent2D extent, std::string vertexShader, std::string geometryShader, std::string fragmentShader, std::vector<VkVertexInputBindingDescription> vertexInputDescription,
			std::vector<VkVertexInputAttributeDescription> attributeInputDescription, std::vector<UniformBufferObjectLayout> uniformBufferObjectLayouts,
			std::vector<TextureLayout> textureLayouts, std::vector<ImageLayout> imageLayouts, std::vector<SamplerLayout> samplerLayouts, std::vector<BufferLayout> bufferLayouts,
			std::vector<bool> alphaBlending);
		Renderer(VkDevice device, VkExtent2D extent, std::string vertexShader, std::string fragmentShader, 
			std::string tessellationControlShader, std::string tessellationEvaluationShader, std::vector<VkVertexInputBindingDescription> vertexInputDescription,
			std::vector<VkVertexInputAttributeDescription> attributeInputDescription, std::vector<UniformBufferObjectLayout> uniformBufferObjectLayouts,
			std::vector<TextureLayout> textureLayouts, std::vector<ImageLayout> imageLayouts, std::vector<SamplerLayout> samplerLayouts, std::vector<BufferLayout> bufferLayouts,
			std::vector<bool> alphaBlending, bool enableDepthTesting, bool enableConservativeRasterization, VkPolygonMode polygonMode);
		Renderer(VkDevice device, VkExtent2D extent, std::string vertexShader, std::vector<VkVertexInputBindingDescription> vertexInputDescription,
			std::vector<VkVertexInputAttributeDescription> attributeInputDescription, std::vector<UniformBufferObjectLayout> uniformBufferObjectLayouts,
			std::vector<TextureLayout> textureLayouts, std::vector<bool> alphaBlending);
		void create(VkDevice device, VkRenderPass renderPass, VkSampleCountFlagBits msaa, VkDescriptorPool descriptorPool);
		void destroyPipeline(VkDevice device);

		void setViewport(std::array<float, 2> viewportScale, std::array<float, 2> viewportOffset);

		int addMesh(VertexBuffer vertexBuffer,
			std::vector<std::pair<UniformBufferObject*, UniformBufferObjectLayout>> ubos, std::vector<std::pair<Texture*, TextureLayout>> textures,
			std::vector<std::pair<Image*, ImageLayout>> images, std::vector<std::pair<Sampler*, SamplerLayout>> samplers, std::vector<std::pair<VkBuffer, BufferLayout>> buffers);
		int addMeshInstancied(VertexBuffer vertexBuffer, InstanceBuffer instanceBuffer,
			std::vector<std::pair<UniformBufferObject*, UniformBufferObjectLayout>> ubos, std::vector<std::pair<Texture*, TextureLayout>> textures,
			std::vector<std::pair<Image*, ImageLayout>> images, std::vector<std::pair<Sampler*, SamplerLayout>> samplers, std::vector<std::pair<VkBuffer, BufferLayout>> buffers);
		void reloadMeshVertexBuffer(VkDevice device, VertexBuffer vertexBuffer, int meshID);

		void cleanup(VkDevice device, VkDescriptorPool descriptorPool);

		VkPipeline getPipeline() { return m_pipeline.getPipeline(); }
		std::vector<std::tuple<VertexBuffer, InstanceBuffer, VkDescriptorSet>> getMeshes();
		VkPipelineLayout getPipelineLayout() { return m_pipeline.getPipelineLayout(); }

		void setPipelineCreated(bool status) { m_pipelineCreated = status; }

	private:
		/* Information */
		std::string m_vertexShader;
		std::string m_geometryShader;
		std::string m_fragmentShader;
		std::string m_tessellationControlShader;
		std::string m_tessellationEvaluationShader;
		std::vector<VkVertexInputBindingDescription> m_vertexInputDescription;
		std::vector<VkVertexInputAttributeDescription> m_attributeInputDescription;
		std::vector<bool> m_alphaBlending;
		VkPrimitiveTopology m_primitiveTopoly;
		bool m_enableDepthTesting;
		bool m_enableConservativeRasterization;
		VkPolygonMode m_polygonMode;

		VkExtent2D m_extent = { 0, 0};
		std::array<float, 2> m_viewportScale = { 1.0f, 1.0f };
		std::array<float, 2> m_viewportOffset = { 0.0f, 0.0f };

		VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;

		struct MeshInfo
		{
			VertexBuffer vertexBuffer;
			InstanceBuffer instanceBuffer;
			VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

			// Info needed to create descriptor set
			std::vector<std::pair<UniformBufferObject*, UniformBufferObjectLayout>> ubos;
			std::vector<std::pair<Texture*, TextureLayout>> textures;
			std::vector<std::pair<Image*, ImageLayout>> images;
			std::vector<std::pair<Sampler*, SamplerLayout>> samplers;
			std::vector<std::pair<VkBuffer, BufferLayout>> buffers;

			MeshInfo(VertexBuffer vertexBuffer, std::vector<std::pair<UniformBufferObject*, UniformBufferObjectLayout>> ubos, std::vector<std::pair<Texture*, TextureLayout>> textures,
				std::vector<std::pair<Image*, ImageLayout>> images, std::vector<std::pair<Sampler*, SamplerLayout>> samplers, std::vector<std::pair<VkBuffer, BufferLayout>> buffers, InstanceBuffer instanceBuffer = InstanceBuffer())
			{
				this->vertexBuffer = vertexBuffer;
				this->instanceBuffer = instanceBuffer;
				
				this->ubos = std::move(ubos);
				this->textures = std::move(textures);
				this->images = std::move(images);
				this->samplers = std::move(samplers);
				this->buffers = std::move(buffers);
			}

			bool needDescriptorSet() const
			{
				return !ubos.empty() || !textures.empty() || !images.empty() || !samplers.empty() || !buffers.empty();
			}
		};
		std::vector<MeshInfo> m_meshes;
		std::vector<std::tuple<VertexBuffer, InstanceBuffer, VkDescriptorSet>> m_meshesInstancied;

		Pipeline m_pipeline;
		bool m_pipelineCreated = false;

	private:
		void createDescriptorSetLayout(VkDevice device, std::vector<UniformBufferObjectLayout> uniformBufferObjectLayouts, std::vector<TextureLayout> textureLayouts,
			std::vector<ImageLayout> imageLayouts, std::vector<SamplerLayout> samplerLayouts, std::vector<BufferLayout> bufferLayouts);
		VkDescriptorSet createDescriptorSet(VkDevice device, VkDescriptorPool descriptorPool,
			std::vector<std::pair<UniformBufferObject*, UniformBufferObjectLayout>> ubos, std::vector<std::pair<Texture*, TextureLayout>> textures,
			std::vector<std::pair<Image*, ImageLayout>> images, std::vector<std::pair<Sampler*, SamplerLayout>> samplers, std::vector<std::pair<VkBuffer, BufferLayout>> buffers);
	};

}
