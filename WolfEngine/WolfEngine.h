#pragma once

#include <string>

#include "Vulkan.h"
#include "Window.h"
#include "Debug.h"
#include "SwapChain.h"
#include "Scene.h"
#include "Model.h"
#include "Font.h"
#include "Text.h"

namespace Wolf
{	
	struct WolfInstanceCreateInfo
	{
		uint32_t majorVersion;
		uint32_t minorVersion;
		std::string applicationName;

		uint32_t windowWidth = 0;
		uint32_t windowHeight = 0;

		std::function<void(Debug::Severity, std::string)> debugCallback;
	};
	
	class WolfInstance
	{
	public:
		WolfInstance(WolfInstanceCreateInfo createInfo);
		~WolfInstance() = default;

		Scene* createScene(Scene::SceneCreateInfo createInfo);
		Model* createModel(Model::ModelCreateInfo createInfo);
		template<typename T>
		Instance<T>* createInstanceBuffer();
		UniformBufferObject* createUniformBufferObject();
		Texture* createTexture();
		Font* createFont();
		Text* createText();
		
		void frame(Scene* scene);
		bool windowShouldClose();

		void waitIdle();

		// Getters
	public:
		GLFWwindow* getWindowPtr() { return m_window->getWindow(); }

	private:
		static void windowResizeCallback(void* systemManagerInstance, int width, int height)
		{
			//reinterpret_cast<WolfInstance*>(systemManagerInstance)->resize(width, height);
		}

	private:
		std::unique_ptr<Vulkan> m_vulkan;
		std::unique_ptr<Window> m_window;
		std::unique_ptr<SwapChain> m_swapChain;

		CommandPool m_graphicsCommandPool;
		CommandPool m_computeCommandPool;

		std::vector<std::unique_ptr<Scene>> m_scenes;
		std::vector<std::unique_ptr<Model>> m_models;
		std::vector<std::unique_ptr<InstanceParent>> m_instances;
		std::vector<std::unique_ptr<UniformBufferObject>> m_uniformBufferObjects;
		std::vector<std::unique_ptr<Texture>> m_textures;
		std::vector<std::unique_ptr<Font>> m_fonts;
		std::vector<std::unique_ptr<Text>> m_texts;

	private:
		uint32_t MAX_HEIGHT = 2160;
		uint32_t MAX_WIDTH = 3840;
	};

	template <typename T>
	Instance<T>* WolfInstance::createInstanceBuffer()
	{
		Instance<T> *instance = new Instance<T>(m_vulkan->getDevice(), m_vulkan->getPhysicalDevice(), m_graphicsCommandPool.getCommandPool(), m_vulkan->getGraphicsQueue());
		m_instances.push_back(std::unique_ptr<InstanceParent>(instance));
		return instance;
	}
}
