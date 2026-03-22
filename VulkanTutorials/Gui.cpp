#if USE_IMGUI

#include "Gui.h"

#include "imgui_impl_win32.h"
#include "imgui_impl_vulkan.h"

IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

static void check_vk_result(VkResult err) {
    if (err == 0)
        return;

    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);

    if (err < 0)
        abort();
}

void Gui::Init(HWND window, VulkanRenderer* m_renderer) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); 
	(void)io;

	ImGui::StyleColorsDark();

	io.DisplaySize = ImVec2(m_renderer->GetFrameContext().viewport.width, std::abs(m_renderer->GetFrameContext().viewport.height));
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Platform/Renderer backends
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = m_renderer->GetVulkanInstance();
	init_info.PhysicalDevice = m_renderer->GetPhysicalDevice();
	init_info.Device = m_renderer->GetDevice();
	init_info.QueueFamily = m_renderer->GetQueueFamily(CommandType::AsyncCompute);
	init_info.Queue = m_renderer->GetQueue(CommandType::AsyncCompute);
	init_info.DescriptorPool = m_renderer->GetDescriptorPool();;
	init_info.Subpass = 0;
	init_info.MinImageCount = 2;
	init_info.ImageCount = 2;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.CheckVkResultFn = check_vk_result;
	init_info.UseDynamicRendering = true;

	VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
	VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;

	VkPipelineRenderingCreateInfoKHR pipelineRenderingCreateInfo = {};
	pipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
	pipelineRenderingCreateInfo.pNext = nullptr;
	pipelineRenderingCreateInfo.viewMask = 0;
	pipelineRenderingCreateInfo.colorAttachmentCount = 1;
	pipelineRenderingCreateInfo.pColorAttachmentFormats = &colorFormat;
	pipelineRenderingCreateInfo.depthAttachmentFormat = depthFormat;
	pipelineRenderingCreateInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

	init_info.PipelineRenderingCreateInfo = pipelineRenderingCreateInfo;

	ImGui_ImplWin32_Init((void*)window);
	ImGui_ImplVulkan_Init(&init_info);
	ImGui_ImplVulkan_CreateFontsTexture();
	ImGui_ImplVulkan_DestroyFontsTexture();
}

void Gui::StartNewFrame()
{
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void Gui::Render(VkCommandBuffer cmdBuffer) {
	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
}

bool Gui::CallWndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
}

void Gui::Destroy()
{
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

bool Gui::WantCaptureMouse()
{
	ImGuiIO& io = ImGui::GetIO();
	return io.WantCaptureMouse;
}

#endif // USE_IMGUI
