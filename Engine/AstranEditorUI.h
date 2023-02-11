#pragma once
#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include <stdio.h>          // printf, fprintf
#include <stdlib.h>         // abort
#include <vector>
#include <iostream>
#include <vulkan/vulkan.h>
#include <windows.h>
#include <memory>

//#define IMGUI_UNLIMITED_FRAME_RATE
#ifdef _DEBUG
#define IMGUI_VULKAN_DEBUG_REPORT
#endif

static void check_vk_result(VkResult err)
{
	if (err == 0)
		return;
	fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
	if (err < 0)
		abort();
}

#ifdef IMGUI_VULKAN_DEBUG_REPORT
static VKAPI_ATTR VkBool32 VKAPI_CALL debug_report(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType, uint64_t object, size_t location, int32_t messageCode, const char* pLayerPrefix, const char* pMessage, void* pUserData)
{
	(void)flags; (void)object; (void)location; (void)messageCode; (void)pUserData; (void)pLayerPrefix; // Unused arguments
	fprintf(stderr, "[vulkan] Debug report from ObjectType: %i\nMessage: %s\n\n", objectType, pMessage);
	return VK_FALSE;
}
#endif // IMGUI_VULKAN_DEBUG_REPORT

class Texture;
struct GLFWwindow;

/*
class Layer
{
public:
	Layer::Layer(const std::string& debugName)
		: m_DebugName(debugName)
	{
	}

	virtual ~Layer() = default;

	virtual void OnAttach() {}
	virtual void OnDetach() {}
	virtual void OnUpdate(Timestep ts) {}
	virtual void OnImGuiRender() {}
	virtual void OnEvent(Event& event) {}

	const std::string& GetName() const { return m_DebugName; }
protected:
	std::string m_DebugName;
};

class LayerStack
{
public:
	LayerStack() = default;
	~LayerStack();

// 	void PushLayer(Layer* layer);
// 	void PushOverlay(Layer* overlay);
// 	void PopLayer(Layer* layer);
// 	void PopOverlay(Layer* overlay);

	LayerStack::~LayerStack()
	{
		for (Layer* layer : m_Layers)
		{
			layer->OnDetach();
			delete layer;
		}
	}

	void LayerStack::PushLayer(Layer* layer)
	{
		m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);
		m_LayerInsertIndex++;
	}

	void LayerStack::PushOverlay(Layer* overlay)
	{
		m_Layers.emplace_back(overlay);
	}

	void LayerStack::PopLayer(Layer* layer)
	{
		auto it = std::find(m_Layers.begin(), m_Layers.begin() + m_LayerInsertIndex, layer);
		if (it != m_Layers.begin() + m_LayerInsertIndex)
		{
			layer->OnDetach();
			m_Layers.erase(it);
			m_LayerInsertIndex--;
		}
	}

	void LayerStack::PopOverlay(Layer* overlay)
	{
		auto it = std::find(m_Layers.begin() + m_LayerInsertIndex, m_Layers.end(), overlay);
		if (it != m_Layers.end())
		{
			overlay->OnDetach();
			m_Layers.erase(it);
		}
	}

	std::vector<Layer*>::iterator begin() { return m_Layers.begin(); }
	std::vector<Layer*>::iterator end() { return m_Layers.end(); }
	std::vector<Layer*>::reverse_iterator rbegin() { return m_Layers.rbegin(); }
	std::vector<Layer*>::reverse_iterator rend() { return m_Layers.rend(); }

	std::vector<Layer*>::const_iterator begin() const { return m_Layers.begin(); }
	std::vector<Layer*>::const_iterator end()	const { return m_Layers.end(); }
	std::vector<Layer*>::const_reverse_iterator rbegin() const { return m_Layers.rbegin(); }
	std::vector<Layer*>::const_reverse_iterator rend() const { return m_Layers.rend(); }
private:
	std::vector<Layer*> m_Layers;
	unsigned int m_LayerInsertIndex = 0;
};
*/


#define DECLAREBASE(T) using base = T;

class ImguiStack
{
public:
	ImguiStack() = default;
	virtual ~ImguiStack() = default;

	virtual void Begin() = 0;
	virtual void Update() = 0;
	virtual void End() = 0;

	void Initialize()
	{
		//colorToPop = GetContext().ColorStack.Size;
		//styleToPop = GetContext().StyleVarStack.Size;
	}

	void Destruct()
	{
		//ImGui::PopStyleColor(GetContext().ColorStack.Size - colorToPop);
		//ImGui::PopStyleVar(GetContext().StyleVarStack.Size - styleToPop);
	}

	ImGuiStyle& GetStyle()
	{
		return ImGui::GetStyle();
	}

	ImGuiContext& GetContext()
	{
		return *GImGui;
	}

private:
	int styleToPop = 0;
	int colorToPop = 0;
};

class ImguiWindowStack : public ImguiStack
{
	DECLAREBASE(ImguiStack);

public:
	void Begin() override
	{
		const ImGuiViewport* main_viewport = ImGui::GetMainViewport();

		ImGui::SetNextWindowPos(main_viewport->Pos);
		ImGui::SetNextWindowSize(main_viewport->Size);
		ImGui::SetNextWindowDockID(main_viewport->ID);

		ImGuiWindowFlags window_flags = 0;
		{
			window_flags |= ImGuiWindowFlags_NoDocking;
			window_flags |= ImGuiWindowFlags_NoTitleBar;
			window_flags |= ImGuiWindowFlags_NoCollapse;
			window_flags |= ImGuiWindowFlags_NoResize;
			//window_flags |= ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
			//window_flags |= ImGuiWindowFlags_NoNavInputs;
			//window_flags |= ImGuiWindowFlags_NoFocusOnAppearing;
			window_flags |= ImGuiWindowFlags_AlwaysAutoResize;
			//window_flags |= ImGuiWindowFlags_NoBackground;
			//window_flags |= ImGuiWindowFlags_NoSavedSettings;
			//window_flags |= ImGuiWindowFlags_NoMouseInputs;
			//window_flags |= ImGuiWindowFlags_MenuBar;
			//window_flags |= ImGuiWindowFlags_HorizontalScrollbar;
			//window_flags |= ImGuiWindowFlags_AlwaysVerticalScrollbar;
			//window_flags |= ImGuiWindowFlags_AlwaysHorizontalScrollbar;
			//window_flags |= ImGuiWindowFlags_AlwaysUseWindowPadding;
			//window_flags |= ImGuiWindowFlags_NoNavFocus;
			//window_flags |= ImGuiWindowFlags_UnsavedDocument;
		}

		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
		//ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);

		ImGui::Begin("AstranEditor", nullptr, window_flags);

		//window_flags &= ~ImGuiWindowFlags_NoDocking;
		window_flags |= ImGuiWindowFlags_NoBackground;
		//window_flags |= ImGuiWindowFlags_NoFocusOnAppearing;

		ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
		ImGuiID mainDockedSpaceID = ImGui::GetID("MainDockedSpace");

		//Teena - clear color is fake so we can allow main window to be resized without delay
		ImVec4 clear_color = ImVec4(1.0f, 1.0f, 1.0f, 0.00f);
		//ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
		ImVec4 activeTabColor = ImVec4(0.16, 0.17, 0.18, 1);
		ImVec4 normalTabColor = ImVec4(0.07, 0.08, 0.09, 1);
		ImVec2 tabFramePadding = ImVec2(20, 10);
		ImVec4 black = ImVec4(0, 0, 0, 1);

		ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, ImVec4(0, 0, 0, 1));
		ImGui::PushStyleColor(ImGuiCol_TitleBg, normalTabColor);
		ImGui::PushStyleColor(ImGuiCol_TitleBgActive, normalTabColor);
		ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, normalTabColor);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, tabFramePadding); //Tab EXPANDING
		ImGui::DockSpace(mainDockedSpaceID, ImVec2(0, 0));
	}


	void Update() override
	{
		/*
// 		ImGui::Text("io.WantCaptureMouse: %d", io.WantCaptureMouse);
// 		ImGui::Text("io.WantCaptureKeyboard: %d", io.WantCaptureKeyboard);
// 		ImGui::Text("io.WantTextInput: %d", io.WantTextInput);
// 		ImGui::Text("io.WantSetMousePos: %d", io.WantSetMousePos);
// 		//ImGui::Text("curWind->MenuBarHeight(): %d", curWind->MenuBarHeight());
// 		ImGui::Text("io.NavActive: %d, io.NavVisible: %d", io.NavActive, io.NavVisible);
// 		*/
	}

	void End() override
	{
		ImGui::End();
	}
};


class ImguiMainWindowStack : public ImguiStack
{
	DECLAREBASE(ImguiStack);

public:
	void Begin() override
	{
		const ImGuiViewport* main_viewport = ImGui::GetMainViewport();

		ImGui::SetNextWindowPos(main_viewport->Pos);
		ImGui::SetNextWindowSize(main_viewport->Size);
		ImGui::SetNextWindowDockID(main_viewport->ID);

		ImGuiWindowFlags window_flags = 0;
		{
			window_flags |= ImGuiWindowFlags_NoDocking;
			window_flags |= ImGuiWindowFlags_NoTitleBar;
			window_flags |= ImGuiWindowFlags_NoCollapse;
			window_flags |= ImGuiWindowFlags_NoResize;
			//window_flags |= ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
			//window_flags |= ImGuiWindowFlags_NoNavInputs;
			//window_flags |= ImGuiWindowFlags_NoFocusOnAppearing;
			window_flags |= ImGuiWindowFlags_AlwaysAutoResize;
			//window_flags |= ImGuiWindowFlags_NoBackground;
			//window_flags |= ImGuiWindowFlags_NoSavedSettings;
			//window_flags |= ImGuiWindowFlags_NoMouseInputs;
			//window_flags |= ImGuiWindowFlags_MenuBar;
			//window_flags |= ImGuiWindowFlags_HorizontalScrollbar;
			//window_flags |= ImGuiWindowFlags_AlwaysVerticalScrollbar;
			//window_flags |= ImGuiWindowFlags_AlwaysHorizontalScrollbar;
			//window_flags |= ImGuiWindowFlags_AlwaysUseWindowPadding;
			//window_flags |= ImGuiWindowFlags_NoNavFocus;
			//window_flags |= ImGuiWindowFlags_UnsavedDocument;
		}

		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
		//ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);

		ImGui::Begin("AstranEditor", nullptr, window_flags);

		ImGui::PopStyleVar(4);

		//window_flags &= ~ImGuiWindowFlags_NoDocking;
		window_flags |= ImGuiWindowFlags_NoBackground;
		//window_flags |= ImGuiWindowFlags_NoFocusOnAppearing;

		ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
		ImGuiID mainDockedSpaceID = ImGui::GetID("MainDockedSpace");

		//Teena - clear color is fake so we can allow main window to be resized without delay
		ImVec4 clear_color = ImVec4(1.0f, 1.0f, 1.0f, 0.00f);
		//ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
		ImVec4 activeTabColor = ImVec4(0.16, 0.17, 0.18, 1);
		ImVec4 normalTabColor = ImVec4(0.07, 0.08, 0.09, 1);
		ImVec2 tabFramePadding = ImVec2(20, 10);
		ImVec4 black = ImVec4(0, 0, 0, 1);

		ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, ImVec4(0, 0, 0, 1));
		ImGui::PushStyleColor(ImGuiCol_TitleBg, normalTabColor);
		ImGui::PushStyleColor(ImGuiCol_TitleBgActive, normalTabColor);
		ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, normalTabColor);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, tabFramePadding); //Tab EXPANDING
		ImGui::DockSpace(mainDockedSpaceID, ImVec2(0, 0));


	}


	void Update() override
	{
		/*
// 		ImGui::Text("io.WantCaptureMouse: %d", io.WantCaptureMouse);
// 		ImGui::Text("io.WantCaptureKeyboard: %d", io.WantCaptureKeyboard);
// 		ImGui::Text("io.WantTextInput: %d", io.WantTextInput);
// 		ImGui::Text("io.WantSetMousePos: %d", io.WantSetMousePos);
// 		//ImGui::Text("curWind->MenuBarHeight(): %d", curWind->MenuBarHeight());
// 		ImGui::Text("io.NavActive: %d, io.NavVisible: %d", io.NavActive, io.NavVisible);
// 		*/
	}

	void End() override
	{
		ImGui::End();
	}
};


class ImguiInspectorPanel : public ImguiStack
{
	DECLAREBASE(ImguiStack);

public:
	void Begin() override
	{
		const ImGuiViewport* main_viewport = ImGui::GetMainViewport();

		ImGui::SetNextWindowPos(main_viewport->Pos);
		ImGui::SetNextWindowSize(main_viewport->Size);
		ImGui::SetNextWindowDockID(main_viewport->ID);

		ImGuiWindowFlags window_flags = 0;
		{
			window_flags |= ImGuiWindowFlags_NoDocking;
			window_flags |= ImGuiWindowFlags_NoTitleBar;
			window_flags |= ImGuiWindowFlags_NoCollapse;
			window_flags |= ImGuiWindowFlags_NoResize;
			//window_flags |= ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
			//window_flags |= ImGuiWindowFlags_NoNavInputs;
			//window_flags |= ImGuiWindowFlags_NoFocusOnAppearing;
			window_flags |= ImGuiWindowFlags_AlwaysAutoResize;
			//window_flags |= ImGuiWindowFlags_NoBackground;
			//window_flags |= ImGuiWindowFlags_NoSavedSettings;
			//window_flags |= ImGuiWindowFlags_NoMouseInputs;
			//window_flags |= ImGuiWindowFlags_MenuBar;
			//window_flags |= ImGuiWindowFlags_HorizontalScrollbar;
			//window_flags |= ImGuiWindowFlags_AlwaysVerticalScrollbar;
			//window_flags |= ImGuiWindowFlags_AlwaysHorizontalScrollbar;
			//window_flags |= ImGuiWindowFlags_AlwaysUseWindowPadding;
			//window_flags |= ImGuiWindowFlags_NoNavFocus;
			//window_flags |= ImGuiWindowFlags_UnsavedDocument;
		}

		/*
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 10);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 10);
		*/

		ImGui::Begin("AstranEditor", nullptr, window_flags);
	}


	void Update() override
	{

	}

	void End() override
	{
		ImGui::End();
	}
};

class AstranEditorUI
{
	ImFont* DroidSans;
	ImFont* RobotoMedium;
	Texture* saveButton;
	Texture* appIcon;
	Texture* GameObjectOn;
	Texture* TransformIcon;
	Texture* ThreeDotButtonIcon;

public:
	AstranEditorUI()
	{
	}
	~AstranEditorUI()
	{
		for (auto t : m_ImguiStacks)
		{
			delete(t);
		}

		m_ImguiStacks.clear();
	}

	int StartupModule();
	
	void IconLoad();

	void IconDestroy();

	void ShutdownModule();

	void UE5Editor();

	float NormColor(float input);

	void WindowBarButton();

	void Editor();

	void EditorStyle();

	void JayanamMeshEditor();


	void Sample();

	void ImGuiRender();

	static VkInstance GetInstance();
	
	static VkPhysicalDevice GetPhysicalDevice();
	
	static VkDevice GetDevice();
	
	static VkCommandBuffer GetCommandBuffer(bool begin);
	
	static void FlushCommandBuffer(VkCommandBuffer commandBuffer);

private:
	void StyleColorsDarkUE5();

	std::vector<ImguiStack*> m_ImguiStacks;

	// Our state
	bool show_demo_window = true;
	bool show_another_window = false;
	bool m_Running = false;

	GLFWwindow* window;
	//Teena - clear color is fake so we can allow main window to be resized without delay
	ImVec4 clear_color = ImVec4(1.0f, 1.0f, 1.0f, 0.00f);
	//ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	ImVec4 activeTabColor = ImVec4(0.16, 0.17, 0.18, 1);
	ImVec4 normalTabColor = ImVec4(0.07, 0.08, 0.09, 1);
	ImVec2 tabFramePadding = ImVec2(20, 10);
	ImVec4 black = ImVec4(0, 0, 0, 1);

	ImVec4 searchBarFrameColor = ImVec4(NormColor(26), NormColor(27), NormColor(28), 1);
};
