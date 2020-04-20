#include "Model.h"

Wolf::Model::Model(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, InputVertexTemplate inputVertexTemplate)
{
	m_device = device;
	m_physicalDevice = physicalDevice;
	m_commandPool = commandPool;
	m_graphicsQueue = graphicsQueue;

	m_inputVertexTemplate = inputVertexTemplate;
}

Wolf::Model::~Model()
{
}
