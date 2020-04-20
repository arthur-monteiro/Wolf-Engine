#pragma once

#include "Mesh.h"
#include "InputVertexTemplate.h"

namespace Wolf
{
	class Model
	{
	public:
		struct ModelCreateInfo
		{
			InputVertexTemplate inputVertexTemplate;
		};

		Model(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, InputVertexTemplate inputVertexTemplate);		
		virtual ~Model();

		virtual int addMeshFromVertices(void* vertices, uint32_t vertexCount, size_t vertexSize, std::vector<uint32_t> indices) { return -1; }
		virtual void loadObj(std::string filename, std::string mtlFolder) {}

		virtual std::vector<VertexBuffer> getVertexBuffers() { return {}; }
		
	protected:
		VkDevice m_device;
		VkPhysicalDevice m_physicalDevice;
		VkCommandPool m_commandPool;
		VkQueue m_graphicsQueue;

		InputVertexTemplate m_inputVertexTemplate;
	};
}
