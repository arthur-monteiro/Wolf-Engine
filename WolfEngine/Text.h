#pragma once

#include "Mesh.h"
#include "Font.h"
#include "UniformBufferObject.h"
#include "InputVertexTemplate.h"

namespace Wolf
{
	class Text : public VulkanElement
	{
	public:
		Text(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, VkQueue graphicsQueue);
		~Text();

		int addWString(std::wstring text, glm::vec2 position, glm::vec3 color);
		void build(VkExtent2D outputExtent, Font* font, float size);

		float simulateSizeX(std::wstring text, VkExtent2D outputExtent, Font* font, float maxSize);

		VertexBuffer getVertexBuffer() { return m_mesh.getVertexBuffer(); }
		UniformBufferObject* getUBO() const { return m_ubo.get(); }

		void setColor(VkDevice device, unsigned int ID, glm::vec3 color);
		void translate(VkDevice device, unsigned int ID, glm::vec2 offset);
		void setPosOffset(VkDevice device, unsigned int ID, glm::vec2 offset);

	private:
		void updateUBO(VkDevice device);

	private:
		struct TextStructure
		{
			glm::vec2 position;
			glm::vec2 posOffset = glm::vec2(0.0f);
			std::wstring textValue;
			glm::vec3 color;

			TextStructure(const glm::vec2 pos, std::wstring text, glm::vec3 color) : position(pos), textValue(std::move(text)), color(color) {}
		};
		std::vector<TextStructure> m_texts;

		Mesh<Vertex2DTexturedWithMaterial> m_mesh;
		std::unique_ptr<UniformBufferObject> m_ubo;

		struct TextUBO
		{
			std::array<glm::vec4, 128> color;
			std::array<glm::vec4, 128> posOffset;
		};
		TextUBO m_uboData;
	};
}
