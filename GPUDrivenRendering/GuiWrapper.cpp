/** @file GuiWrapper.cpp
 * ImGui wrapper using docking-branch Vulkan backend API.
 */
#ifdef USE_IMGUI
#include "GuiWrapper.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_vulkan.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

static void check_vk_result(VkResult err) {
	if (err == 0) return;
	fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
	if (err < 0) abort();
}

void GuiWrapper::Init(HWND window, VulkanRenderer* renderer) {
	std::cout << "[GuiWrapper] Init called\n";
	m_renderer = renderer;
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.DisplaySize = ImVec2(
		(float)renderer->GetFrameContext().viewport.width,
		(float)std::abs(renderer->GetFrameContext().viewport.height));
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

	ImGui::StyleColorsDark();

	FrameContext const& ctx = renderer->GetFrameContext();

	ImGui_ImplVulkan_InitInfo info = {};
	info.ApiVersion    = VK_API_VERSION_1_3;
	info.Instance      = renderer->GetVulkanInstance();
	info.PhysicalDevice = renderer->GetPhysicalDevice();
	info.Device        = renderer->GetDevice();
	info.QueueFamily   = ctx.queueFamilies[CommandType::AsyncCompute];
	info.Queue         = ctx.queues[CommandType::AsyncCompute];
	info.DescriptorPoolSize = 0;
	info.DescriptorPool = ctx.descriptorPool;
	info.MinImageCount = 2;
	info.ImageCount    = 2;
	info.UseDynamicRendering = true;
	info.CheckVkResultFn = check_vk_result;

	VkPipelineRenderingCreateInfoKHR pipeRender{};
	pipeRender.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR;
	pipeRender.colorAttachmentCount = 1;
	VkFormat colorFormat = (VkFormat)ctx.colourFormat;
	pipeRender.pColorAttachmentFormats = &colorFormat;
	pipeRender.depthAttachmentFormat   = (VkFormat)ctx.depthFormat;

	info.PipelineInfoMain.PipelineRenderingCreateInfo = pipeRender;
	info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

	ImGui_ImplWin32_Init((void*)window);
	io.BackendFlags &= ~ImGuiBackendFlags_PlatformHasViewports; // single-window only, no multi-viewport
	VkInstance vkInstance = static_cast<VkInstance>(renderer->GetVulkanInstance());
	PFN_vkGetInstanceProcAddr gipa = dynamicLoader.getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
	auto loadCtx = std::make_pair(gipa, vkInstance);
	ImGui_ImplVulkan_LoadFunctions(VK_API_VERSION_1_3, [](const char* name, void* user) -> PFN_vkVoidFunction {
		auto p = (std::pair<PFN_vkGetInstanceProcAddr, VkInstance>*)user;
		return p->first(p->second, name);
	}, &loadCtx);
	ImGui_ImplVulkan_Init(&info);
}

void GuiWrapper::SyncInput(HWND hwnd) {
	ImGuiIO& io = ImGui::GetIO();

	POINT p;
	GetCursorPos(&p);
	ScreenToClient(hwnd, &p);
	io.AddMousePosEvent((float)p.x, (float)p.y);

	const Mouse* mouse = Window::GetMouse();
	static bool prevL, prevR, prevM;
	bool curL = mouse->ButtonDown(NCL::MouseButtons::Left);
	bool curR = mouse->ButtonDown(NCL::MouseButtons::Right);
	bool curM = mouse->ButtonDown(NCL::MouseButtons::Middle);
	if (curL != prevL) io.AddMouseButtonEvent(0, curL);
	if (curR != prevR) io.AddMouseButtonEvent(1, curR);
	if (curM != prevM) io.AddMouseButtonEvent(2, curM);
	prevL = curL; prevR = curR; prevM = curM;

	const Keyboard* kb = Window::GetKeyboard();
	static bool prevCtrl, prevShift, prevAlt;
	bool curCtrl  = kb->KeyDown(KeyCodes::CONTROL);
	bool curShift = kb->KeyDown(KeyCodes::SHIFT);
	bool curAlt   = kb->KeyDown(KeyCodes::MENU);
	if (curCtrl  != prevCtrl)  io.AddKeyEvent(ImGuiMod_Ctrl,  curCtrl);
	if (curShift != prevShift) io.AddKeyEvent(ImGuiMod_Shift, curShift);
	if (curAlt   != prevAlt)   io.AddKeyEvent(ImGuiMod_Alt,   curAlt);
	prevCtrl = curCtrl; prevShift = curShift; prevAlt = curAlt;

}

void GuiWrapper::StartNewFrame() {
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void GuiWrapper::Render(vk::CommandBuffer cmdBuffer) {
	ImGui::Render();
	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
}

void GuiWrapper::Destroy() {
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}
#endif
