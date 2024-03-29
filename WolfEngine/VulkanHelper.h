#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <vector>
#include <mutex>

struct QueueFamilyIndices
{
	int graphicsFamily = -1;
	int presentFamily = -1;
	int computeFamily = -1;

	bool isComplete()
	{
		return graphicsFamily >= 0 && presentFamily >= 0 && computeFamily >= 0;
	}
};

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};

struct HardwareCapabilities
{
	bool rayTracingAvailable = false;
	bool meshShaderAvailable = false;
	VkDeviceSize VRAMSize = 0;
};

struct Queue
{
	VkQueue queue;
	std::mutex* mutex;
};

std::vector<const char*> getRequiredExtensions();
bool isDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, std::vector<const char*> deviceExtensions, HardwareCapabilities& outHardwareCapabilities);
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDevice physicalDevice);
bool checkDeviceExtensionSupport(VkPhysicalDevice device, std::vector<const char*> deviceExtensions);
SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
VkFormat findDepthFormat(VkPhysicalDevice physicalDevice);
VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features, VkPhysicalDevice physicalDevice);
void createBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
void copyBuffer(VkDevice device, VkCommandPool commandPool, Queue graphicsQueue, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);
void endSingleTimeCommands(VkDevice device, Queue graphicsQueue, VkCommandBuffer commandBuffer, VkCommandPool commandPool);
VkCommandPool createCommandPool(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, uint32_t queueFamilyIndex);
bool hasStencilComponent(VkFormat format);
bool hasDepthComponent(VkFormat format);
VkPhysicalDeviceRayTracingPropertiesNV getPhysicalDeviceRayTracingProperties(VkPhysicalDevice physicalDevice);
VkPhysicalDeviceMeshShaderPropertiesNV getPhysicalDeviceMeshShaderProperties(VkPhysicalDevice physicalDevice);
void copyImage(VkDevice device, VkCommandPool commandPool, Queue graphicsQueue, VkImage source, VkImage dst, uint32_t width, uint32_t height, uint32_t baseArrayLayer, uint32_t dstMipLevel);

// Ray Tracing
VkAccelerationStructureNV createAccelerationStructure(VkDevice device, std::vector<VkGeometryNV> geometry, VkAccelerationStructureTypeNV accelerationStructureType, uint32_t instanceCount,
	VkBuildAccelerationStructureFlagsNV buildFlags);