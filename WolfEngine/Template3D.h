#pragma once

#include "WolfEngine.h"

namespace Wolf
{
	class Template3D
	{
	public:
		Template3D(Wolf::WolfInstance* wolfInstance, Wolf::Scene* scene, std::string modelFilename, std::string mtlFolder);

		void updateViewMatrix(glm::mat4 view);

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
	};
}

