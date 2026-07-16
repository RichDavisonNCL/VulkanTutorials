/** @file Main.cpp
 * Entry point for GPUDrivenRendering.
 * Interactive mode: window + optional ImGui monitor. Press ESC to exit.
 * Benchmark mode: headless, runs warmup + recording frames, outputs CSV.
 */
#include "GPUSceneManagement.h"
#include "BenchmarkPanel.h"

#ifdef USE_IMGUI
#include "GuiWrapper.h"
#include "imgui.h"
#include "Win32Window.h"
#endif

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

int main(int argc, char* argv[]) {
	WindowInitialisation winInit = {
		.width       = 1920,
		.height      = 1080,
		.windowTitle = "GPU-Driven Scene Management",
	};

	VulkanInitialisation vkInit;
	vkInit.depthStencilFormat = vk::Format::eD32SfloatS8Uint;
	vkInit.majorVersion = 1;
	vkInit.minorVersion = 3;
	vkInit.deviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	vkInit.deviceExtensions.push_back("VK_EXT_robustness2");
	vkInit.deviceExtensions.emplace_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
	vkInit.deviceExtensions.emplace_back(VK_EXT_CALIBRATED_TIMESTAMPS_EXTENSION_NAME);
	vkInit.instanceExtensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
	vkInit.instanceExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	vkInit.instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#ifdef WIN32
	vkInit.instanceExtensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif

	static vk::PhysicalDeviceRobustness2FeaturesEXT robustness{ .nullDescriptor = true };
	static vk::PhysicalDeviceSynchronization2Features syncFeatures{ .synchronization2 = true };
	static vk::PhysicalDeviceDynamicRenderingFeaturesKHR dynamicRendering{ .dynamicRendering = true };
	static vk::PhysicalDeviceTimelineSemaphoreFeatures timelineSemaphores{ .timelineSemaphore = true };
	static vk::PhysicalDeviceScalarBlockLayoutFeatures scalarFeatures{ .scalarBlockLayout = true };
	vkInit.features.push_back(&robustness);
	vkInit.features.push_back(&syncFeatures);
	vkInit.features.push_back(&dynamicRendering);
	vkInit.features.push_back(&timelineSemaphores);
	vkInit.features.push_back(&scalarFeatures);
	vkInit.framesInFlight = 1;
	vkInit.useHDRSurface = true;

	BenchmarkConfig benchConfig;
	bool benchmarkMode = false;
	bool lockCursor    = false;

	for (int i = 1; i < argc; ++i) {
		std::string arg(argv[i]);
		for (char& c : arg) c = (char)tolower(c);

		if (arg == "-benchmark")           { benchmarkMode = true; }
		else if (arg == "-gridsize")       { benchConfig.gridSize     = std::stoi(argv[++i]); }
		else if (arg == "-chunksize")      { benchConfig.chunkSize    = std::stoi(argv[++i]); }
		else if (arg == "-density")        { benchConfig.density      = std::stoi(argv[++i]); }
		else if (arg == "-scheme")         { benchConfig.scheme       = static_cast<RenderScheme>(std::stoi(argv[++i])); }
		else if (arg == "-seed")           { benchConfig.seed         = std::stoi(argv[++i]); }
		else if (arg == "-warmupframes")   { benchConfig.warmupFrames = std::stoi(argv[++i]); }
		else if (arg == "-recordframes")   { benchConfig.recordFrames = std::stoi(argv[++i]); }
		else if (arg == "-output")         { benchConfig.outputPath    = argv[++i]; }
		else if (arg == "-updatesize")     { benchConfig.updateSize    = std::stoi(argv[++i]); }
		else if (arg == "-updatebatched")  { benchConfig.updateBatched = std::stoi(argv[++i]) != 0; }
		else if (arg == "-w")              { winInit.width             = std::stoi(argv[++i]); }
		else if (arg == "-h")              { winInit.height            = std::stoi(argv[++i]); }
		else if (arg == "-lockcursor")     { lockCursor = true; }
		else if (arg == "-forceintegratedgpu") { vkInit.idealGPU = vk::PhysicalDeviceType::eIntegratedGpu; }
	}

	if (benchmarkMode) {
		winInit.width  = 256;
		winInit.height = 256;
		// Disable vsync in benchmark mode — FIFO throttles frames to display
		// refresh (~5.5ms wait), masking true pipeline throughput. Mailbox
		// presents without blocking on vertical sync.
		vkInit.idealPresentMode = vk::PresentModeKHR::eMailbox;
	}

	Window* w = Window::CreateGameWindow(winInit);
	if (!w->HasInitialised()) return -1;
	w->LockMouseToWindow(lockCursor);

	GPUSceneManagement app(*w, vkInit);

	// Hide cursor early — SetBenchmarkConfig may take minutes for WFC generation.
	// The Alt-toggle state machine in RunFrame handles show/hide from frame 0 onward.
	while (ShowCursor(FALSE) >= 0);  // force counter to -1

#ifdef USE_IMGUI
	GuiWrapper* gui = nullptr;
	BenchmarkPanel benchPanel;
	if (!benchmarkMode) {
		gui = new GuiWrapper();
		gui->Init(static_cast<Win32Code::Win32Window*>(w)->GetHandle(), app.GetRenderer());
		app.SetGui(gui);
		app.SetBenchPanel(&benchPanel);
	}
#endif

	if (benchmarkMode) {
		app.SetBenchmarkConfig(benchConfig);
		app.SetBenchmarkEnabled(true);  // headless benchmark records unconditionally

		// Local-update pilot: when -UpdateSize N is passed, measure the standalone
		// cost of RegenerateChunks(N) over K repeats and exit — no render loop.
		// Answers "is the update cost measurable above noise, and does it scale?"
		if (benchConfig.updateSize > 0) {
			const int kWarmup = 10, kRepeat = 100;
			for (int i = 0; i < kWarmup; ++i) app.RunLocalUpdate(benchConfig.updateSize);

			std::vector<double> samples;
			samples.reserve(kRepeat);
			for (int i = 0; i < kRepeat; ++i) {
				app.RunLocalUpdate(benchConfig.updateSize);
				samples.push_back(app.GetLastUpdateUs());
			}

			std::sort(samples.begin(), samples.end());
			double sum = 0.0;
			for (double s : samples) sum += s;
			double avg = sum / samples.size();
			double var = 0.0;
			for (double s : samples) var += (s - avg) * (s - avg);
			double stddev = std::sqrt(var / samples.size());
			double p99 = samples[(size_t)(samples.size() * 99 / 100)];

			std::cout << "\n[Pilot] mode=" << (benchConfig.updateBatched ? "batched" : "perchunk")
			          << " updateSize=" << benchConfig.updateSize
			          << " scene=" << benchConfig.gridSize << "^2 chunk=" << benchConfig.chunkSize
			          << " density=" << benchConfig.density << " seed=" << benchConfig.seed
			          << " scheme=" << (int)benchConfig.scheme << "\n";
			std::cout << "[Pilot] cost_us over " << kRepeat << " repeats: "
			          << "avg=" << avg << " min=" << samples.front() << " max=" << samples.back()
			          << " p99=" << p99 << " stddev=" << stddev << "\n";

			app.Finish();
			Window::DestroyGameWindow();
			return 0;
		}

		int totalFrames = benchConfig.warmupFrames + benchConfig.recordFrames;
		int frameIdx = 0;
		while (w->UpdateWindow() && frameIdx < totalFrames && !app.IsBenchmarkComplete()) {
			app.RunFrame(1.0f / 60.0f);
			frameIdx++;
		}
		app.Finish();
	}
	else {
		if (benchConfig.gridSize > 0) {
			benchConfig.headless = false;
			app.SetBenchmarkConfig(benchConfig);
		}

		while (w->UpdateWindow() && !Window::GetKeyboard()->KeyDown(KeyCodes::ESCAPE)) {
			app.RunFrame(w->GetTimer().GetTimeDeltaSeconds());

			if (Window::GetKeyboard()->KeyPressed(KeyCodes::NUM1))
				app.SetScheme(RenderScheme::CPU_Instanced);
			if (Window::GetKeyboard()->KeyPressed(KeyCodes::NUM2))
				app.SetScheme(RenderScheme::CPU_CullIndirect);
			if (Window::GetKeyboard()->KeyPressed(KeyCodes::NUM3))
				app.SetScheme(RenderScheme::GPU_CullIndirect);

		}
		app.Finish();
	}

#ifdef USE_IMGUI
	if (gui) { gui->Destroy(); delete gui; }
#endif
	Window::DestroyGameWindow();
	return 0;
}
