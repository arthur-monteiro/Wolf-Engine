#pragma once

#include "VulkanHelper.h"
#include "Image.h"

namespace Wolf
{
	struct Attachment
	{
		VkExtent2D extent = {0, 0};
		VkFormat format;
		VkSampleCountFlagBits sampleCount;
		VkImageLayout finalLayout;
		VkAttachmentStoreOp storeOperation;

		VkImageUsageFlags usageType{};

		Image* image = nullptr;

		Attachment(VkExtent2D extent, VkFormat format, VkSampleCountFlagBits sampleCount, VkImageLayout finalLayout,
		           VkAttachmentStoreOp storeOperation, VkImageUsageFlags usageType, Image* image = nullptr);
		Attachment(): format(), sampleCount(), finalLayout(), storeOperation()
		{
		}
	};
}