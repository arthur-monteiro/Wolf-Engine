#pragma once

#include "WolfEngine.h"

// Effects
#include "GBuffer.h"
#include "CascadedShadowMapping.h"

namespace Wolf
{
	class Template3D
	{
	public:
		Template3D(Wolf::WolfInstance* wolfInstance, Wolf::Scene* scene, std::string modelFilename, std::string mtlFolder, float ratio);

		void update(glm::mat4 view, glm::vec3 cameraPosition, glm::vec3 cameraOrientation);

		// Getters
		std::vector<int> getCommandBufferToSubmit();
		std::vector<std::pair<int, int>> getCommandBufferSynchronisation();

	private:
		void updateMVP();
		
	private:
		Wolf::WolfInstance* m_wolfInstance;
		Wolf::Scene* m_scene;

		// Render Pass
		int m_renderPassID = -1;
		int m_rendererID = -1;

		// Data
		UniformBufferObject* m_uboMVP;
		glm::mat4 m_projectionMatrix;
		glm::mat4 m_viewMatrix;
		glm::mat4 m_modelMatrix;

		// Effects
		int m_gBufferCommandBufferID = -2;
		std::unique_ptr<GBuffer> m_GBuffer;
		std::unique_ptr<CascadedShadowMapping> m_cascadedShadowMapping;
	};
}

