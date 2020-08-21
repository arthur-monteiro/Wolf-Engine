#pragma once

#include "WolfEngine.h"

namespace Wolf
{
	class Template3D_VR
	{
	public:
		Template3D_VR(Wolf::WolfInstance* wolfInstance, Wolf::Scene* scene, std::string modelFilename, std::string mtlFolder);

		void update();

	private:
		void updateMVP();

	private:
		Wolf::WolfInstance* m_wolfInstance;
		Wolf::Scene* m_scene;

		// Render Pass
		int m_renderPassID = -1;
		std::array<int , 2> m_rendererIDs = { -1, -1 };

		// Data
		std::array<UniformBuffer*, 2> m_ubosMVP;
	};
}

