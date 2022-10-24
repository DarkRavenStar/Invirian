#include "AstranEditorUI.h"
#include <vulkan/vulkan.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_vulkan.h"

#include "Renderer/Texture.h"
#include "AstranWidgetUI.h"
#include <filesystem>

static VkAllocationCallbacks* g_Allocator = NULL;
static VkInstance               g_Instance = VK_NULL_HANDLE;
static VkPhysicalDevice         g_PhysicalDevice = VK_NULL_HANDLE;
static VkDevice                 g_Device = VK_NULL_HANDLE;
static uint32_t                 g_QueueFamily = (uint32_t)-1;
static VkQueue                  g_Queue = VK_NULL_HANDLE;
static VkDebugReportCallbackEXT g_DebugReport = VK_NULL_HANDLE;
static VkPipelineCache          g_PipelineCache = VK_NULL_HANDLE;
static VkDescriptorPool         g_DescriptorPool = VK_NULL_HANDLE;

static ImGui_ImplVulkanH_Window g_MainWindowData;
static int                      g_MinImageCount = 2;
static bool                     g_SwapChainRebuild = false;

static const int RESOLUTION_X = 800;
static const int RESOLUTION_Y = 600;
static bool ShowConsoleWindow;


static void SetupVulkan(const char** extensions, uint32_t extensions_count)
{
	VkResult err;

	// Create Vulkan Instance
	{
		VkInstanceCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		create_info.enabledExtensionCount = extensions_count;
		create_info.ppEnabledExtensionNames = extensions;
#ifdef IMGUI_VULKAN_DEBUG_REPORT
		// Enabling validation layers
		const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
		create_info.enabledLayerCount = 1;
		create_info.ppEnabledLayerNames = layers;

		// Enable debug report extension (we need additional storage, so we duplicate the user array to add our new extension to it)
		const char** extensions_ext = (const char**)malloc(sizeof(const char*) * (extensions_count + 1));
		memcpy(extensions_ext, extensions, extensions_count * sizeof(const char*));
		extensions_ext[extensions_count] = "VK_EXT_debug_report";
		create_info.enabledExtensionCount = extensions_count + 1;
		create_info.ppEnabledExtensionNames = extensions_ext;

		// Create Vulkan Instance
		err = vkCreateInstance(&create_info, g_Allocator, &g_Instance);
		check_vk_result(err);
		free(extensions_ext);

		// Get the function pointer (required for any extensions)
		auto vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(g_Instance, "vkCreateDebugReportCallbackEXT");
		IM_ASSERT(vkCreateDebugReportCallbackEXT != NULL);

		// Setup the debug report callback
		VkDebugReportCallbackCreateInfoEXT debug_report_ci = {};
		debug_report_ci.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
		debug_report_ci.pfnCallback = debug_report;
		debug_report_ci.pUserData = NULL;
		err = vkCreateDebugReportCallbackEXT(g_Instance, &debug_report_ci, g_Allocator, &g_DebugReport);
		check_vk_result(err);
#else
		// Create Vulkan Instance without any debug feature
		err = vkCreateInstance(&create_info, g_Allocator, &g_Instance);
		check_vk_result(err);
		IM_UNUSED(g_DebugReport);
#endif
	}

	// Select GPU
	{
		uint32_t gpu_count;
		err = vkEnumeratePhysicalDevices(g_Instance, &gpu_count, NULL);
		check_vk_result(err);
		IM_ASSERT(gpu_count > 0);

		VkPhysicalDevice* gpus = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * gpu_count);
		err = vkEnumeratePhysicalDevices(g_Instance, &gpu_count, gpus);
		check_vk_result(err);

		// If a number >1 of GPUs got reported, find discrete GPU if present, or use first one available. This covers
		// most common cases (multi-gpu/integrated+dedicated graphics). Handling more complicated setups (multiple
		// dedicated GPUs) is out of scope of this sample.
		int use_gpu = 0;
		for (int i = 0; i < (int)gpu_count; i++)
		{
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(gpus[i], &properties);
			if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				use_gpu = i;
				break;
			}
		}

		g_PhysicalDevice = gpus[use_gpu];
		free(gpus);
	}

	// Select graphics queue family
	{
		uint32_t count;
		vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &count, NULL);
		VkQueueFamilyProperties* queues = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * count);
		vkGetPhysicalDeviceQueueFamilyProperties(g_PhysicalDevice, &count, queues);
		for (uint32_t i = 0; i < count; i++)
			if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				g_QueueFamily = i;
				break;
			}
		free(queues);
		IM_ASSERT(g_QueueFamily != (uint32_t)-1);
	}

	// Create Logical Device (with 1 queue)
	{
		int device_extension_count = 1;
		const char* device_extensions[] = { "VK_KHR_swapchain" };
		const float queue_priority[] = { 1.0f };
		VkDeviceQueueCreateInfo queue_info[1] = {};
		queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_info[0].queueFamilyIndex = g_QueueFamily;
		queue_info[0].queueCount = 1;
		queue_info[0].pQueuePriorities = queue_priority;
		VkDeviceCreateInfo create_info = {};
		create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		create_info.queueCreateInfoCount = sizeof(queue_info) / sizeof(queue_info[0]);
		create_info.pQueueCreateInfos = queue_info;
		create_info.enabledExtensionCount = device_extension_count;
		create_info.ppEnabledExtensionNames = device_extensions;
		err = vkCreateDevice(g_PhysicalDevice, &create_info, g_Allocator, &g_Device);
		check_vk_result(err);
		vkGetDeviceQueue(g_Device, g_QueueFamily, 0, &g_Queue);
	}

	// Create Descriptor Pool
	{
		VkDescriptorPoolSize pool_sizes[] =
		{
			{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
			{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
		};
		VkDescriptorPoolCreateInfo pool_info = {};
		pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
		pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
		pool_info.pPoolSizes = pool_sizes;
		err = vkCreateDescriptorPool(g_Device, &pool_info, g_Allocator, &g_DescriptorPool);
		check_vk_result(err);
	}
}

// All the ImGui_ImplVulkanH_XXX structures/functions are optional helpers used by the demo.
// Your real engine/app may not use them.
static void SetupVulkanWindow(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height)
{
	wd->Surface = surface;

	// Check for WSI support
	VkBool32 res;
	vkGetPhysicalDeviceSurfaceSupportKHR(g_PhysicalDevice, g_QueueFamily, wd->Surface, &res);
	if (res != VK_TRUE)
	{
		fprintf(stderr, "Error no WSI support on physical device 0\n");
		exit(-1);
	}

	// Select Surface Format
	const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
	const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
	wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(g_PhysicalDevice, wd->Surface, requestSurfaceImageFormat, (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat), requestSurfaceColorSpace);

	// Select Present Mode
#ifdef IMGUI_UNLIMITED_FRAME_RATE
	VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
#else
	VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
#endif
	wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(g_PhysicalDevice, wd->Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));
	//printf("[vulkan] Selected PresentMode = %d\n", wd->PresentMode);

	// Create SwapChain, RenderPass, Framebuffer, etc.
	IM_ASSERT(g_MinImageCount >= 2);
	ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, wd, g_QueueFamily, g_Allocator, width, height, g_MinImageCount);
}

static void CleanupVulkan()
{
	vkDestroyDescriptorPool(g_Device, g_DescriptorPool, g_Allocator);

#ifdef IMGUI_VULKAN_DEBUG_REPORT
	// Remove the debug report callback
	auto vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(g_Instance, "vkDestroyDebugReportCallbackEXT");
	vkDestroyDebugReportCallbackEXT(g_Instance, g_DebugReport, g_Allocator);
#endif // IMGUI_VULKAN_DEBUG_REPORT

	vkDestroyDevice(g_Device, g_Allocator);
	vkDestroyInstance(g_Instance, g_Allocator);
}

static void CleanupVulkanWindow()
{
	ImGui_ImplVulkanH_DestroyWindow(g_Instance, g_Device, &g_MainWindowData, g_Allocator);
}

static void FrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* draw_data)
{
	VkResult err;

	VkSemaphore image_acquired_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].ImageAcquiredSemaphore;
	VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
	err = vkAcquireNextImageKHR(g_Device, wd->Swapchain, UINT64_MAX, image_acquired_semaphore, VK_NULL_HANDLE, &wd->FrameIndex);
	if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
	{
		g_SwapChainRebuild = true;
		return;
	}
	check_vk_result(err);

	ImGui_ImplVulkanH_Frame* fd = &wd->Frames[wd->FrameIndex];
	{
		err = vkWaitForFences(g_Device, 1, &fd->Fence, VK_TRUE, UINT64_MAX);    // wait indefinitely instead of periodically checking
		check_vk_result(err);

		err = vkResetFences(g_Device, 1, &fd->Fence);
		check_vk_result(err);
	}
	{
		err = vkResetCommandPool(g_Device, fd->CommandPool, 0);
		check_vk_result(err);
		VkCommandBufferBeginInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		err = vkBeginCommandBuffer(fd->CommandBuffer, &info);
		check_vk_result(err);
	}
	{
		VkRenderPassBeginInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		info.renderPass = wd->RenderPass;
		info.framebuffer = fd->Framebuffer;
		info.renderArea.extent.width = wd->Width;
		info.renderArea.extent.height = wd->Height;
		info.clearValueCount = 1;
		info.pClearValues = &wd->ClearValue;
		vkCmdBeginRenderPass(fd->CommandBuffer, &info, VK_SUBPASS_CONTENTS_INLINE);
	}

	// Record dear imgui primitives into command buffer
	ImGui_ImplVulkan_RenderDrawData(draw_data, fd->CommandBuffer);

	// Submit command buffer
	vkCmdEndRenderPass(fd->CommandBuffer);
	{
		VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		VkSubmitInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		info.waitSemaphoreCount = 1;
		info.pWaitSemaphores = &image_acquired_semaphore;
		info.pWaitDstStageMask = &wait_stage;
		info.commandBufferCount = 1;
		info.pCommandBuffers = &fd->CommandBuffer;
		info.signalSemaphoreCount = 1;
		info.pSignalSemaphores = &render_complete_semaphore;

		err = vkEndCommandBuffer(fd->CommandBuffer);
		check_vk_result(err);
		err = vkQueueSubmit(g_Queue, 1, &info, fd->Fence);
		check_vk_result(err);
	}
}

static void FramePresent(ImGui_ImplVulkanH_Window* wd)
{
	if (g_SwapChainRebuild)
		return;
	VkSemaphore render_complete_semaphore = wd->FrameSemaphores[wd->SemaphoreIndex].RenderCompleteSemaphore;
	VkPresentInfoKHR info = {};
	info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	info.waitSemaphoreCount = 1;
	info.pWaitSemaphores = &render_complete_semaphore;
	info.swapchainCount = 1;
	info.pSwapchains = &wd->Swapchain;
	info.pImageIndices = &wd->FrameIndex;
	VkResult err = vkQueuePresentKHR(g_Queue, &info);
	if (err == VK_ERROR_OUT_OF_DATE_KHR || err == VK_SUBOPTIMAL_KHR)
	{
		g_SwapChainRebuild = true;
		return;
	}
	check_vk_result(err);
	wd->SemaphoreIndex = (wd->SemaphoreIndex + 1) % wd->ImageCount; // Now we can use the next set of semaphores
}

static void glfw_error_callback(int error, const char* description)
{
	fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

static void WindowKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
	{
		ShowConsoleWindow = !ShowConsoleWindow;
	}
}

static void MouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
}

class ImGuiCleanUp
{
public:
	bool opened = false;
	bool alwaysCallDestructor = false;
	bool log = false;
	//void(*Constructor)();
	//typedef void(*Destructor)(void) destructor;
	//const char* name;

	using Destructor = void(*)();

	Destructor destructor;

	ImGuiCleanUp(bool inOpened, Destructor inDestructor, bool InAlwaysCallDestructor = false)
	{
		destructor = inDestructor;
		opened = inOpened;
		alwaysCallDestructor = InAlwaysCallDestructor;
	}

	/*
	ImGuiCleanUp(bool inOpened, Destructor inDestructor, bool inAlwaysCallDestructor, bool inLog, const char* inName)
	{
		destructor = inDestructor;
		opened = inOpened;
		alwaysCallDestructor = inAlwaysCallDestructor;
		name = inName;
		log = inLog;
	}
	*/

	~ImGuiCleanUp() noexcept(false)
	{
		if (alwaysCallDestructor || opened)
		{
			//std::cout << name << " - alwaysCallDestructor: " << alwaysCallDestructor;
			//if (log) std::cout << " - alwaysCallDestructor: " << alwaysCallDestructor << "\n";
			destructor();
		}
	}

};

int AstranEditorUI::StartupModule()
{
	// Setup window
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		return 1;

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
	glfwWindowBorderlessHint(GLFW_TRUE);

	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const GLFWvidmode* mode = glfwGetVideoMode(monitor);

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(RESOLUTION_X, RESOLUTION_Y, "Application Template using Astran Game Engine by Teena 0118856", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	// Setup Vulkan
	if (!glfwVulkanSupported())
	{
		std::cerr << "GLFW: Vulkan not supported!\n";
		return -1;
	}

	//glfwMaximizeWindow(window);
	//glfwSetWindowSizeCallback(window, onWindowResized);
	//onWindowResized(window, RESOLUTION_X, RESOLUTION_Y);
	glfwSetKeyCallback(window, WindowKeyCallback);
	//glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, 1);

	uint32_t extensions_count = 0;
	const char** extensions = glfwGetRequiredInstanceExtensions(&extensions_count);
	SetupVulkan(extensions, extensions_count);
	// Create Window Surface
	VkSurfaceKHR surface;
	VkResult err = glfwCreateWindowSurface(g_Instance, window, g_Allocator, &surface);
	check_vk_result(err);

	// Create Framebuffers
	int w, h;
	glfwGetFramebufferSize(window, &w, &h);
	ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
	SetupVulkanWindow(wd, surface, w, h);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.IniFilename = nullptr;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
	//io.ConfigWindowsMoveFromTitleBarOnly = true;
	//io.ConfigViewportsNoAutoMerge = true;
	//io.ConfigViewportsNoTaskBarIcon = true;

	//io.ConfigWindowsMoveFromTitleBarOnly = false;

	// Setup Dear ImGui style
	//StyleColorsDarkUE5();


	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForVulkan(window, true);
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = g_Instance;
	init_info.PhysicalDevice = g_PhysicalDevice;
	init_info.Device = g_Device;
	init_info.QueueFamily = g_QueueFamily;
	init_info.Queue = g_Queue;
	init_info.PipelineCache = g_PipelineCache;
	init_info.DescriptorPool = g_DescriptorPool;
	init_info.Subpass = 0;
	init_info.MinImageCount = g_MinImageCount;
	init_info.ImageCount = g_MinImageCount;// wd->ImageCount;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.Allocator = g_Allocator;
	init_info.CheckVkResultFn = check_vk_result;
	ImGui_ImplVulkan_Init(&init_info, wd->RenderPass);

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	RobotoMedium = io.Fonts->AddFontFromFileTTF("../ThirdParty/imgui/misc/fonts/Roboto-Medium.ttf", 20.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);

	ImFontConfig config;
	//config.OversampleH = 2;
	//config.OversampleV = 1;
	//config.GlyphExtraSpacing.x = 5.0f;
	DroidSans = io.Fonts->AddFontFromFileTTF("../ThirdParty/imgui/misc/fonts/DroidSans.ttf", 18.0f, &config);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != NULL);

	// Upload Fonts
	{
		// Use any command queue
		VkCommandPool command_pool = wd->Frames[wd->FrameIndex].CommandPool;
		VkCommandBuffer command_buffer = wd->Frames[wd->FrameIndex].CommandBuffer;

		err = vkResetCommandPool(g_Device, command_pool, 0);
		check_vk_result(err);
		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		err = vkBeginCommandBuffer(command_buffer, &begin_info);
		check_vk_result(err);

		ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

		VkSubmitInfo end_info = {};
		end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		end_info.commandBufferCount = 1;
		end_info.pCommandBuffers = &command_buffer;
		err = vkEndCommandBuffer(command_buffer);
		check_vk_result(err);
		err = vkQueueSubmit(g_Queue, 1, &end_info, VK_NULL_HANDLE);
		check_vk_result(err);

		err = vkDeviceWaitIdle(g_Device);
		check_vk_result(err);
		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}

	std::cout << "Current path is " << std::filesystem::current_path() << '\n';

	IconLoad();

	return 0;
}


void AstranEditorUI::IconLoad()
{
	//saveButton = new Texture("../Content/Editor/Slate/Starship/MainToolbar/save.svg", Texture::TextureSourceType::VECTOR, 4);

	static std::string primaryPath = "";// "../../../../";

	std::string path = "../Contents/Editor/Icons/";
	
	appIcon = new Texture(path + "UE4.png", Texture::TextureSourceType::RASTER, 1, false);
	GameObjectOn = new Texture(path + "GameObject On@64.png", Texture::TextureSourceType::RASTER, 1, false);
	TransformIcon = new Texture(path + "d_Transform@32.png", Texture::TextureSourceType::RASTER, 1, false);
	ThreeDotButtonIcon = new Texture(path + "_Menu@2x.png", Texture::TextureSourceType::RASTER, 1, false);

	/*
	appIcon = new Texture("Contents/Editor/Icons/UE4.png", Texture::TextureSourceType::RASTER, 1, false);
	GameObjectOn = new Texture("Contents/Editor/Icons/GameObject On@64.png", Texture::TextureSourceType::RASTER, 1, false);
	TransformIcon = new Texture("Contents/Editor/Icons/d_Transform@32.png", Texture::TextureSourceType::RASTER, 1, false);
	ThreeDotButtonIcon = new Texture("Contents/Editor/Icons/_Menu@2x.png", Texture::TextureSourceType::RASTER, 1, false);
	*/

	saveButton = appIcon;

	auto testIcon = TransformIcon;

	std::cout << (ImTextureID)testIcon->GetDescriptorSet() << ", " << testIcon->m_height << ", " << testIcon->m_width << "\n";
}

void AstranEditorUI::IconDestroy()
{
	//delete saveButton;
	delete appIcon;
	delete GameObjectOn;
	delete TransformIcon;
	delete ThreeDotButtonIcon;
}

void AstranEditorUI::ShutdownModule()
{
	IconDestroy();

	// Cleanup
	VkResult err = vkDeviceWaitIdle(g_Device);
	check_vk_result(err);
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	CleanupVulkanWindow();
	CleanupVulkan();

	glfwDestroyWindow(window);
	glfwTerminate();
}

void AstranEditorUI::UE5Editor()
{
	/*
	static bool doOnce = false;

	static int x, y, w, h;
	if (!doOnce)
	{
		glfwGetWindowPos(window, &x, &y);
		glfwGetWindowSize(window, &w, &h);

		//const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(ImVec2(x, y));
		ImGui::SetNextWindowSize(ImVec2(w, h));

//             if (x < 600)
//             {
//                 x++;
//                 //glfwSetWindowPos(window, x, y);
//                 main_viewport->Pos = ImVec2(x, y);
//                 //glfwSetWindowPos(window, main_viewport->Pos.x, main_viewport->Pos.y);
//
			doOnce = true;
			glfwSetWindowPos(window, x, y);
			glfwSetWindowSize(window, 0, 0);
		}
		else
		{
			glfwSetWindowPos(window, x, y);
			//glfwSetWindowSize(window, w, h);
		}
		//glfwHideWindow(window);
		*/

	const ImGuiViewport* main_viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(main_viewport->WorkPos);
	ImGui::SetNextWindowSize(main_viewport->WorkSize);


	// Examples Apps (accessible from the "Examples" menu)
	static bool show_app_main_menu_bar = false;
	static bool show_app_dockspace = false;
	static bool show_app_documents = false;

	static bool show_app_console = false;
	static bool show_app_log = false;
	static bool show_app_layout = false;
	static bool show_app_property_editor = false;
	static bool show_app_long_text = false;
	static bool show_app_auto_resize = false;
	static bool show_app_constrained_resize = false;
	static bool show_app_simple_overlay = false;
	static bool show_app_fullscreen = false;
	static bool show_app_window_titles = false;
	static bool show_app_custom_rendering = false;

	ImGuiWindowFlags window_flags = 0;
	//window_flags |= ImGuiWindowFlags_None;
	window_flags |= ImGuiWindowFlags_NoTitleBar;
	window_flags |= ImGuiWindowFlags_NoResize;
	//window_flags |= ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoScrollbar;
	window_flags |= ImGuiWindowFlags_NoScrollWithMouse;
	window_flags |= ImGuiWindowFlags_NoCollapse;
	//window_flags |= ImGuiWindowFlags_AlwaysAutoResize;
	//window_flags |= ImGuiWindowFlags_NoBackground;
	//window_flags |= ImGuiWindowFlags_NoSavedSettings;
	//window_flags |= ImGuiWindowFlags_NoMouseInputs;
	window_flags |= ImGuiWindowFlags_MenuBar;
	//window_flags |= ImGuiWindowFlags_HorizontalScrollbar;
	window_flags |= ImGuiWindowFlags_NoFocusOnAppearing;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
	//window_flags |= ImGuiWindowFlags_AlwaysVerticalScrollbar;
	//window_flags |= ImGuiWindowFlags_AlwaysHorizontalScrollbar;
	window_flags |= ImGuiWindowFlags_AlwaysUseWindowPadding;
	//window_flags |= ImGuiWindowFlags_NoNavInputs;
	//window_flags |= ImGuiWindowFlags_NoNavFocus;
	//window_flags |= ImGuiWindowFlags_UnsavedDocument;
	window_flags |= ImGuiWindowFlags_NoDocking;
	bool* p_open = NULL; // Don't pass our bool* to Begin

	//ImGuiWindowSettings* settings = ImGui::FindOrCreateWindowSettings("Dear ImGui Demo");
	//settings->

	// Main body of the Demo window starts here.
	if (ImGui::Begin("Dear ImGui Demo", p_open, window_flags))
	{
		/*
		x = ImGui::GetWindowPos().x;
		y = ImGui::GetWindowPos().y;
		w = ImGui::GetWindowSize().x;
		h = ImGui::GetWindowSize().y;
		//std::cout << "ImGui::GetWindowPos().x:" << ImGui::GetWindowPos().x << "ImGui::GetWindowPos().y:" << ImGui::GetWindowPos().y;
		//std::cout << "ImGui::GetWindowSize().x:" <<  << "ImGui::GetWindowSize().y:" << ImGui::GetWindowSize().y;

		//ImGui::Image()
		*/

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Examples"))
			{
				ImGui::MenuItem("Main menu bar", NULL, &show_app_main_menu_bar);
				ImGui::MenuItem("Console", NULL, &show_app_console);
				ImGui::MenuItem("Log", NULL, &show_app_log);
				ImGui::MenuItem("Simple layout", NULL, &show_app_layout);
				ImGui::MenuItem("Property editor", NULL, &show_app_property_editor);
				ImGui::MenuItem("Long text display", NULL, &show_app_long_text);
				ImGui::MenuItem("Auto-resizing window", NULL, &show_app_auto_resize);
				ImGui::MenuItem("Constrained-resizing window", NULL, &show_app_constrained_resize);
				ImGui::MenuItem("Simple overlay", NULL, &show_app_simple_overlay);
				ImGui::MenuItem("Fullscreen window", NULL, &show_app_fullscreen);
				ImGui::MenuItem("Manipulating window titles", NULL, &show_app_window_titles);
				ImGui::MenuItem("Custom rendering", NULL, &show_app_custom_rendering);
				ImGui::MenuItem("Dockspace", NULL, &show_app_dockspace);
				ImGui::MenuItem("Documents", NULL, &show_app_documents);
				ImGui::EndMenu();
			}

			//ImGui::CloseButton(49994, main_viewport->Pos - ImVec2(0, 100));

			ImGui::EndMenuBar();
		}

		/*
		ImGuiViewportP* viewport = (ImGuiViewportP*)(void*)ImGui::GetMainViewport();
		ImGuiWindowFlags window_flagsp = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
		float height = ImGui::GetFrameHeight();

		if (ImGui::BeginViewportSideBar("##SecondaryMenuBar", viewport, ImGuiDir_Down, height, window_flagsp)) {
			if (ImGui::BeginMenuBar()) {
				ImGui::Text("Happy secondary menu bar");
				ImGui::EndMenuBar();
			}
			ImGui::End();
		}
		*/



		//GetMainViewport()->WorkPos
		//ImGui::GetWindowContentRegionWidth()

		ImGui::SetNextWindowPos(ImGui::GetCursorScreenPos(), ImGuiCond_FirstUseEver);

		ImGui::SetNextWindowSize(ImVec2(500, ImGui::GetContentRegionAvail().y));
		// Main body of the Demo window starts here.
		if (ImGui::Begin("Dear ImGui DemoP"))// p_openp, window_flags))
		{
			//ImGui::SetNextWindowSize(main_viewport->WorkSize)
			ImGui::Text("Hello");

		}
		ImGui::End();

		//ImGui::ScaleWindowsInViewport()

		//ImGui::SetNextWindowPos(ImGui::GetCursorScreenPos(), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImGui::GetContentRegionAvail());
		// Main body of the Demo window starts here.
		if (ImGui::Begin("Dear ImGui DemollP"))// p_openp, window_flags))
		{
			//ImGui::SetNextWindowSize(main_viewport->WorkSize)
			ImGui::Text("Goddamn");

		}
		ImGui::End();
		/*
		// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
		{
			static float f = 0.0f;
			static int counter = 0;

			ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

			ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
			ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
			ImGui::Checkbox("Another Window", &show_another_window);

			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
			ImGui::ColorEdit4("clear color", (float*)&clear_color); // Edit 3 floats representing a color

			if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

			ImGui::test_fancy_separator();

			ImGui::End();
		}
		*/
		ImGui::End();
	}




}

float AstranEditorUI::NormColor(float input)
{
	return input / 255.0f;
}

void AstranEditorUI::WindowBarButton()
{
	auto curWind = ImGui::GetCurrentWindow();
	ImGuiStyle& style = ImGui::GetStyle();
	ImGuiIO& io = ImGui::GetIO();
	ImGuiContext& g = *GImGui;
	
	glfwSetAllowCustomTitlebarTest(window, 0);

	if (io.WantCaptureMouse)
	if (g.HoveredWindow)
	if (g.HoveredWindow == curWind)
	if (ImGui::GetHoveredID() == 0)
	{
		glfwSetAllowCustomTitlebarTest(window, 1);

		/*
		if (ImGui::IsMouseDoubleClicked(0))
		{
			if (!glfwGetWindowAttrib(window, GLFW_MAXIMIZED))
			{
				glfwMaximizeWindow(window);
			}
			else
			{
				glfwRestoreWindow(window);
			}
		}

		if (ImGui::IsMouseDragging(0))
		{
			if (glfwGetWindowAttrib(window, GLFW_MAXIMIZED))
			{
				glfwRestoreWindow(window);
			}


			double xpos, ypos;
			glfwGetCursorPos(window, &xpos, &ypos);
			glfwSetWindowPos(window, xpos, ypos);

			//ImVec2 pos = g.IO.MousePos - g.ActiveIdClickOffset;
			//glfwSetWindowPos(window, pos.x, pos.y);
		}

		*/
		
		
		/*
		std::cout << (g.HoveredWindow ? g.HoveredWindow->Name : "") << 
			" - ImGui::GetHoveredID(): " << ImGui::GetHoveredID() << 
			" - curWind: " << curWind->ID << " - " << 
			//" - delta: " << delta.x  << ", " << delta.y << " - " <<
			"\n";
		*/
	}
	
	float windowButtonFramePadding = 10;

	if (curWind->ResizeBorderHeld != -1)
	{
		//glfwSetWindowPos(window, curWind->Pos.x, curWind->Pos.y);
		//glfwSetWindowSize(window, curWind->Size.x, curWind->Size.y);
	}


	

	//Minimize, Maximize, Close Button
	{
		ImGuiID minimize_button_id = curWind->GetID("WindowMinimizeButton");
		ImGuiID maximize_button_id = curWind->GetID("WindowMaximizeButton");
		ImGuiID close_button_id = curWind->GetID("WindowCloseButton");

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(windowButtonFramePadding, style.FramePadding.y));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

		ImGui::SameLine();
		if(ImGui::WindowButton(minimize_button_id, ImGui::WindowButtonMode::MINIMIZE))
		{
			if (!glfwGetWindowAttrib(window, GLFW_ICONIFIED))
			{
				glfwIconifyWindow(window);
			}
		}

		ImGui::SameLine();
		if (ImGui::WindowButton(maximize_button_id, ImGui::WindowButtonMode::MAXIMIZE))
		{
			if (!glfwGetWindowAttrib(window, GLFW_MAXIMIZED))
			{
				glfwMaximizeWindow(window);
			}
			else
			{
				glfwRestoreWindow(window);
			}
		}

		ImGui::SameLine();
		if (ImGui::WindowButton(close_button_id, ImGui::WindowButtonMode::EXIT))
		{
			glfwSetWindowShouldClose(window, 1);
		}

		ImGui::PopAllStyleVar();
	}
}

bool setuonce = false;

void AstranEditorUI::Editor()
{
	ImGuiStyle& style = ImGui::GetStyle();
	ImGuiContext& g = *GImGui;
	
	const ImGuiViewport* main_viewport = ImGui::GetMainViewport();

	//std::cout << "main_viewport->Posx: " << main_viewport->Pos.x << " - main_viewport->Posy: " << main_viewport->Pos.y << "\n";

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
		//window_flags |= ImGuiWindowFlags_AlwaysAutoResize;
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

	if (ImGuiCleanUp cp(ImGui::Begin("AstranEditor", nullptr, window_flags
		//& ~ImGuiWindowFlags_NoResize
	), ImGui::End, true); cp.opened)
	{
		ImGui::PopAllStyleVar();

		ImGui::BeginGroup();
		{
			auto curWind = ImGui::GetCurrentWindow();

			auto frameHeight = ImGui::GetFrameHeight();
			auto workRectWidth = curWind->WorkRect.GetWidth();
			auto sixButtonSpace = frameHeight * 6;
			auto windowButtonSpace = frameHeight * 3;

			auto posToUse = curWind->DC.CursorPos;


			//auto iconWidth = frameHeight * 2;
			//auto iconHeight = frameHeight * 2;

			auto iconWidth = appIcon->m_width * 0.25f;
			auto iconHeight = appIcon->m_height * 0.25f;

			//ImGui::ImageButton((void*)(intptr_t)appIcon->m_textureID, ImVec2(iconWidth / 4, iconHeight / 4));
			ImGui::Image((ImTextureID)appIcon->GetDescriptorSet(), ImVec2(iconWidth, iconHeight));

			auto firstMenuPos = posToUse;
			firstMenuPos.x += iconWidth;
			float workingWidth = workRectWidth - firstMenuPos.x;

			//Project Title
			///////////////////////////////////////////////////////
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(40, style.FramePadding.y));

			auto projectName = "Project Name is ver long sometimes so be careful";
			ImVec2 projectNameLabelSize = ImGui::CalcTextSize(projectName, nullptr, true);
			ImVec2 projectNameItemSize = ImGui::CalcItemSize(ImVec2(0, 0), projectNameLabelSize.x + style.FramePadding.x * 2.0f, projectNameLabelSize.y + style.FramePadding.y * 2.0f);

			ImGui::PopAllStyleVar();
			///////////////////////////////////////////////////////

			auto menuWidth = workingWidth - projectNameItemSize.x - sixButtonSpace - 8;//8 - is size 2 border size subtraction ???
			auto menuHeight = curWind->DC.MenuBarOffset.y + curWind->CalcFontSize() + g.Style.FramePadding.y * 2.0f;


			window_flags |= ImGuiWindowFlags_MenuBar;

			//Main Menu
			{
				ImGui::SetNextWindowPos(firstMenuPos);
				//ImGui::SetNextWindowSize(main_viewport->Size);
				//ImGui::SetNextWindowDockID(main_viewport->ID);
				ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(1, 0, 0, 0));
				ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1, 0, 0, 1));

				//ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.x, 10));
				if (ImGuiCleanUp cp(ImGui::BeginChild("MainMenu", ImVec2(menuWidth, menuHeight + style.FramePadding.y), false, window_flags), ImGui::EndChild, true); cp.opened)
				{
					if (ImGuiCleanUp cp(ImGui::BeginMenuBar(), ImGui::EndMenuBar); cp.opened)
					{
						if (ImGuiCleanUp cp(ImGui::BeginMenu("File"), ImGui::EndMenu); cp.opened)
						{
							ImGui::MenuItem("Main menu bar", NULL);
							ImGui::test_fancy_separator();
						}

						if (ImGuiCleanUp cp(ImGui::BeginMenu("Edit"), ImGui::EndMenu); cp.opened)
						{
							ImGui::MenuItem("Main menu bar", NULL);
						}

						if (ImGuiCleanUp cp(ImGui::BeginMenu("View"), ImGui::EndMenu); cp.opened)
						{
							ImGui::MenuItem("Main menu bar", NULL);
						}

						if (ImGuiCleanUp cp(ImGui::BeginMenu("Help"), ImGui::EndMenu); cp.opened)
						{
							ImGui::MenuItem("Main menu bar", NULL);
						}
					}
				}

				ImGui::PopAllStyleVar();
				ImGui::PopAllStyleColorVar();
			}

			ImGui::BeginGroup();
			{
				ImVec4 disableColor = ImVec4(0, 0, 0, 0);

				//Project Name Button
				{
					ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2);
					ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2);
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(40, style.FramePadding.y));
					ImGui::PushStyleColor(ImGuiCol_Button, disableColor);
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, disableColor);
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, disableColor);

					ImGui::SameLine();
					ImGui::Button(projectName);

					ImGui::PopAllStyleColorVar();
					ImGui::PopAllStyleVar();
				}

				float windowButtonFramePadding = 10;

				//Spacing Dummy
				ImGui::SameLine();
				ImGui::Dummy(ImVec2((workRectWidth - curWind->DC.CursorPos.x) - (20 + windowButtonFramePadding * 2) * 3, 0)); //40 = 20 ori + 10*2 padding

				//Minimize, Maximize, Close Button
				WindowBarButton();
			}
			ImGui::EndGroup();

			/*
			firstMenuPos.y += menuHeight;

			//Secondary Menu
			{
				ImGui::SetNextWindowPos(firstMenuPos);
				//ImGui::SetNextWindowSize(main_viewport->Size);
				//ImGui::SetNextWindowDockID(main_viewport->ID);
				ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(1, 1, 0, 1));
				ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(1, 1, 0, 1));

				ImGui::BeginChild("MenuWindow2", ImVec2(workingWidth, menuHeight), false, window_flags);
				{

				}
				ImGui::EndChild();

				ImGui::PopAllStyleColorVar();
			}
			*/
		}
		ImGui::EndGroup();

		ImGuiIO& io = ImGui::GetIO();

		/*
		ImGui::Text("io.WantCaptureMouse: %d", io.WantCaptureMouse);
		ImGui::Text("io.WantCaptureKeyboard: %d", io.WantCaptureKeyboard);
		ImGui::Text("io.WantTextInput: %d", io.WantTextInput);
		ImGui::Text("io.WantSetMousePos: %d", io.WantSetMousePos);
		//ImGui::Text("curWind->MenuBarHeight(): %d", curWind->MenuBarHeight());
		ImGui::Text("io.NavActive: %d, io.NavVisible: %d", io.NavActive, io.NavVisible);
		*/

		//window_flags &= ~ImGuiWindowFlags_NoDocking;
		window_flags |= ImGuiWindowFlags_NoBackground;
		//window_flags |= ImGuiWindowFlags_NoFocusOnAppearing;

		ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;
		ImGuiID mainDockedSpaceID = ImGui::GetID("MainDockedSpace");

		ImGui::PushStyleColor(ImGuiCol_DockingEmptyBg, ImVec4(0, 0, 0, 1));
		ImGui::PushStyleColor(ImGuiCol_TitleBg, normalTabColor);
		ImGui::PushStyleColor(ImGuiCol_TitleBgActive, normalTabColor);
		ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, normalTabColor);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, tabFramePadding); //Tab EXPANDING
		ImGui::DockSpace(mainDockedSpaceID, ImVec2(0, 0));
		ImGui::PopAllStyleVar();
		ImGui::PopAllStyleColorVar();

		static ImGuiID dock_id_right;
		static auto dock_id_left = ImGui::DockBuilderSplitNode(mainDockedSpaceID, ImGuiDir_Left, 0.3f, nullptr, &dock_id_right);


		// we now dock our windows into the docking node we made above
		//ImGui::DockBuilderDockWindow("SecondaryWorld", dock_id_right);
		//ImGui::DockBuilderDockWindow("MainWorld", dock_id_left);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, tabFramePadding); //Tab EXPANDING
		ImGui::SetNextWindowDockID(dock_id_left, ImGuiCond_FirstUseEver);
		ImGui::PopAllStyleVar();

		ImGui::PopAllStyleColorVar();

		ImGui::PushStyleColor(ImGuiCol_FrameBg, activeTabColor);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, activeTabColor);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, tabFramePadding); //Tab EXPANDING
		if (ImGuiCleanUp cp(ImGui::Begin("Inspector"), ImGui::End, true); cp.opened) {}
		ImGui::PopAllStyleVar();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, tabFramePadding); //Tab EXPANDING
		if (ImGuiCleanUp cp(ImGui::Begin("Inspector"), ImGui::End, true); cp.opened)
		{
			ImGui::PopAllStyleVar();
			ImGuiTreeNodeFlags collapseHeaderFlag;

			//GameObject details
			{
				ImGuiTableFlags tableFlags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_NoBordersInBody;

				if (ImGuiCleanUp cp(ImGui::BeginTable("GameObject Table", 3, tableFlags), ImGui::EndTable, true); cp.opened)
				{
					ImGui::TableSetupColumn("Icon", ImGuiTableColumnFlags_WidthFixed);
					ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
					ImGui::TableSetupColumn("Toggle", ImGuiTableColumnFlags_WidthFixed);

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					auto frameHeight = ImGui::GetFrameHeight();
					ImGui::ImageButton((ImTextureID)GameObjectOn->GetDescriptorSet(), ImVec2(frameHeight, frameHeight));

					ImGui::TableNextColumn();
					ImGui::PushItemWidth(ImGui::GetColumnWidth());
					static const int nameLength = 30; //Teena - Better to limit object name to 30 char - UE4 FName does it to 20
					static char name[nameLength] = "Box";
					ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5);
					ImGui::PushStyleColor(ImGuiCol_FrameBg, searchBarFrameColor);
					ImGui::InputText("###GameObjectName", name, nameLength);
					ImGui::PopItemWidth();
					ImGui::PopAllStyleColorVar();
					ImGui::PopAllStyleVar();


					ImGui::TableNextColumn();
					static bool toggleTest = false;
					ImGui::PushStyleColor(ImGuiCol_Button, searchBarFrameColor);
					ImGui::ToggleButtonNoHover("No", &toggleTest, 0.4, 0.6);
					ImGui::PopAllStyleColorVar();
				}

				if (ImGuiCleanUp cp(ImGui::BeginTable("GameObject Properties Table", 2, tableFlags), ImGui::EndTable, true); cp.opened)
				{
					ImGui::TableSetupColumn("PropertyName", ImGuiTableColumnFlags_WidthFixed);
					//ImGui::TableSetupColumn("Spacing", ImGuiTableColumnFlags_WidthStretch);
					ImGui::TableSetupColumn("PropertyValue", ImGuiTableColumnFlags_WidthStretch);

					auto curwind = ImGui::GetCurrentWindow();
					const float w = ImGui::CalcItemWidth();
					const ImVec2 label_size = ImGui::CalcTextSize("X", NULL, true);
					const ImRect frame_bb(curwind->DC.CursorPos, curwind->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
					const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));
					auto t = total_bb.GetSize();
					t.x = 0;

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::ItemSize(t, style.FramePadding.y);
					ImGui::SameLine();
					ImGui::Text("Tag");

					//ImGui::TableNextColumn();
					ImGui::TableNextColumn();
					ImGui::PushItemWidth(ImGui::GetColumnWidth());

					// Using the generic BeginCombo() API, you have full control over how to display the combo contents.
					// (your selection data could be an index, a pointer to the object, an id for the object, a flag intrusively
					// stored in the object itself, etc.)
					const char* items[] = { "AAAA", "BBBB", "CCCC", "DDDD", "EEEE", "FFFF", "GGGG", "HHHH", "IIII", "JJJJ", "KKKK", "LLLLLLL", "MMMM", "OOOOOOO" };
					static int item_current_idx = 0; // Here we store our selection data as an index.
					const char* combo_preview_value = items[item_current_idx];  // Pass in the preview value visible before opening the combo (it could be anything)
					if (ImGui::BeginCombo("combo 1", combo_preview_value))
					{
						for (int n = 0; n < IM_ARRAYSIZE(items); n++)
						{
							const bool is_selected = (item_current_idx == n);
							if (ImGui::Selectable(items[n], is_selected))
								item_current_idx = n;

							// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
							if (is_selected)
								ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}

					ImGui::TableNextRow();
					ImGui::TableNextColumn();
					ImGui::ItemSize(t, style.FramePadding.y);
					ImGui::SameLine();
					ImGui::Text("Layer");

					//ImGui::TableNextColumn();
					ImGui::TableNextColumn();
					ImGui::PushItemWidth(ImGui::GetColumnWidth());

					if (ImGui::BeginCombo("combo 2", combo_preview_value))
					{
						for (int n = 0; n < IM_ARRAYSIZE(items); n++)
						{
							const bool is_selected = (item_current_idx == n);
							if (ImGui::Selectable(items[n], is_selected))
								item_current_idx = n;

							// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
							if (is_selected)
								ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}
				}

			}

			ImGui::Separator();

			if (ImGui::CollapsingHeaderComponentGUI("Transform", 0, (ImTextureID)TransformIcon->GetDescriptorSet()))
			{
				static float test = 0;

				ImGuiTableFlags tableFlags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_NoBordersInBody;

				if (ImGuiCleanUp cp(ImGui::BeginTable("Transform Table", 7, tableFlags), ImGui::EndTable, true); cp.opened)
				{
					ImGui::TableSetupColumn("FieldName", ImGuiTableColumnFlags_WidthFixed, 200);
					ImGui::TableSetupColumn("columnXText", ImGuiTableColumnFlags_WidthFixed);
					ImGui::TableSetupColumn("columnXValue", ImGuiTableColumnFlags_WidthStretch);
					ImGui::TableSetupColumn("columnYText", ImGuiTableColumnFlags_WidthFixed);
					ImGui::TableSetupColumn("columnYValue", ImGuiTableColumnFlags_WidthStretch);
					ImGui::TableSetupColumn("columnZText", ImGuiTableColumnFlags_WidthFixed);
					ImGui::TableSetupColumn("columnZValue", ImGuiTableColumnFlags_WidthStretch);

					for (int i = 0; i < 3; i++)
					{
						auto curwind = ImGui::GetCurrentWindow();

						ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2);
						//ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2);
						ImGui::PushStyleColor(ImGuiCol_FrameBg, searchBarFrameColor);

						const float w = ImGui::CalcItemWidth();
						const ImVec2 label_size = ImGui::CalcTextSize("X", NULL, true);
						const ImRect frame_bb(curwind->DC.CursorPos, curwind->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
						const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));
						auto t = total_bb.GetSize();
						t.x = 0;

						ImGui::TableNextRow();
						ImGui::TableNextColumn();
						//ImGui::TableSetColumnWidth(ImGui::GetColumnIndex(), 60);
						//Teena - Only call this once for now, since the rest have correct text alignment
						ImGui::ItemSize(t, style.FramePadding.y);
						ImGui::SameLine();
						//std::string name = 
						ImGui::Text("Position");

						ImGui::TableNextColumn();
						ImGui::Text("X:");
						ImGui::TableNextColumn();
						ImGui::PushItemWidth(ImGui::GetColumnWidth());
						ImGui::DragFloat("####X", &test, 0.01f, 0, 0, "%.2f");
						ImGui::PopItemWidth();

						ImGui::TableNextColumn();
						ImGui::Text("Y:");
						ImGui::TableNextColumn();
						ImGui::PushItemWidth(ImGui::GetColumnWidth());
						ImGui::DragFloat("###Y", &test, 0.01f, 0, 0, "%.2f");
						ImGui::PopItemWidth();

						ImGui::TableNextColumn();
						ImGui::Text("Z:");
						ImGui::TableNextColumn();
						ImGui::PushItemWidth(ImGui::GetColumnWidth());
						ImGui::DragFloat("###Z", &test, 0.01f, 0, 0, "%.2f");
						ImGui::PopItemWidth();

						ImGui::PopAllStyleColorVar();
						ImGui::PopAllStyleVar();
					}
				}

			}

			ImGuiTableFlags tableFlags = ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_Resizable | ImGuiTableFlags_NoSavedSettings | ImGuiTableFlags_NoBordersInBody;
			static bool rigidBodyHeaderOpened = false;



			//bool hovered, held;
			//bool pressed = ButtonBehavior(interact_bb, id, &hovered, &held, button_flags);

			static ImVec2 framePosBegin;
			static ImVec2 framePosEnd;

			ImRect rect(framePosBegin, framePosEnd);

			//printf_s("x: %f - y: %f", (framePosBegin - framePosEnd).x, (framePosBegin - framePosEnd).y);

			//ImGui::RenderFrame(rect.Min, rect.Max, ImGui::GetColorU32(ImVec4(1, 0, 0, 1)), true, 0);


			framePosBegin = ImGui::GetCurrentWindowRead()->DC.CursorPos;
			//ImGui::TablePushBackgroundChannel();



			if (ImGuiCleanUp cp(ImGui::BeginTable("RigidBodyHeader", 6, tableFlags), ImGui::EndTable, true); cp.opened)
			{
				ImGui::TableSetupColumn("ArrowButton", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Icon", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Text", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("Spacing", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("Toggle", ImGuiTableColumnFlags_WidthFixed);
				ImGui::TableSetupColumn("ThreeDotButton", ImGuiTableColumnFlags_WidthFixed);

				ImGui::TableNextRow();
				ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImVec4(1, 0, 0, 1)));

				ImGui::TableNextColumn();
				ImVec4 transparent = ImVec4(0, 0, 0, 0);
				ImGui::PushStyleColor(ImGuiCol_FrameBg, transparent);
				ImGui::PushStyleColor(ImGuiCol_Button, transparent);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, transparent);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, transparent);
				if (ImGui::ArrowButton("###RigidBodyHeaderArrow", rigidBodyHeaderOpened ? ImGuiDir_Down : ImGuiDir_Right)) { rigidBodyHeaderOpened = !rigidBodyHeaderOpened; }
				ImGui::PopAllStyleColorVar();

				ImGui::TableNextColumn();
				ImGui::Image((ImTextureID)TransformIcon->GetDescriptorSet(), ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight()));

				ImGui::TableNextColumn();
				ImGui::Text("RigidBody");

				ImGui::TableNextColumn();
				ImGui::InvisibleButton("###Check", ImVec2(ImGui::GetColumnWidth(), ImGui::GetFrameHeight()));
				//rect = ImRect(rect.Min, rect.Min + ImGui::GetItemRectSize());
				//ImGui::RenderFrame(rect.Min, rect.Max, ImGui::GetColorU32(ImVec4(1, 0, 0, 1)), true, 0);

				ImGui::TableNextColumn();
				ImGui::ToggleButtonNoHover("rigidbodyEnabled", &rigidBodyHeaderOpened, 0.4, 0.6);

				ImGui::TableNextColumn();
				ImGui::PushStyleColor(ImGuiCol_FrameBg, transparent);
				ImGui::PushStyleColor(ImGuiCol_Button, transparent);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, transparent);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, transparent);
				ImGui::ImageButton((ImTextureID)ThreeDotButtonIcon->GetDescriptorSet(), ImVec2(ImGui::GetFrameHeight(), ImGui::GetFrameHeight()), ImVec2(), ImVec2(1, 1), 0);
				ImGui::PopAllStyleVar();
				ImGui::PopAllStyleColorVar();
				framePosEnd = ImGui::GetCurrentWindowRead()->DC.CursorPos;
				framePosEnd.x += ImGui::GetFrameHeight();
			}



			ImGui::PushStyleColor(ImGuiCol_Separator, black);
			ImGui::Separator();
			ImGui::PopAllStyleColorVar();
		}

		ImGui::SetNextWindowDockID(dock_id_right, ImGuiCond_FirstUseEver);
		ImGui::Begin("Viewport");
		ImGui::End();

		//Teena: Do a final clean up regardless of orders
		ImGui::PopAllStyleVar();
		ImGui::PopAllStyleColorVar();
	}

	ImGuiWindowFlags window_flagsTes = 0;
	{
		window_flagsTes |= ImGuiWindowFlags_NoDocking;
		//window_flagsTes |= ImGuiWindowFlags_NoTitleBar;
		window_flagsTes |= ImGuiWindowFlags_MenuBar;
		window_flagsTes |= ImGuiWindowFlags_AlwaysUseWindowPadding;
	}

	ImGui::Begin("AstranEditorChild", nullptr, window_flagsTes);
	ImGui::Button("HHHHHHH");
	ImGui::Text("Hello");
	ImGui::End();
}

void AstranEditorUI::EditorStyle()
{
	ImGuiStyle* style = &ImGui::GetStyle();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	//IM_COL32(255, 255, 255, 255)

	//Teena - Tab colors
	style->Colors[ImGuiCol_Tab] = normalTabColor;
	style->Colors[ImGuiCol_TabUnfocused] = normalTabColor;
	style->Colors[ImGuiCol_TabActive] = activeTabColor;
	style->Colors[ImGuiCol_TabUnfocusedActive] = activeTabColor;
	style->Colors[ImGuiCol_TabHovered] = activeTabColor;

	//style->Colors[ImGuiCol_Separator] = searchBarFrameColor;

	//Teena - Good ways to check for tabs coloring
	/*
	style->Colors[ImGuiCol_Tab] = ImVec4(1, 0, 0, 1);
	style->Colors[ImGuiCol_TabActive] = ImVec4(0, 1, 0, 1);
	style->Colors[ImGuiCol_TabUnfocused] = ImVec4(0, 0, 1, 1);
	style->Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(1, 1, 0, 1);
	*/
}

void AstranEditorUI::JayanamMeshEditor()
{
	ImGui::SetCurrentFont(RobotoMedium);
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
		window_flags |= ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus;
		//window_flags |= ImGuiWindowFlags_NoNavInputs;
		//window_flags |= ImGuiWindowFlags_NoFocusOnAppearing;
		//window_flags |= ImGuiWindowFlags_AlwaysAutoResize;
		//window_flags |= ImGuiWindowFlags_NoBackground;
		//window_flags |= ImGuiWindowFlags_NoSavedSettings;
		//window_flags |= ImGuiWindowFlags_NoMouseInputs;
		window_flags |= ImGuiWindowFlags_MenuBar;
		//window_flags |= ImGuiWindowFlags_HorizontalScrollbar;
		//window_flags |= ImGuiWindowFlags_AlwaysVerticalScrollbar;
		//window_flags |= ImGuiWindowFlags_AlwaysHorizontalScrollbar;
		//window_flags |= ImGuiWindowFlags_AlwaysUseWindowPadding;
		//window_flags |= ImGuiWindowFlags_NoNavFocus;
		//window_flags |= ImGuiWindowFlags_UnsavedDocument;
	}



	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 18));
	ImGui::Begin("Hello", NULL, window_flags);
	ImGui::PopStyleVar(2);

	//ImGui::PushFont(DroidSans);

	//ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, -2.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 18));

	if (ImGui::BeginMenuBar())
	{
		ImGui::PopStyleVar();

		ImGuiStyle& style = ImGui::GetStyle();
		//ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(80, 80));
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, style.ItemSpacing.y + 10));
		bool File = ImGui::BeginMenu("File");
		ImGui::PopStyleVar();

		if (File)
		{
			ImGui::MenuItem("Main menu bar", NULL);
			ImGui::MenuItem("Console", NULL);
			ImGui::MenuItem("Log", NULL);
			ImGui::MenuItem("Simple layout", NULL);
			ImGui::MenuItem("Property editor", NULL);
			ImGui::MenuItem("Long text display", NULL);
			ImGui::MenuItem("Auto-resizing window", NULL);
			ImGui::MenuItem("Constrained-resizing window", NULL);
			ImGui::MenuItem("Simple overlay", NULL);
			ImGui::MenuItem("Fullscreen window", NULL);
			ImGui::MenuItem("Manipulating window titles", NULL);
			ImGui::MenuItem("Custom rendering", NULL);
			ImGui::MenuItem("Dockspace", NULL);
			ImGui::MenuItem("Documents", NULL);
			ImGui::test_fancy_separator();
			ImGui::EndMenu();

		}
		ImGui::EndMenuBar();
	}


	//ImGui::PopFont();


	ImGuiID dockedSpaceID = ImGui::GetID("HelloDockedSpaceID");

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(30, 10));
	ImGui::DockSpace(dockedSpaceID, ImVec2(0, 0), ImGuiDockNodeFlags_PassthruCentralNode);
	ImGui::PopStyleVar();



	/*
	ImGuiID InformationID = ImGui::GetID("Information");
	//ImGui::BeginChild(dockedSpaceID);
	ImGui::BeginChildEx("Information", InformationID, ImVec2(0,0), true, ImGuiWindowFlags_None);
	ImGui::Text("gODME");
	ImGui::EndChild();
	*/


	/*
	ImGui::Begin("Example ");
	ImGuiStyle& style = ImGui::GetStyle();
	auto FramePadding = style.FramePadding;
	style.FramePadding = ImVec2(100, 123);
	if (ImGui::BeginTabBar("##tabbar", ImGuiTabBarFlags_::ImGuiTabBarFlags_NoTooltip)) {

		if (ImGui::BeginTabItem("   Controls   ")) {
			style.FramePadding = FramePadding;
			ImGui::Button("TestButton", ImVec2(200.0f, 50.0f));
			ImGui::EndTabItem();
		}
		style.FramePadding = ImVec2(100, 123);
		if (ImGui::BeginTabItem("   Settings   ")) {
			style.FramePadding = FramePadding;
			ImGui::Button("TestButton2", ImVec2(200.0f, 50.0f));
			ImGui::EndTabItem();
		}
		style.FramePadding = FramePadding;
		ImGui::EndTabBar();
	}
	ImGui::End();
	*/


	ImGui::SetNextWindowDockID(dockedSpaceID, ImGuiCond_FirstUseEver);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(30, 10));
	ImGui::Begin("Helloh");
	ImGui::PopStyleVar(2);

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
	ImGui::Image((void*)(intptr_t)saveButton->GetDescriptorSet(), ImVec2(saveButton->m_width, saveButton->m_height));
	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.00f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
	ImGui::ImageButton((void*)(intptr_t)saveButton->GetDescriptorSet(), ImVec2(saveButton->m_width, saveButton->m_height));
	ImGui::PopStyleColor(3);
	ImGui::PopStyleVar();
	/*
	ImGuiViewportP* viewport = (ImGuiViewportP*)(void*)ImGui::GetWindowViewport();
	ImGuiWindowFlags window_flagsp = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
	float height = ImGui::GetFrameHeight();

	if (ImGui::BeginViewportSideBar("##SecondaryMenuBar", viewport, ImGuiDir_Up, height, window_flagsp)) {
		if (ImGui::BeginMenuBar()) {
			ImGui::Text("Happy secondary menu bar");
			ImGui::EndMenuBar();
		}
		ImGui::End();
	}
	*/
	ImGui::End();

	ImGui::End();
}

void AstranEditorUI::Sample()
{
	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New"))
			{
				if (ImGui::MenuItem("TT"))
				{
					if (ImGui::MenuItem("GG"))
					{
						//Do something
					}
				}

			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("FileEdit"))
		{
			if (ImGui::MenuItem("New"))
			{
				//Do something
			}
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	ImGuiViewportP* viewport = (ImGuiViewportP*)(void*)ImGui::GetMainViewport();
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_MenuBar;
	float height = ImGui::GetFrameHeight();



	if (ImGui::BeginViewportSideBar("##SecondaryMenuBar", viewport, ImGuiDir_Up, height, window_flags))
	{
		if (ImGui::BeginMenuBar())
		{
			ImGui::Text("Happy secondary menu bar");
			ImGui::EndMenuBar();
		}
		ImGui::End();
	}

	//ImGuiContext& g = *GImGui;
	//g.Style.Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);



	if (ImGui::BeginViewportSideBar("##MainStatusBar", viewport, ImGuiDir_Down, height, window_flags))
	{
		if (ImGui::BeginMenuBar())
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
			ImGui::Text("Happy status bar");
			ImGui::PopStyleColor(1);
			ImGui::EndMenuBar();
		}
		ImGui::End();
	}


	// 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

	// 2. Show a simple window that we create ourselves. We use a Begin/End pair to created a named window.
	{
		static float f = 0.0f;
		static int counter = 0;

		ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

		ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
		ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
		ImGui::Checkbox("Another Window", &show_another_window);

		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
		ImGui::ColorEdit4("clear color", (float*)&clear_color); // Edit 3 floats representing a color

		if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
			counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", counter);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGui::test_fancy_separator();

		ImGui::End();
	}

	// 3. Show another simple window.
	if (show_another_window)
	{
		ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
		ImGui::Text("Hello from another window!");
		if (ImGui::Button("Close Me"))
			show_another_window = false;
		ImGui::End();
	}
}

void AstranEditorUI::ImGuiRender()
{
	m_Running = true;

	ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
	ImGuiIO& io = ImGui::GetIO();

	srand(time(NULL));
	double prevTime = glfwGetTime();

	while (!glfwWindowShouldClose(window) && m_Running)
	{
		// Poll and handle events (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.

		glfwPollEvents();

		if (ShowConsoleWindow == true)
		{
			HWND hWnd = GetConsoleWindow();
			ShowWindow(hWnd, SW_SHOW);
		}
		else if (ShowConsoleWindow == false)
		{
			HWND hWnd = GetConsoleWindow();
			ShowWindow(hWnd, SW_HIDE);
		}

		double currentTime = glfwGetTime();
		double elapsedTime = currentTime - prevTime;

		if (glfwGetKey(window, GLFW_KEY_ESCAPE))
		{
			break;
		}

		//Teena - research bck the vulkan tutorial for optimal resize timing
		// Resize swap chain?
		if (g_SwapChainRebuild)
		{
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);
			if (width > 0 && height > 0)
			{
				ImGui_ImplVulkan_SetMinImageCount(g_MinImageCount);
				ImGui_ImplVulkanH_CreateOrResizeWindow(g_Instance, g_PhysicalDevice, g_Device, &g_MainWindowData, g_QueueFamily, g_Allocator, width, height, g_MinImageCount);
				g_MainWindowData.FrameIndex = 0;
				g_SwapChainRebuild = false;
			}
		}

		// Start the Dear ImGui frame
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		EditorStyle();
		Editor();
		//UE5Editor();

		// Rendering
		ImGui::Render();
		ImDrawData* main_draw_data = ImGui::GetDrawData();
		const bool main_is_minimized = (main_draw_data->DisplaySize.x <= 0.0f || main_draw_data->DisplaySize.y <= 0.0f);
		wd->ClearValue.color.float32[0] = clear_color.x * clear_color.w;
		wd->ClearValue.color.float32[1] = clear_color.y * clear_color.w;
		wd->ClearValue.color.float32[2] = clear_color.z * clear_color.w;
		wd->ClearValue.color.float32[3] = clear_color.w;
		if (!main_is_minimized)
			FrameRender(wd, main_draw_data);

		// Update and Render additional Platform Windows
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

		// Present Main Platform Window
		if (!main_is_minimized)
			FramePresent(wd);

		prevTime = currentTime;
	}
}

void AstranEditorUI::StyleColorsDarkUE5()
{
	ImGuiStyle* style = &ImGui::GetStyle();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style->WindowRounding = 0.0f;
		style->TabRounding = 0.0f;
		//style->FramePadding = ImVec2(30, 10);
		//style.ScaleAllSizes(3.0f);
		//style.AntiAliasedLinesUseTex = true;
		style->Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	//style->ScaleAllSizes(3);
	ImVec4* colors = style->Colors;

	//colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	//colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	colors[ImGuiCol_Text] = ImVec4(0.67, 0.67, 0.67, 1.00f);
	colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);

	//colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
	colors[ImGuiCol_WindowBg] = ImVec4(0.078, 0.078, 0.078, 1.00f);
	colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);

	colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

	colors[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.29f, 0.48f, 0.54f);
	colors[ImGuiCol_FrameBgHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	colors[ImGuiCol_FrameBgActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);

	//colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
	//colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
	//colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	//colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	colors[ImGuiCol_MenuBarBg] = ImVec4(0.078, 0.078, 0.078, 1.00f);

	colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);

	colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);

	//One of these contains the color for hide tab icon
	/*
	colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 0.00f);
	colors[ImGuiCol_SliderGrab] = ImVec4(0.24f, 0.52f, 0.88f, 0.00f);
	colors[ImGuiCol_SliderGrabActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.00f);
	colors[ImGuiCol_Button] = ImVec4(0.26f, 0.59f, 0.98f, 0.00f);
	colors[ImGuiCol_ButtonHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.00f);
	colors[ImGuiCol_ButtonActive] = ImVec4(0.06f, 0.53f, 0.98f, 0.00f);
	*/

	//ImGui original darkside
	colors[ImGuiCol_Header] = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
	colors[ImGuiCol_HeaderHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
	colors[ImGuiCol_HeaderActive] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);

	//Teena - Color related to tabs and possible other headers.
	//colors[ImGuiCol_Header] = ImVec4(0.14, 0.14, 0.14, 1.00f);
	//colors[ImGuiCol_HeaderHovered] = ImVec4(0.14, 0.14, 0.14, 1.00f);
	//colors[ImGuiCol_HeaderActive] = ImVec4(0.14, 0.14, 0.14, 1.00f);


	colors[ImGuiCol_Separator] = colors[ImGuiCol_Border];
	colors[ImGuiCol_SeparatorHovered] = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
	colors[ImGuiCol_SeparatorActive] = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);

	colors[ImGuiCol_ResizeGrip] = ImVec4(0.26f, 0.59f, 0.98f, 0.20f);
	colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	colors[ImGuiCol_ResizeGripActive] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);

	//ImGui original darkside

	colors[ImGuiCol_Tab] = ImLerp(colors[ImGuiCol_Header], colors[ImGuiCol_TitleBgActive], 0.80f);
	colors[ImGuiCol_TabHovered] = colors[ImGuiCol_HeaderHovered];
	colors[ImGuiCol_TabActive] = ImLerp(colors[ImGuiCol_HeaderActive], colors[ImGuiCol_TitleBgActive], 0.60f);
	colors[ImGuiCol_TabUnfocused] = ImLerp(colors[ImGuiCol_Tab], colors[ImGuiCol_TitleBg], 0.80f);
	colors[ImGuiCol_TabUnfocusedActive] = ImLerp(colors[ImGuiCol_TabActive], colors[ImGuiCol_TitleBg], 0.40f);


	//Teena - Color related to tabs. Revisit for better color shading. V1
	/*
	colors[ImGuiCol_Tab] = colors[ImGuiCol_Header];
	colors[ImGuiCol_TabHovered] = colors[ImGuiCol_HeaderHovered];
	colors[ImGuiCol_TabActive] = colors[ImGuiCol_HeaderActive];
	colors[ImGuiCol_TabUnfocused] = colors[ImGuiCol_Tab];
	colors[ImGuiCol_TabUnfocusedActive] = colors[ImGuiCol_TabActive];
	*/

	//Teena - Color related to tabs. Revisit for better color shading. V2
	/*
	colors[ImGuiCol_Tab] = ImVec4(0.14, 0.14, 0.14, 1.00f);
	colors[ImGuiCol_TabHovered] = ImVec4(0.14, 0.14, 1.14, 1.00f);
	colors[ImGuiCol_TabActive] = ImVec4(0.14, 0.14, 0.14, 1.00f);
	colors[ImGuiCol_TabUnfocused] = ImVec4(0.14, 0.14, 0.14, 1.00f);
	colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.14, 0.14, 0.14, 1.00f);
	*/

	colors[ImGuiCol_DockingPreview] = colors[ImGuiCol_HeaderActive] * ImVec4(1.0f, 1.0f, 1.0f, 0.7f);
	colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
	colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
	colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);   // Prefer using Alpha=1.0 here
	colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);   // Prefer using Alpha=1.0 here
	colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
}


VkInstance AstranEditorUI::GetInstance()
{
	return g_Instance;
}

VkPhysicalDevice AstranEditorUI::GetPhysicalDevice()
{
	return g_PhysicalDevice;
}

VkDevice AstranEditorUI::GetDevice()
{
	return g_Device;
}

VkCommandBuffer AstranEditorUI::GetCommandBuffer(bool begin)
{
	ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;

	// Use any command queue
	VkCommandPool command_pool = wd->Frames[wd->FrameIndex].CommandPool;

	VkCommandBufferAllocateInfo cmdBufAllocateInfo = {};
	cmdBufAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufAllocateInfo.commandPool = command_pool;
	cmdBufAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufAllocateInfo.commandBufferCount = 1;

	VkCommandBuffer command_buffer;
	auto err = vkAllocateCommandBuffers(g_Device, &cmdBufAllocateInfo, &command_buffer);

	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	err = vkBeginCommandBuffer(command_buffer, &begin_info);
	check_vk_result(err);

	return command_buffer;
}

void AstranEditorUI::FlushCommandBuffer(VkCommandBuffer commandBuffer)
{
	const uint64_t DEFAULT_FENCE_TIMEOUT = 100000000000;

	VkSubmitInfo end_info = {};
	end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	end_info.commandBufferCount = 1;
	end_info.pCommandBuffers = &commandBuffer;
	auto err = vkEndCommandBuffer(commandBuffer);
	check_vk_result(err);

	// Create fence to ensure that the command buffer has finished executing
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = 0;
	VkFence fence;
	err = vkCreateFence(g_Device, &fenceCreateInfo, nullptr, &fence);
	check_vk_result(err);

	err = vkQueueSubmit(g_Queue, 1, &end_info, fence);
	check_vk_result(err);

	err = vkWaitForFences(g_Device, 1, &fence, VK_TRUE, DEFAULT_FENCE_TIMEOUT);
	check_vk_result(err);

	vkDestroyFence(g_Device, fence, nullptr);
}
