#include "Template3D_VR.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

Wolf::Template3D_VR::Template3D_VR(Wolf::WolfInstance* wolfInstance, Wolf::Scene* scene, std::string modelFilename,
	std::string mtlFolder) : m_wolfInstance(wolfInstance), m_scene(scene)
{
	// Render Pass Creation
	Scene::RenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.commandBufferID = -1; // default command buffer
	renderPassCreateInfo.outputIsSwapChain = true;
	m_renderPassID = m_scene->addRenderPass(renderPassCreateInfo);

	Model::ModelCreateInfo modelCreateInfo{};
	modelCreateInfo.inputVertexTemplate = InputVertexTemplate::FULL_3D_MATERIAL;
	Model* model = m_wolfInstance->createModel(modelCreateInfo);

	Model::ModelLoadingInfo modelLoadingInfo;
	modelLoadingInfo.filename = std::move(modelFilename);
	modelLoadingInfo.mtlFolder = std::move(mtlFolder);
	model->loadObj(modelLoadingInfo);

	// For each eye (normally it should stay 2)
	for(int i(0); i < 2; ++i)
	{
		Scene::RendererCreateInfo rendererCreateInfo;
		rendererCreateInfo.vertexShaderPath = "Shaders/template3D/vert.spv";
		rendererCreateInfo.fragmentShaderPath = "Shaders/template3D/frag.spv";
		rendererCreateInfo.inputVerticesTemplate = InputVertexTemplate::FULL_3D_MATERIAL;
		rendererCreateInfo.instanceTemplate = InstanceTemplate::NO;
		rendererCreateInfo.renderPassID = m_renderPassID;

		UniformBufferObjectLayout mvpLayout{};
		mvpLayout.accessibility = VK_SHADER_STAGE_VERTEX_BIT;
		mvpLayout.binding = 0;
		rendererCreateInfo.uboLayouts.push_back(mvpLayout);

		SamplerLayout samplerLayout{};
		samplerLayout.accessibility = VK_SHADER_STAGE_FRAGMENT_BIT;
		samplerLayout.binding = 1;
		rendererCreateInfo.samplerLayouts.push_back(samplerLayout);

		for (size_t i(0); i < model->getNumberOfImages(); ++i)
		{
			ImageLayout imageLayout{};
			imageLayout.accessibility = VK_SHADER_STAGE_FRAGMENT_BIT;
			imageLayout.binding = static_cast<uint32_t>(i + 2);
			rendererCreateInfo.imageLayouts.push_back(imageLayout);
		}

		if(i == 0) // left
			rendererCreateInfo.viewportScale = { 0.5f, 1.0f };
		else
		{
			rendererCreateInfo.viewportScale = { 0.5f, 1.0f };
			rendererCreateInfo.viewportOffset = { 0.5f, 0.0f };
		}
			
		m_rendererIDs[i] = m_scene->addRenderer(rendererCreateInfo);

		Scene::AddModelInfo addModelInfo{};
		addModelInfo.renderPassID = m_renderPassID;
		addModelInfo.rendererID = m_rendererIDs[i];
		addModelInfo.model = model;

		// UBO
		m_ubosMVP[i] = wolfInstance->createUniformBufferObject();
		glm::mat4 mvp = glm::mat4(1.0f);
		m_ubosMVP[i]->initializeData(&mvp, sizeof(glm::mat4));
		addModelInfo.ubos.emplace_back(m_ubosMVP[i], mvpLayout);

		// Sampler
		addModelInfo.samplers.emplace_back(model->getSampler(), samplerLayout);

		// Images
		std::vector<Image*> images = model->getImages();
		for (size_t i(0); i < images.size(); ++i)
		{
			ImageLayout imageLayout{};
			imageLayout.accessibility = VK_SHADER_STAGE_FRAGMENT_BIT;
			imageLayout.binding = static_cast<uint32_t>(i + 2);

			addModelInfo.images.emplace_back(images[i], imageLayout);
		}

		m_scene->addModel(addModelInfo);
	}

	m_scene->record();
}


void Wolf::Template3D_VR::update()
{
	for(int i(0); i < 2; ++i)
	{
		glm::mat4 glmView = m_wolfInstance->getVRViewMatrices()[i];
		glm::mat4 glmProjection = m_wolfInstance->getVRProjMatrices()[i];

		glm::mat4 mvp = glmProjection * glmView * glm::scale(glm::mat4(1.0f), glm::vec3(0.01f));
		m_ubosMVP[i]->updateData(&mvp);
	}
}
