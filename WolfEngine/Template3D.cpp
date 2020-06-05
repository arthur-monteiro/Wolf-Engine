#include "Template3D.h"

#include <utility>

Wolf::Template3D::Template3D(Wolf::WolfInstance* wolfInstance, Wolf::Scene* scene, std::string modelFilename,
                             std::string mtlFolder, float ratio) : m_wolfInstance(wolfInstance), m_scene(scene)
{
	Model::ModelCreateInfo modelCreateInfo{};
	modelCreateInfo.inputVertexTemplate = InputVertexTemplate::FULL_3D_MATERIAL;
	Model* model = m_wolfInstance->createModel(modelCreateInfo);

	Model::ModelLoadingInfo modelLoadingInfo;
	modelLoadingInfo.filename = std::move(modelFilename);
	modelLoadingInfo.mtlFolder = std::move(mtlFolder);
	model->loadObj(modelLoadingInfo);

	// Renderer
	{
		Scene::CommandBufferCreateInfo commandBufferCreateInfo;
		commandBufferCreateInfo.finalPipelineStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		commandBufferCreateInfo.commandType = Scene::CommandType::GRAPHICS;
		m_gBufferCommandBufferID = scene->addCommandBuffer(commandBufferCreateInfo);
		m_GBuffer = std::make_unique<GBuffer>(wolfInstance, scene, m_gBufferCommandBufferID, wolfInstance->getWindowSize(), VK_SAMPLE_COUNT_1_BIT, model, glm::mat4(1.0f), false);
		
		//m_cascadedShadowMapping = std::make_unique<CascadedShadowMapping>(wolfInstance, scene, model, 0.1f, 32.f, glm::radians(45.0f), ratio);

		// Render Pass Creation
		Scene::RenderPassCreateInfo renderPassCreateInfo{};
		renderPassCreateInfo.commandBufferID = -1; // default command buffer
		renderPassCreateInfo.outputIsSwapChain = true;
		m_renderPassID = m_scene->addRenderPass(renderPassCreateInfo);
		
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
		addModelInfo.renderPassID = m_renderPassID;
		addModelInfo.rendererID = m_rendererID;

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

void Wolf::Template3D::update(glm::mat4 view, glm::vec3 cameraPosition, glm::vec3 cameraOrientation)
{
	m_viewMatrix = view;
	updateMVP();
	//m_cascadedShadowMapping->updateMatrices(glm::vec3(-1.0f, -5.0f, 0.0f), cameraPosition, cameraOrientation, m_modelMatrix);
}

std::vector<int> Wolf::Template3D::getCommandBufferToSubmit()
{
	return { m_gBufferCommandBufferID };
}

std::vector<std::pair<int, int>> Wolf::Template3D::getCommandBufferSynchronisation()
{
	return { { m_gBufferCommandBufferID, -1 } };
}

void Wolf::Template3D::updateMVP()
{
	glm::mat4 mvp = m_projectionMatrix * m_viewMatrix * m_modelMatrix;
	m_GBuffer->updateMVPMatrix(m_modelMatrix, m_viewMatrix, m_projectionMatrix);
	m_uboMVP->updateData(&mvp);
}
