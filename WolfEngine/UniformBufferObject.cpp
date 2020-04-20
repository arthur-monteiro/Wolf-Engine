#include "UniformBufferObject.h"

Wolf::UniformBufferObject::UniformBufferObject(VkDevice device, VkPhysicalDevice physicalDevice)
{
	m_device = device;
	m_physicalDevice = physicalDevice;
}

Wolf::UniformBufferObject::~UniformBufferObject()
{
	if (m_size <= 0)
		return;

	vkDestroyBuffer(m_device, m_uniformBuffer, nullptr);
	vkFreeMemory(m_device, m_uniformBufferMemory, nullptr);

	m_size = 0;
}

bool Wolf::UniformBufferObject::initializeData(void* data, VkDeviceSize size)
{
	createBuffer(m_device, m_physicalDevice, size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_uniformBuffer, m_uniformBufferMemory);

	m_size = size;

	void* pData;
	vkMapMemory(m_device, m_uniformBufferMemory, 0, m_size, 0, &pData);
	memcpy(pData, data, m_size);
	vkUnmapMemory(m_device, m_uniformBufferMemory);

	return true;
}

void Wolf::UniformBufferObject::updateData(void* data)
{
	void* pData;
	vkMapMemory(m_device, m_uniformBufferMemory, 0, m_size, 0, &pData);
	memcpy(pData, data, m_size);
	vkUnmapMemory(m_device, m_uniformBufferMemory);
}

void Wolf::UniformBufferObject::cleanup()
{
	this->~UniformBufferObject();
}
