#include "Texture.h"

Wolf::Texture::Texture(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue)
{
	m_device = device;
	m_physicalDevice = physicalDevice;
	m_commandPool = commandPool;
	m_graphicsQueue = graphicsQueue;	
}

Wolf::Texture::~Texture()
{
}

void Wolf::Texture::create(VkDevice device, VkPhysicalDevice physicalDevice, VkExtent2D extent, VkImageUsageFlags usage, VkFormat format, VkSampleCountFlagBits sampleCount, VkImageAspectFlags aspect)
{
	m_image.create(device, physicalDevice, extent, usage, format, sampleCount, aspect);
}

void Wolf::Texture::createFromImage(VkDevice device, Image* image)
{
	m_imagePtr = image;
}

void Wolf::Texture::createFromPixels(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool,
                                     VkQueue graphicsQueue, VkExtent3D extent, VkFormat format, unsigned char* pixels)
{
	m_image.createFromPixels(device, physicalDevice, commandPool, graphicsQueue, extent, format, pixels);
}

bool Wolf::Texture::loadFromFile(std::string filename)
{
	m_image.createFromFile(m_device, m_physicalDevice, m_commandPool, m_graphicsQueue, filename);

	return true;
}

void Wolf::Texture::createSampler(VkSamplerAddressMode addressMode, float mipLevels, VkFilter filter)
{
	m_sampler.initialize(m_device, addressMode, mipLevels, filter);
}

void Wolf::Texture::setImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkImageLayout newLayout, VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage)
{
	m_imagePtr == nullptr ?
		m_image.setImageLayout(device, commandPool, graphicsQueue, newLayout, sourceStage, destinationStage) :
		m_imagePtr->setImageLayout(device, commandPool, graphicsQueue, newLayout, sourceStage, destinationStage);
}

void Wolf::Texture::cleanup(VkDevice device)
{
	if (m_imagePtr == nullptr)
		m_image.cleanup(device);
	m_sampler.cleanup(device);
}
