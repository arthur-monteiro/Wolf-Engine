#pragma once

#include "Model.h"
#include "Mesh.h"
#include "InputVertexTemplate.h"

namespace Wolf
{
	class Model3D : public Model
	{
	public:
		Model3D(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, InputVertexTemplate inputVertexTemplate) : Model(device, physicalDevice, commandPool, graphicsQueue, inputVertexTemplate) {};
		~Model3D();

		int addMeshFromVertices(void* vertices, uint32_t vertexCount, size_t vertexSize, std::vector<uint32_t> indices);
		void loadObj(std::string filename, std::string mtlFolder);

		std::vector<Wolf::VertexBuffer> getVertexBuffers();

	private:
		std::vector<Wolf::Mesh<Vertex3D>> m_meshes;
	};
}