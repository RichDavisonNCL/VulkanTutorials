#if USE_IMGUI
#pragma once

#include "imgui.h"
#include "../NCLCoreClasses/Win32Window.h"

namespace NCL::Rendering::Vulkan {
    class Gui {
    public:
        void Init(HWND window, VulkanRenderer* m_renderer);
        void StartNewFrame();
        void Render(VkCommandBuffer cmdBuffer);
        bool CallWndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

        void Destroy();
        bool WantCaptureMouse();

    private:
        UINT g_frameIndex = 0;
        HANDLE g_hSwapChainWaitableObject = NULL;
    };
}
#endif // USE_IMGUI