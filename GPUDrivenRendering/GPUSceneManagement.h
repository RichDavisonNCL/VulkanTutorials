/** @file GPUSceneManagement.h
 * GPU-Driven Scene Management evaluation.
 * Compares CPU-instanced, CPU-cull+indirect, and GPU-cull+indirect rendering
 * using WFC-generated modular 2D-grid scenes with split-SSBO architecture.
 */
#pragma once
#include <vector>
#include <cstdint>
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <map>
#include <chrono>
#include <functional>
#include <string>
#include <iostream>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "vulkan/vulkan.hpp"

#include "Camera.h"
#include "GameTimer.h"
#include "Keyboard.h"
#include "KeyboardMouseController.h"
#include "Matrix.h"
#include "Mouse.h"
#include "Vector.h"
#include "Window.h"

#include "VulkanRenderer.h"
#include "VulkanMesh.h"
#include "VulkanTexture.h"
#include "VulkanPipelineBuilder.h"
#include "VulkanUtils.h"
#include "VulkanMemoryManager.h"

class BenchmarkPanel;

#ifdef USE_IMGUI
#include "GuiWrapper.h"
#endif

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

enum class RenderScheme : uint32_t {
	CPU_Instanced    = 1,
	CPU_CullIndirect = 2,
	GPU_CullIndirect = 3,
};

struct CullingDatum {
	float centerX, centerY, centerZ;
	float halfX, halfY, halfZ;
};
static_assert(sizeof(CullingDatum) == 24);

struct RenderDatum {
	float r, g, b, a;
};
static_assert(sizeof(RenderDatum) == 16);

struct ChunkInfo {
	uint32_t gridX, gridY;
	uint32_t instanceOffset;
	uint32_t instanceCount;
	uint32_t cubeCount;
	uint32_t sphereCount;
	float aabbMinX, aabbMinY, aabbMinZ;
	float aabbMaxX, aabbMaxY, aabbMaxZ;
};
static_assert(sizeof(ChunkInfo) == 48);

struct FrameStats {
	double cpuRecordUs;    // CPU command recording (cull + submit), excludes fence wait
	double cpuWaitUs;      // CPU wait on swapchain acquire fence (present overhead)
	double gpuExecUs;      // GPU execution time (timestamp query)
	double frameWallUs;    // end-to-end wall-clock (record + wait), contains present noise
	uint32_t drawCalls;
	uint32_t visibleChunks;
};

struct BenchmarkConfig {
	uint32_t gridSize     = 16;
	uint32_t chunkSize    = 16;
	uint32_t density      = 50;
	RenderScheme scheme   = RenderScheme::GPU_CullIndirect;
	uint32_t seed         = 42;
	uint32_t warmupFrames = 120;
	uint32_t recordFrames = 1200;
	uint32_t updateSize   = 0;
	bool updateBatched    = true;   // batch all region copies into one submit
	std::string outputPath;
	bool headless         = false;
	// Supplementary-experiment override: split the density-derived otherWeight
	// into independent cube/sphere weights. Unset (<0, default) means the
	// normal density switch in GenerateScene() applies, byte-identical to the
	// formal 810-config matrix. When set, bypasses the WFC disk cache (this
	// path is for the small weight-ratio sweep, not the cached formal matrix).
	float cubeWeightOverride   = -1.0f;
	float sphereWeightOverride = -1.0f;
};

class GPUSceneManagement {
public:
	GPUSceneManagement(Window& window, VulkanInitialisation& vkInit);
	~GPUSceneManagement();

	void SetBenchmarkConfig(const BenchmarkConfig& config);
	void RunFrame(float dt);
	void Finish();

	bool IsBenchmarkComplete() const { return m_benchmarkComplete; }
	VulkanRenderer* GetRenderer() { return m_renderer; }
	const BenchmarkConfig& GetBenchmarkConfig() const { return m_benchConfig; }
	void SetScheme(RenderScheme s) { m_benchConfig.scheme = s; }
	void SetBenchmarkEnabled(bool on) { m_benchmarkEnabled = on; }
	void ResetBenchmarkState(RenderScheme scheme, uint32_t warmup, uint32_t record, const std::string& outputPath) {
		m_benchConfig.scheme = scheme;
		m_benchConfig.warmupFrames = warmup;
		m_benchConfig.recordFrames = record;
		m_benchConfig.outputPath = outputPath;
		m_frameStats.clear();
		m_recordFrameIdx = 0;
		m_isRecording = false;
		m_benchmarkComplete = false;
	}
	void SetSceneParams(uint32_t gridSize, uint32_t chunkSize, uint32_t seed, uint32_t density) {
		m_benchConfig.gridSize = gridSize;
		m_benchConfig.seed = seed;
		m_benchConfig.density = density;
		m_benchConfig.chunkSize = chunkSize;
	}

	const std::vector<FrameStats>& GetFrameStats() const { return m_frameStats; }
	const std::vector<uint32_t>& GetTileGrid() const { return m_tileGrid; }
	uint32_t GetGridSize() const { return m_benchConfig.gridSize; }
	bool IsBenchmarkRecording() const { return m_isRecording; }
	double GetLastUpdateUs() const { return m_lastUpdateUs; }
	void RunLocalUpdate(uint32_t count) { RegenerateChunks(count); }

#ifdef USE_IMGUI
	void SetGui(GuiWrapper* gui) { m_gui = gui; }
	void SetBenchPanel(BenchmarkPanel* panel) { m_benchPanel = panel; }
#endif

	void GenerateScene(const std::vector<uint32_t>& tileGrid);
	void UpdateSceneFromTileGrid(const std::vector<uint32_t>& grid);
	void SetRenderPartial(bool partial) { m_renderPartial = partial; }
	void CreateBuffers();
	void CreateDescriptorSets();
	void CreateQueryPool();

protected:
	void Initialise();
	void BuildCamera();

	UniqueVulkanMesh LoadMesh(const std::string& filename);
	void UploadMeshWait(VulkanMesh& m);

	void GenerateScene();
	void CreatePipelines();
	void WriteInstanceData();
	void ComputeChunkAABBs();
	void ReadbackGPUVisibility();
	void RegenerateChunks(uint32_t count);

	void RenderScheme1(float dt);
	void RenderScheme2(float dt);
	void RenderScheme3(float dt);

	void ExtractFrustumPlanes(Vector4 planes[6]) const;
	void UploadCameraUniform();

	void BeginMeasurement();
	void EndMeasurement();
	void WriteCSVSummary();

public:
	void WriteUpdatePilotCSV(const std::vector<double>& samples, uint32_t updateSize,
	                         const std::string& path);
protected:

	VulkanInitialisation m_vkInit;
	VulkanRenderer*      m_renderer      = nullptr;
	VulkanMemoryManager* m_memoryManager = nullptr;
	Window&              m_hostWindow;

	PerspectiveCamera m_camera;
	KeyboardMouseController m_controller;
	VulkanBuffer m_cameraBuffer;
	vk::UniqueDescriptorSet       m_cameraDescriptor;
	vk::UniqueDescriptorSetLayout m_cameraLayout;
	vk::UniqueSampler m_defaultSampler;

	UniqueVulkanMesh m_cubeMesh;
	UniqueVulkanMesh m_sphereMesh;
	uint32_t m_cubeIndexCount;
	uint32_t m_sphereIndexCount;

	std::vector<uint32_t>     m_tileGrid;
	std::vector<CullingDatum> m_cullingData;
	std::vector<RenderDatum>  m_renderData;
	std::vector<ChunkInfo>    m_chunks;
	std::vector<bool>         m_chunkVisible;
	uint32_t m_totalInstances;
	uint32_t m_gridChunks;

	VulkanBuffer m_cullingBuffer;
	VulkanBuffer m_renderBuffer;
	VulkanBuffer m_indirectBuffer;
	VulkanBuffer m_chunkBuffer;

	vk::UniqueQueryPool m_queryPool;
	uint64_t m_gpuTimestampPeriod;
	uint64_t m_cpuTimestampPeriod;

	VulkanPipeline m_graphicsPipeline;
	VulkanPipeline m_computePipeline;
	vk::UniqueDescriptorSetLayout m_sceneLayout;
	vk::UniqueDescriptorSetLayout m_computeLayout;
	vk::UniqueDescriptorSet       m_sceneDescriptor;
	vk::UniqueDescriptorSet       m_computeDescriptor;

	BenchmarkConfig m_benchConfig;
	std::vector<FrameStats> m_frameStats;
	bool m_isRecording;
	bool m_benchmarkComplete;
	uint32_t m_currentFrame;
	uint32_t m_recordFrameIdx = 0;
	uint32_t m_drawCallCount = 0;
	VulkanBuffer m_visibilityStaging;
	LARGE_INTEGER m_qpcFrequency;
	LARGE_INTEGER m_frameStartQpc;   // wall-clock frame start (set in BeginMeasurement)
	double m_cpuRecordAccumUs = 0;   // accumulated CPU recording time, excludes fence wait
	LARGE_INTEGER m_segStartQpc;     // current CPU segment start
	double m_cpuWaitUs = 0;          // fence-wait duration for current frame

	// CPU timing segment helpers (exclude BeginRenderToScreen fence wait)
	void CpuSegBegin();
	void CpuSegEnd();
	void MarkFenceWait(double waitUs);

	class ChunkMonitor* m_monitor = nullptr;
		uint32_t m_monitorGridChunks = 0;
	bool m_altWasHeld = false;
	bool m_pendingReadback = false;
	bool m_benchmarkEnabled = false;
	bool m_renderPartial = false;
	float m_cellSize = 4.0f;
	double m_lastUpdateUs = 0.0;   // pilot: last RegenerateChunks transfer cost
#ifdef USE_IMGUI
	GuiWrapper* m_gui = nullptr;
	BenchmarkPanel* m_benchPanel = nullptr;
#endif

};
