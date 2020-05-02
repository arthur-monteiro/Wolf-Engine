#include "Template3D.h"

#include <utility>

Wolf::Template3D::Template3D(Wolf::WolfInstance* wolfInstance, Wolf::Scene* scene, std::string modelFilename,
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

	// Renderer
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

		m_rendererID = m_scene->addRenderer(rendererCreateInfo);

		Scene::AddModelInfo addModelInfo{};
		addModelInfo.model = model;

		// UBO
		m_uboMVP = wolfInstance->createUniformBufferObject();

		m_projectionMatrix = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 100.0f);
		m_projectionMatrix[1][1] *= -1;
		m_viewMatrix = glm::lookAt(glm::vec3(-2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		m_modelMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(0.01f));
		
		glm::mat4 mvp = m_projectionMatrix * m_viewMatrix * m_modelMatrix;
		m_uboMVP->initializeData(&mvp, sizeof(glm::mat4));
		addModelInfo.ubos.emplace_back(m_uboMVP, mvpLayout);

		// Sampler
		addModelInfo.samplers.emplace_back(model->getSampler(), samplerLayout);

		// Images
		std::vector<Image*> images = model->getImages();
		for(size_t i(0); i < images.size(); ++i)
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

void Wolf::Template3D::updateViewMatrix(glm::mat4 view)
{
	m_viewMatrix = view;
	updateMVP();
}

void Wolf::Template3D::updateMVP()
{
	glm::mat4 mvp = m_projectionMatrix * m_viewMatrix * m_modelMatrix;
	m_uboMVP->updateData(&mvp);
}
