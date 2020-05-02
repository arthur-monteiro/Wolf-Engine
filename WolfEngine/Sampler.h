#pragma once

#include "VulkanElement.h"

namespace Wolf
{
	struct SamplerLayout
	{
		VkShaderStageFlags accessibility;
		uint32_t binding;
	};
	
	class Sampler : public VulkanElement
	{
	public:
		Sampler(VkDevice device);
		~Sampler();

		void initialize(VkSamplerAddressMode addressMode, float mipLevels, VkFilter filter, float maxAnisotropy = 16.0f);
		void cleanup(VkDevice device);

		VkSampler getSampler() { return m_textureSampler; }

	private:
		VkSampler m_textureSampler = VK_NULL_HANDLE;
	};


}
