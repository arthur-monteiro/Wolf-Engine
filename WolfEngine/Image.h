#pragma once

#include <iostream>
#include <cmath>
#include <cstring>
#include <array>

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "VulkanHelper.h"

namespace Wolf
{
	struct ImageLayout
	{
		VkShaderStageFlags accessibility;
		uint32_t binding;
	};

	class Image
	{
	public:
		void create(VkDevice device, VkPhysicalDevice physicalDevice, VkExtent2D extent, VkImageUsageFlags usage, VkFormat format, VkSampleCountFlagBits sampleCount, VkImageAspectFlags aspect);
		void createFromImage(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspect, VkExtent2D extent);
		void createFromPixels(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, VkExtent3D extent, VkFormat format, unsigned char* pixels);
		void createFromBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, VkExtent3D extent, VkFormat format, VkBuffer buffer);
		void createFromFile(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, std::string filename);
		void createCubeMapFromImages(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, std::array<Image*, 6> images);

		void setImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkImageLayout newLayout, VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage);
		void setImageLayoutWithoutOperation(VkImageLayout newImageLayout) { m_imageLayout = newImageLayout; }

		void cleanup(VkDevice device);

		VkImage getImage() { return m_image; }
		VkDeviceMemory getImageMemory() { return m_imageMemory; }
		VkImageView getImageView() { return m_imageView; }
		VkFormat getFormat() { return m_imageFormat; }
		VkSampleCountFlagBits getSampleCount() { return m_sampleCount; }
		VkExtent2D getExtent() { return m_extent; }
		VkImageLayout getImageLayout() { return m_imageLayout; }
		uint32_t getMipLevels() { return m_mipLevels; }

	private:
		VkImage m_image;
		VkDeviceMemory  m_imageMemory;
		VkImageView m_imageView = VK_NULL_HANDLE;

		VkImageLayout m_imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkFormat m_imageFormat;

		uint32_t m_mipLevels;
		VkExtent2D m_extent;
		VkSampleCountFlagBits m_sampleCount;

	private:
		static void createImage(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format,
			VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, uint32_t arrayLayers, VkImageCreateFlags flags, VkImageLayout initialLayout,
			VkImage& image, VkDeviceMemory& imageMemory);
		static VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels, VkImageViewType viewType);
		static void transitionImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,
			uint32_t mipLevels, uint32_t arrayLayers, VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage);
		static void copyBufferToImage(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t baseArrayLayer);
		static void generateMipmaps(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue, VkImage image, VkFormat imageFormat, int32_t texWidth,
			int32_t texHeight, uint32_t mipLevels, uint32_t baseArrayLayer);

	public:
		static void transitionImageLayoutUsingCommandBuffer(VkCommandBuffer commandBuffer, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout,
			uint32_t mipLevels, VkPipelineStageFlags sourceStage, VkPipelineStageFlags destinationStage,
			uint32_t arrayLayer);
	};
}