#pragma once
#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"
#include <stdio.h>          // printf, fprintf
#include <stdlib.h>         // abort
#include <iostream>
#include <vulkan/vulkan.h>
#include <windows.h>

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
	}

	int StartupModule();
	
	void IconLoad();

	void IconDestroy();

	void ShutdownModule();

	void UE5Editor();

	float NormColor(float input);

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

	// Our state
	bool show_demo_window = true;
	bool show_another_window = false;
	bool m_Running = false;

	GLFWwindow* window;
	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
	ImVec4 activeTabColor = ImVec4(0.16, 0.17, 0.18, 1);
	ImVec4 normalTabColor = ImVec4(0.07, 0.08, 0.09, 1);
	ImVec2 tabFramePadding = ImVec2(20, 10);
	ImVec4 black = ImVec4(0, 0, 0, 1);

	ImVec4 searchBarFrameColor = ImVec4(NormColor(26), NormColor(27), NormColor(28), 1);
};
