#include "Attachment.h"

Wolf::Attachment::Attachment(VkFormat format, VkSampleCountFlagBits sampleCount, VkImageLayout finalLayout,
                             VkAttachmentStoreOp storeOperation, VkImageUsageFlags usageType)
{
	this->format = format;
	this->sampleCount = sampleCount;
	this->finalLayout = finalLayout;
	this->storeOperation = storeOperation;
	this->usageType = usageType;
}
