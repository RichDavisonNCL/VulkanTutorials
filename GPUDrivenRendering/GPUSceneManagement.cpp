/** @file GPUSceneManagement.cpp
 * Implements three GPU-driven rendering scheme comparisons.
 * Uses split-SSBO: CullingData (24B) + RenderData (16B) per instance.
 *
 * Scheme 1: CPU traverses visible chunks, per-chunk vkCmdDrawIndexed.
 * Scheme 2: CPU frustum-culls chunks, fills indirect buffer, 2 indirect draws.
 * Scheme 3: Compute shader frustum-culls chunks, fills indirect buffer, 2 indirect draws.
 *
 * Measurement: QPC (CPU) + vkCmdWriteTimestamp2 (GPU), CSV output with per-metric stats.
 * GPU timestamps collected per-frame via large query pool, read back in bulk at end.
 * Two modes: interactive (window + optional ImGui + ChunkMonitor) and headless (-Benchmark).
 */
#include "GPUSceneManagement.h"
#include "FrustumCulling.h"
#include "MshLoader.h"
#include "VulkanComputePipelineBuilder.h"
#include "VulkanDescriptorSetLayoutBuilder.h"
#include "VulkanVMAMemoryManager.h"
#include "WFCGenerator.h"

#include "ChunkMonitor.h"
#include "Win32Window.h"

#include "BenchmarkPanel.h"

using namespace NCL;
using namespace Rendering;
using namespace Vulkan;

GPUSceneManagement::GPUSceneManagement(Window& window, VulkanInitialisation& vkInit)
	: m_hostWindow(window), m_controller(*window.GetKeyboard(), *window.GetMouse())
	, m_vkInit(vkInit), m_totalInstances(0), m_gridChunks(0)
	, m_cubeIndexCount(0), m_sphereIndexCount(0)
	, m_isRecording(false), m_benchmarkComplete(false), m_currentFrame(0)
	, m_recordFrameIdx(0), m_drawCallCount(0) {

	m_vkInit.autoBeginDynamicRendering = false;
	Initialise();
	CreatePipelines();
}

GPUSceneManagement::~GPUSceneManagement() {
	m_renderer->GetDevice().waitIdle();
	// Discard ALL VMA-managed buffers before deleting the allocator.
	// Members are destroyed after the destructor body; VMA would see
	// live allocations and assert if we delete it first.
	if (m_cameraBuffer.buffer)    m_memoryManager->DiscardBuffer(m_cameraBuffer,    DiscardMode::Immediate);
	if (m_cullingBuffer.buffer)   m_memoryManager->DiscardBuffer(m_cullingBuffer,   DiscardMode::Immediate);
	if (m_renderBuffer.buffer)    m_memoryManager->DiscardBuffer(m_renderBuffer,    DiscardMode::Immediate);
	if (m_indirectBuffer.buffer)  m_memoryManager->DiscardBuffer(m_indirectBuffer,  DiscardMode::Immediate);
	if (m_chunkBuffer.buffer)     m_memoryManager->DiscardBuffer(m_chunkBuffer,     DiscardMode::Immediate);
	// Destroy meshes before deleting the memory manager — VulkanMesh::m_gpuBuffer
	// holds a VMA allocation that ~VulkanBuffer frees via m_sourceManager.
	m_cubeMesh.reset();
	m_sphereMesh.reset();
	delete m_memoryManager;
	delete m_renderer;
	if (m_monitor) delete m_monitor;
}

void GPUSceneManagement::Finish() {
	m_renderer->GetDevice().waitIdle();
}

void GPUSceneManagement::Initialise() {
	m_renderer      = new VulkanRenderer(m_hostWindow, m_vkInit);
	m_memoryManager = new VulkanVMAMemoryManager(m_renderer->GetDevice(),
		m_renderer->GetPhysicalDevice(), m_renderer->GetVulkanInstance(), m_vkInit);
	BuildCamera();

	QueryPerformanceFrequency(&m_qpcFrequency);
	m_cpuTimestampPeriod = 1'000'000'000 / m_qpcFrequency.QuadPart;

	FrameContext const& ctx = m_renderer->GetFrameContext();
	m_defaultSampler = ctx.device.createSamplerUnique(vk::SamplerCreateInfo()
		.setMinFilter(vk::Filter::eLinear)
		.setMagFilter(vk::Filter::eLinear));

	m_cameraLayout = DescriptorSetLayoutBuilder(ctx.device)
		.WithUniformBuffers(0, 1, vk::ShaderStageFlagBits::eVertex)
		.Build("CameraMatrices");
	m_cameraDescriptor = CreateDescriptorSet(ctx.device, ctx.descriptorPool, *m_cameraLayout);
	WriteBufferDescriptor(ctx.device, *m_cameraDescriptor, 0, vk::DescriptorType::eUniformBuffer, m_cameraBuffer);

	m_cubeMesh   = LoadMesh("Cube.msh");
	m_sphereMesh = LoadMesh("Sphere.msh");
	m_cubeIndexCount   = m_cubeMesh->GetIndexCount();
	m_sphereIndexCount = m_sphereMesh->GetIndexCount();

	vk::PhysicalDeviceProperties props = m_renderer->GetPhysicalDevice().getProperties();
	m_gpuTimestampPeriod = props.limits.timestampPeriod;

	// Query pool created in CreateQueryPool() once config is known.

	m_camera.SetFieldOfVision(45.0f).SetNearPlane(1.0f).SetFarPlane(12000.0f);
	m_camera.SetPosition(Vector3(128, 120, -200));
	m_camera.SetPitch(-35.0f).SetYaw(180.0f);
	m_camera.SetController(m_controller);

	m_controller.MapAxis(0, "Sidestep");
	m_controller.MapAxis(1, "UpDown");
	m_controller.MapAxis(2, "Forward");
	m_controller.MapAxis(3, "XLook");
	m_controller.MapAxis(4, "YLook");
}

UniqueVulkanMesh GPUSceneManagement::LoadMesh(const string& filename) {
	VulkanMesh* newMesh = new VulkanMesh();
	MshLoader::LoadMesh(filename, *newMesh);
	UploadMeshWait(*newMesh);
	return UniqueVulkanMesh(newMesh);
}

void GPUSceneManagement::UploadMeshWait(VulkanMesh& m) {
	FrameContext const& ctx = m_renderer->GetFrameContext();
	vk::UniqueCommandBuffer cmdBuffer = CmdBufferCreateBegin(ctx.device,
		ctx.commandPools[CommandType::Graphics], "Mesh Upload");
	m.UploadToGPU(*cmdBuffer, m_memoryManager);
	CmdBufferEndSubmitWait(*cmdBuffer, ctx.device, ctx.queues[CommandType::Graphics]);
}

void GPUSceneManagement::BuildCamera() {
	m_cameraBuffer = m_memoryManager->CreateBuffer(
		{ .size = sizeof(Matrix4) * 2,
		  .usage = vk::BufferUsageFlagBits::eUniformBuffer },
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
		"Camera Buffer");
}

void GPUSceneManagement::UploadCameraUniform() {
	Matrix4* cameraMatrices = m_cameraBuffer.Map<Matrix4>();
	cameraMatrices[0] = m_camera.BuildViewMatrix();
	cameraMatrices[1] = m_camera.BuildProjectionMatrix(m_hostWindow.GetScreenAspect());
	m_cameraBuffer.Unmap();
}

void GPUSceneManagement::SetBenchmarkConfig(const BenchmarkConfig& config) {
	m_benchConfig = config;
	std::cout << "[GPUDriven] SetBenchmarkConfig: grid=" << config.gridSize
	          << " chunk=" << config.chunkSize << " density=" << config.density
	          << " scheme=" << (int)config.scheme << " seed=" << config.seed << "\n";

	GenerateScene();
	CreateBuffers();
	CreateDescriptorSets();
	CreateQueryPool();

	float sceneSize = m_benchConfig.gridSize * m_cellSize;
	// Camera just behind near edge, moderate height — independent of scene scale.
	// Z offset must NOT grow proportionally or buildings become sub-pixel at large N.
	float camZ = -(m_cellSize * 10.0f);
	float camY = std::max(120.0f, sceneSize * 0.08f);
	m_camera.SetPosition(Vector3(sceneSize * 0.5f, camY, camZ));
	m_camera.SetPitch(-35.0f).SetYaw(180.0f);

#ifdef USE_IMGUI
	if (m_monitor) { delete m_monitor; m_monitor = nullptr; }
#endif
}

/** Creates timestamp query pool sized for the recording frame count.
 * 2 queries per frame (begin + end), read back in bulk at benchmark end.
 */
void GPUSceneManagement::CreateQueryPool() {
	FrameContext const& ctx = m_renderer->GetFrameContext();
	uint32_t count = 2 * (m_benchConfig.recordFrames + 1);
	m_queryPool = ctx.device.createQueryPoolUnique(
		vk::QueryPoolCreateInfo()
			.setQueryType(vk::QueryType::eTimestamp)
			.setQueryCount(count));
}

/** For scheme 3: reads the indirect buffer instanceCount fields to determine
 *  per-chunk visibility after GPU compute culling completes.
 *  Called after BeginFrame (fence wait ensures previous frame's GPU work is done).
 *  Schemes 1 and 2 set m_chunkVisible directly during CPU culling — no-op here.
 */
void GPUSceneManagement::ReadbackGPUVisibility() {
	if (m_benchConfig.scheme != RenderScheme::GPU_CullIndirect) return;
	if (m_chunks.empty() || m_currentFrame == 0) return;

	uint32_t* indirect = m_indirectBuffer.Map<uint32_t>();
	for (uint32_t i = 0; i < m_chunks.size(); ++i) {
		uint32_t cubeInst   = indirect[i * 10 + 1];   // cube instanceCount
		uint32_t sphereInst = indirect[i * 10 + 6];   // sphere instanceCount
		m_chunkVisible[i] = (cubeInst > 0 || sphereInst > 0);
	}
	m_indirectBuffer.Unmap();
}

void GPUSceneManagement::RunFrame(float dt) {
	if (m_hostWindow.IsMinimised()) return;

	bool altHeld = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;

	// ShowCursor uses an internal reference counter. Force it to exactly
	// 0 (visible) or -1 (hidden) on each state transition.
	if (altHeld != m_altWasHeld) {
		m_altWasHeld = altHeld;
		HWND hwnd = static_cast<Win32Code::Win32Window&>(m_hostWindow).GetHandle();
		if (altHeld) {
			while (ShowCursor(TRUE) < 0);
			ClipCursor(nullptr);
		} else {
			while (ShowCursor(FALSE) >= 0);
			RECT r; GetClientRect(hwnd, &r);
			POINT tl{ r.left, r.top }, br{ r.right, r.bottom };
			ClientToScreen(hwnd, &tl); ClientToScreen(hwnd, &br);
			RECT cr{ tl.x, tl.y, br.x, br.y };
			ClipCursor(&cr);
		}
	}

	m_renderer->BeginFrame();

	ReadbackGPUVisibility();

	if (!altHeld)
		m_camera.UpdateCamera(dt);

	UploadCameraUniform();
	m_memoryManager->Update();

	if (m_renderPartial) {
		UpdateSceneFromTileGrid(m_tileGrid);
	}

	switch (m_benchConfig.scheme) {
		case RenderScheme::CPU_Instanced:    RenderScheme1(dt); break;
		case RenderScheme::CPU_CullIndirect: RenderScheme2(dt); break;
		case RenderScheme::GPU_CullIndirect: RenderScheme3(dt); break;
	}

#ifdef USE_IMGUI
	if (m_gui) {
		if (altHeld) {
			ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
			HWND hwnd = static_cast<Win32Code::Win32Window&>(m_hostWindow).GetHandle();
			m_gui->SyncInput(hwnd);
		} else {
			ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
		}
		m_gui->StartNewFrame();

		// BenchmarkPanel
		if (m_benchPanel) m_benchPanel->Render(this);
		if (m_benchPanel && m_benchPanel->ShouldRenderPartial()) {
			UpdateSceneFromTileGrid(m_benchPanel->GetPartialTileGrid());
			SetRenderPartial(true);
		}
		if (m_benchPanel && m_benchPanel->GenerationReady() && m_benchPanel->GetState() == PanelState::Ready) {
			SetRenderPartial(false);
		}

		// ChunkMonitor — recreate when scene dimensions change
		if (m_gridChunks > 0 && m_gridChunks <= 128) {
			if (!m_monitor || m_monitorGridChunks != m_gridChunks) {
				delete m_monitor;
				m_monitor = new ChunkMonitor(m_gridChunks, m_benchConfig.chunkSize, m_benchConfig.gridSize);
				m_monitorGridChunks = m_gridChunks;
			}
		}
		if (m_monitor) {
			std::vector<ChunkMonitorCell> cells(m_chunks.size());
			for (size_t i = 0; i < m_chunks.size(); ++i) {
				cells[i] = { m_chunks[i].instanceCount, m_chunkVisible[i] };
			}
			uint32_t dummyVis, dummyInst;
			m_monitor->Update(cells.data(), dummyVis, dummyInst);
			m_monitor->Render();
		}

		FrameContext const& ctx = m_renderer->GetFrameContext();
		m_gui->Render(ctx.cmdBuffer);
	}
#endif

	m_renderer->EndFrame();
	m_renderer->SwapBuffers();

	// Deferred query readback: last frame cmd buffer now submitted by EndFrame.
	if (m_pendingReadback) {
		m_pendingReadback = false;
		Finish();
		FrameContext const& rctx = m_renderer->GetFrameContext();
		uint32_t numQueries = 2 * (uint32_t)m_frameStats.size();
		std::vector<uint64_t> rawQueries(numQueries);
		vk::Result r = rctx.device.getQueryPoolResults(*m_queryPool, 0, numQueries,
			rawQueries.size() * sizeof(uint64_t), rawQueries.data(), sizeof(uint64_t),
			vk::QueryResultFlagBits::e64 | vk::QueryResultFlagBits::eWait);
		if (r == vk::Result::eSuccess) {
			for (uint32_t i = 0; i < m_frameStats.size(); ++i) {
				// GPU execution time from timestamp queries (independent of CPU record).
				m_frameStats[i].gpuExecUs = (double)(rawQueries[2*i+1] - rawQueries[2*i])
					* m_gpuTimestampPeriod / 1000.0;
			}
		}
		WriteCSVSummary();
		m_benchmarkComplete = true;
	}
	if (m_currentFrame == 0)
		std::cout << "[GPUDriven] First frame rendered\n";

	// benchmark progress: print every 100 frames during recording
	if (m_isRecording && (m_recordFrameIdx % 100 == 0)) {
		std::cout << "[Benchmark] frame " << m_currentFrame
		          << " (recording " << m_recordFrameIdx << " of "
		          << m_benchConfig.recordFrames << ")" << std::endl;
	}

	m_currentFrame++;
}

void GPUSceneManagement::GenerateScene() {
	std::cout << "[GPUDriven] Generating scene...\n";

	WFCGenerator gen;
	WFCConfig wfcCfg;
	wfcCfg.gridSize = m_benchConfig.gridSize;
	wfcCfg.seed     = m_benchConfig.seed;

	switch (m_benchConfig.density) {
		case 20: wfcCfg.emptyWeight = 8.0f; wfcCfg.otherWeight = 2.0f; break;
		case 50: wfcCfg.emptyWeight = 5.0f; wfcCfg.otherWeight = 5.0f; break;
		case 80: wfcCfg.emptyWeight = 2.0f; wfcCfg.otherWeight = 10.0f; break;
	}

	m_tileGrid = gen.Generate(wfcCfg);
	GenerateScene(m_tileGrid);
}

void GPUSceneManagement::GenerateScene(const std::vector<uint32_t>& tileGrid) {
	std::cout << "[DEBUG GenScene] gridSize=" << m_benchConfig.gridSize << " chunkSize=" << m_benchConfig.chunkSize << " tileGrid.size=" << tileGrid.size() << std::endl;
	WFCGenerator gen;
	auto instances = gen.TileGridToInstances(tileGrid, m_benchConfig.gridSize, m_cellSize);

	const uint32_t chunkDim = m_benchConfig.chunkSize;
	std::cout << "[DEBUG GenScene] instances=" << instances.size() << " m_gridChunks=" << m_gridChunks << std::endl;
	m_gridChunks = m_benchConfig.gridSize / chunkDim;

	std::vector<std::vector<WFCInstance>> buckets(m_gridChunks * m_gridChunks);
	for (const auto& inst : instances) {
		uint32_t cx = std::min((uint32_t)(inst.posX / (chunkDim * m_cellSize)), m_gridChunks - 1);
		uint32_t cy = std::min((uint32_t)(inst.posZ / (chunkDim * m_cellSize)), m_gridChunks - 1);
		buckets[cy * m_gridChunks + cx].push_back(inst);
	}

	std::vector<WFCInstance> sorted;
	sorted.reserve(instances.size());
	m_chunks.clear();
	m_chunks.reserve(m_gridChunks * m_gridChunks);

	for (uint32_t cy = 0; cy < m_gridChunks; ++cy) {
		for (uint32_t cx = 0; cx < m_gridChunks; ++cx) {
			auto& bucket = buckets[cy * m_gridChunks + cx];
			auto mid = std::stable_partition(bucket.begin(), bucket.end(),
				[](const WFCInstance& i) { return i.isCube; });

			ChunkInfo chunk = {};
			chunk.gridX          = cx;
			chunk.gridY          = cy;
			chunk.instanceOffset = (uint32_t)sorted.size();
			chunk.instanceCount  = (uint32_t)bucket.size();
			chunk.cubeCount      = (uint32_t)std::distance(bucket.begin(), mid);
			chunk.sphereCount    = (uint32_t)std::distance(mid, bucket.end());

			sorted.insert(sorted.end(), bucket.begin(), bucket.end());
			m_chunks.push_back(chunk);
		}
	}
	m_chunkVisible.assign(m_chunks.size(), true);

	m_totalInstances = (uint32_t)sorted.size();
	m_cullingData.resize(m_totalInstances);
	m_renderData.resize(m_totalInstances);
	for (uint32_t i = 0; i < m_totalInstances; ++i) {
		const auto& inst = sorted[i];
		m_cullingData[i] = { inst.posX, inst.posY, inst.posZ,
			inst.scaleX * 0.5f, inst.scaleY * 0.5f, inst.scaleZ * 0.5f };
		m_renderData[i]  = { inst.r, inst.g, inst.b, 1.0f };
	}

	ComputeChunkAABBs();

	std::cout << "[GPUDriven] " << m_totalInstances << " instances in "
	          << m_chunks.size() << " chunks (grid " << m_benchConfig.gridSize
	          << ", chunk " << m_benchConfig.chunkSize
	          << "), scheme=" << (int)m_benchConfig.scheme << "\n";
}

void GPUSceneManagement::UpdateSceneFromTileGrid(const std::vector<uint32_t>& grid) {
	WFCGenerator gen;
	auto instances = gen.TileGridToInstances(grid, m_benchConfig.gridSize, m_cellSize);
	m_cullingData.resize(instances.size());
	m_renderData.resize(instances.size());
	for (size_t i = 0; i < instances.size(); ++i) {
		m_cullingData[i] = { instances[i].posX, instances[i].posY, instances[i].posZ,
			instances[i].scaleX * 0.5f, instances[i].scaleY * 0.5f, instances[i].scaleZ * 0.5f };
		m_renderData[i]  = { instances[i].r, instances[i].g, instances[i].b, 1.0f };
	}
	m_totalInstances = (uint32_t)instances.size();
	WriteInstanceData();
	ComputeChunkAABBs();
}

void GPUSceneManagement::ComputeChunkAABBs() {
	for (auto& chunk : m_chunks) {
		float minX = FLT_MAX, minY = FLT_MAX, minZ = FLT_MAX;
		float maxX = -FLT_MAX, maxY = -FLT_MAX, maxZ = -FLT_MAX;
		for (uint32_t i = 0; i < chunk.instanceCount; ++i) {
			const auto& c = m_cullingData[chunk.instanceOffset + i];
			float cx = c.centerX, cy = c.centerY, cz = c.centerZ;
			float hx = c.halfX, hy = c.halfY, hz = c.halfZ;
			minX = std::min(minX, cx - hx); maxX = std::max(maxX, cx + hx);
			minY = std::min(minY, cy - hy); maxY = std::max(maxY, cy + hy);
			minZ = std::min(minZ, cz - hz); maxZ = std::max(maxZ, cz + hz);
		}
		chunk.aabbMinX = minX; chunk.aabbMinY = minY; chunk.aabbMinZ = minZ;
		chunk.aabbMaxX = maxX; chunk.aabbMaxY = maxY; chunk.aabbMaxZ = maxZ;
	}
}

void GPUSceneManagement::CreateBuffers() {
	FrameContext const& ctx = m_renderer->GetFrameContext();

	m_cullingBuffer = m_memoryManager->CreateBuffer(
		{ .size = sizeof(CullingDatum) * m_totalInstances, .usage = vk::BufferUsageFlagBits::eStorageBuffer },
		vk::MemoryPropertyFlagBits::eDeviceLocal, "CullingData");

	m_renderBuffer = m_memoryManager->CreateBuffer(
		{ .size = sizeof(RenderDatum) * m_totalInstances, .usage = vk::BufferUsageFlagBits::eStorageBuffer },
		vk::MemoryPropertyFlagBits::eDeviceLocal, "RenderData");

	uint32_t indirectSize = m_gridChunks * m_gridChunks * 2 * 5 * sizeof(uint32_t);
	m_indirectBuffer = m_memoryManager->CreateBuffer(
		{ .size = indirectSize, .usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer | vk::BufferUsageFlagBits::eTransferDst },
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
		"IndirectBuffer");

	m_chunkBuffer = m_memoryManager->CreateBuffer(
		{ .size = sizeof(ChunkInfo) * m_chunks.size(), .usage = vk::BufferUsageFlagBits::eStorageBuffer },
		vk::MemoryPropertyFlagBits::eDeviceLocal, "ChunkBuffer");

	WriteInstanceData();
}

void GPUSceneManagement::WriteInstanceData() {
	FrameContext const& ctx = m_renderer->GetFrameContext();

	auto stagingCull = m_memoryManager->CreateBuffer(
		{ .size = sizeof(CullingDatum) * m_totalInstances, .usage = vk::BufferUsageFlagBits::eTransferSrc },
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, "StgCull");
	memcpy(stagingCull.Map<CullingDatum>(), m_cullingData.data(), sizeof(CullingDatum) * m_totalInstances);
	stagingCull.Unmap();

	auto stagingRender = m_memoryManager->CreateBuffer(
		{ .size = sizeof(RenderDatum) * m_totalInstances, .usage = vk::BufferUsageFlagBits::eTransferSrc },
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, "StgRender");
	memcpy(stagingRender.Map<RenderDatum>(), m_renderData.data(), sizeof(RenderDatum) * m_totalInstances);
	stagingRender.Unmap();

	auto stagingChunk = m_memoryManager->CreateBuffer(
		{ .size = sizeof(ChunkInfo) * m_chunks.size(), .usage = vk::BufferUsageFlagBits::eTransferSrc },
		vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, "StgChunk");
	memcpy(stagingChunk.Map<ChunkInfo>(), m_chunks.data(), sizeof(ChunkInfo) * m_chunks.size());
	stagingChunk.Unmap();

	vk::UniqueCommandBuffer cmd = CmdBufferCreateBegin(ctx.device, ctx.commandPools[CommandType::Graphics], "Upload");
	vk::BufferCopy copyCull{0, 0, sizeof(CullingDatum) * m_totalInstances};
	cmd->copyBuffer(stagingCull.buffer, m_cullingBuffer.buffer, 1, &copyCull);
	vk::BufferCopy copyRender{0, 0, sizeof(RenderDatum) * m_totalInstances};
	cmd->copyBuffer(stagingRender.buffer, m_renderBuffer.buffer, 1, &copyRender);
	vk::BufferCopy copyChunk{0, 0, sizeof(ChunkInfo) * m_chunks.size()};
	cmd->copyBuffer(stagingChunk.buffer, m_chunkBuffer.buffer, 1, &copyChunk);
	CmdBufferEndSubmitWait(*cmd, ctx.device, ctx.queues[CommandType::Graphics]);

	m_memoryManager->DiscardBuffer(stagingCull, DiscardMode::Immediate);
	m_memoryManager->DiscardBuffer(stagingRender, DiscardMode::Immediate);
	m_memoryManager->DiscardBuffer(stagingChunk, DiscardMode::Immediate);
}

void GPUSceneManagement::CreatePipelines() {
	FrameContext const& ctx = m_renderer->GetFrameContext();

	m_sceneLayout = DescriptorSetLayoutBuilder(ctx.device)
		.WithUniformBuffers(0, 1, vk::ShaderStageFlagBits::eVertex)
		.WithStorageBuffers(1, 1, vk::ShaderStageFlagBits::eVertex)
		.WithStorageBuffers(2, 1, vk::ShaderStageFlagBits::eVertex)
		.Build("Scene Data");

	m_graphicsPipeline = PipelineBuilder(ctx.device)
		.WithVertexInputState(m_cubeMesh->GetVertexInputState())
		.WithTopology(vk::PrimitiveTopology::eTriangleList)
		.WithColourAttachment(ctx.colourFormat)
		.WithDepthAttachment(ctx.depthFormat)
		.WithDescriptorSetLayout(0, *m_sceneLayout)
		.WithShaderBinary("Scene.vert.spv", vk::ShaderStageFlagBits::eVertex)
		.WithShaderBinary("Scene.frag.spv", vk::ShaderStageFlagBits::eFragment)
		.Build("Scene Pipeline");

	m_computeLayout = DescriptorSetLayoutBuilder(ctx.device)
		.WithStorageBuffers(0, 1, vk::ShaderStageFlagBits::eCompute)
		.WithStorageBuffers(1, 1, vk::ShaderStageFlagBits::eCompute)
		.Build("Compute Layout");

	m_computePipeline = ComputePipelineBuilder(ctx.device)
		.WithDescriptorSetLayout(0, *m_computeLayout)
		.WithShaderBinary("Culling.comp.spv")
		.Build("GPU Culling");
}

void GPUSceneManagement::CreateDescriptorSets() {
	FrameContext const& ctx = m_renderer->GetFrameContext();

	m_sceneDescriptor = CreateDescriptorSet(ctx.device, ctx.descriptorPool, *m_sceneLayout);
	WriteBufferDescriptor(ctx.device, *m_sceneDescriptor, 0, vk::DescriptorType::eUniformBuffer, m_cameraBuffer);
	WriteBufferDescriptor(ctx.device, *m_sceneDescriptor, 1, vk::DescriptorType::eStorageBuffer, m_cullingBuffer);
	WriteBufferDescriptor(ctx.device, *m_sceneDescriptor, 2, vk::DescriptorType::eStorageBuffer, m_renderBuffer);

	m_computeDescriptor = CreateDescriptorSet(ctx.device, ctx.descriptorPool, *m_computeLayout);
	WriteBufferDescriptor(ctx.device, *m_computeDescriptor, 0, vk::DescriptorType::eStorageBuffer, m_chunkBuffer);
	WriteBufferDescriptor(ctx.device, *m_computeDescriptor, 1, vk::DescriptorType::eStorageBuffer, m_indirectBuffer);
}

void GPUSceneManagement::ExtractFrustumPlanes(Vector4 planes[6]) const {
	Matrix4 vp = m_camera.BuildProjectionMatrix(m_hostWindow.GetScreenAspect()) * m_camera.BuildViewMatrix();
	NCL_ExtractFrustumPlanes(vp, planes);
}


void GPUSceneManagement::RenderScheme1(float dt) {
	FrameContext const& ctx = m_renderer->GetFrameContext();
	BeginMeasurement();

	CpuSegBegin();  // segment 1: frustum cull (CPU work)
	Vector4 frustumPlanes[6];
	ExtractFrustumPlanes(frustumPlanes);

	uint32_t drawCount = 0;
	for (uint32_t i = 0; i < m_chunks.size(); ++i) {
		const auto& chunk = m_chunks[i];
		m_chunkVisible[i] = NCL_AABBInFrustum(frustumPlanes,
			chunk.aabbMinX, chunk.aabbMinY, chunk.aabbMinZ,
			chunk.aabbMaxX, chunk.aabbMaxY, chunk.aabbMaxZ);
	}
	CpuSegEnd();

	// BeginRenderToScreen contains the swapchain acquire fence wait — measure
	// it separately as present overhead, not pipeline CPU cost.
	LARGE_INTEGER fw0, fw1;
	QueryPerformanceCounter(&fw0);
	m_renderer->BeginRenderToScreen(ctx.cmdBuffer);
	QueryPerformanceCounter(&fw1);
	MarkFenceWait((double)(fw1.QuadPart - fw0.QuadPart) * m_cpuTimestampPeriod / 1000.0);

	CpuSegBegin();  // segment 2: pipeline bind + draw recording (CPU work)
	ctx.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_graphicsPipeline);
	ctx.cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_graphicsPipeline.layout,
		0, 1, &*m_sceneDescriptor, 0, nullptr);

	for (uint32_t i = 0; i < m_chunks.size(); ++i) {
		if (!m_chunkVisible[i] || m_chunks[i].instanceCount == 0) continue;
		const auto& chunk = m_chunks[i];

		if (chunk.cubeCount > 0) {
			m_cubeMesh->BindToCommandBuffer(ctx.cmdBuffer);
			ctx.cmdBuffer.drawIndexed(m_cubeIndexCount, chunk.cubeCount, 0, 0, chunk.instanceOffset);
			++drawCount;
		}
		if (chunk.sphereCount > 0) {
			m_sphereMesh->BindToCommandBuffer(ctx.cmdBuffer);
			ctx.cmdBuffer.drawIndexed(m_sphereIndexCount, chunk.sphereCount, 0, 0,
				chunk.instanceOffset + chunk.cubeCount);
			++drawCount;
		}
	}
	m_drawCallCount = drawCount;

	ctx.cmdBuffer.endRendering();
	CpuSegEnd();

	EndMeasurement();
}

void GPUSceneManagement::RenderScheme2(float dt) {
	FrameContext const& ctx = m_renderer->GetFrameContext();
	BeginMeasurement();

	CpuSegBegin();  // segment 1: cull + fill indirect buffer (CPU work)
	Vector4 frustumPlanes[6];
	ExtractFrustumPlanes(frustumPlanes);

	uint32_t* indirectMap = m_indirectBuffer.Map<uint32_t>();
	for (uint32_t i = 0; i < m_chunks.size(); ++i) {
		const auto& chunk = m_chunks[i];
		m_chunkVisible[i] = NCL_AABBInFrustum(frustumPlanes,
			chunk.aabbMinX, chunk.aabbMinY, chunk.aabbMinZ,
			chunk.aabbMaxX, chunk.aabbMaxY, chunk.aabbMaxZ);

		uint32_t base = i * 2 * 5;
		indirectMap[base + 0] = m_cubeIndexCount;
		indirectMap[base + 1] = (m_chunkVisible[i] && chunk.cubeCount > 0) ? chunk.cubeCount : 0;
		indirectMap[base + 2] = 0;
		indirectMap[base + 3] = 0;
		indirectMap[base + 4] = chunk.instanceOffset;

		indirectMap[base + 5] = m_sphereIndexCount;
		indirectMap[base + 6] = (m_chunkVisible[i] && chunk.sphereCount > 0) ? chunk.sphereCount : 0;
		indirectMap[base + 7] = 0;
		indirectMap[base + 8] = 0;
		indirectMap[base + 9] = chunk.instanceOffset + chunk.cubeCount;
	}
	m_indirectBuffer.Unmap();

	vk::MemoryBarrier2 memBarrier{};
	memBarrier.srcStageMask  = vk::PipelineStageFlagBits2::eHost;
	memBarrier.srcAccessMask = vk::AccessFlagBits2::eHostWrite;
	memBarrier.dstStageMask  = vk::PipelineStageFlagBits2::eDrawIndirect;
	memBarrier.dstAccessMask = vk::AccessFlagBits2::eIndirectCommandRead;
	ctx.cmdBuffer.pipelineBarrier2(vk::DependencyInfo().setMemoryBarriers(memBarrier));
	CpuSegEnd();

	LARGE_INTEGER fw0, fw1;
	QueryPerformanceCounter(&fw0);
	m_renderer->BeginRenderToScreen(ctx.cmdBuffer);
	QueryPerformanceCounter(&fw1);
	MarkFenceWait((double)(fw1.QuadPart - fw0.QuadPart) * m_cpuTimestampPeriod / 1000.0);

	CpuSegBegin();  // segment 2: indirect draw recording (CPU work)
	ctx.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_graphicsPipeline);
	ctx.cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_graphicsPipeline.layout,
		0, 1, &*m_sceneDescriptor, 0, nullptr);

	uint32_t stride    = 2 * 5 * sizeof(uint32_t);
	uint32_t drawCount = (uint32_t)m_chunks.size();

	m_cubeMesh->BindToCommandBuffer(ctx.cmdBuffer);
	ctx.cmdBuffer.drawIndexedIndirect(m_indirectBuffer.buffer, 0, drawCount, stride);

	m_sphereMesh->BindToCommandBuffer(ctx.cmdBuffer);
	ctx.cmdBuffer.drawIndexedIndirect(m_indirectBuffer.buffer, 5 * sizeof(uint32_t), drawCount, stride);

	m_drawCallCount = 2;

	ctx.cmdBuffer.endRendering();
	CpuSegEnd();
	EndMeasurement();
}

void GPUSceneManagement::RenderScheme3(float dt) {
	FrameContext const& ctx = m_renderer->GetFrameContext();
	BeginMeasurement();

	CpuSegBegin();  // segment 1: fillBuffer + compute dispatch recording (CPU work)
	uint32_t indirectSize = (uint32_t)m_chunks.size() * 2 * 5 * sizeof(uint32_t);
	ctx.cmdBuffer.fillBuffer(m_indirectBuffer.buffer, 0, indirectSize, 0);

	vk::BufferMemoryBarrier2 fillBarrier{};
	fillBarrier.srcStageMask  = vk::PipelineStageFlagBits2::eTransfer;
	fillBarrier.srcAccessMask = vk::AccessFlagBits2::eTransferWrite;
	fillBarrier.dstStageMask  = vk::PipelineStageFlagBits2::eComputeShader;
	fillBarrier.dstAccessMask = vk::AccessFlagBits2::eShaderWrite;
	fillBarrier.buffer = m_indirectBuffer.buffer;
	fillBarrier.size   = indirectSize;
	ctx.cmdBuffer.pipelineBarrier2(vk::DependencyInfo().setBufferMemoryBarriers(fillBarrier));

	Vector4 frustumPlanes[6];
	ExtractFrustumPlanes(frustumPlanes);

	struct { Vector4 planes[6]; uint32_t numChunks, cubeIndexCount, sphereIndexCount; } push;
	memcpy(push.planes, frustumPlanes, sizeof(frustumPlanes));
	push.numChunks       = (uint32_t)m_chunks.size();
	push.cubeIndexCount  = m_cubeIndexCount;
	push.sphereIndexCount = m_sphereIndexCount;

	ctx.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, m_computePipeline);
	ctx.cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, *m_computePipeline.layout,
		0, 1, &*m_computeDescriptor, 0, nullptr);
	ctx.cmdBuffer.pushConstants(*m_computePipeline.layout, vk::ShaderStageFlagBits::eCompute,
		0, sizeof(push), &push);
	ctx.cmdBuffer.dispatch(((uint32_t)m_chunks.size() + 63) / 64, 1, 1);

	vk::BufferMemoryBarrier2 computeBarrier{};
	computeBarrier.srcStageMask  = vk::PipelineStageFlagBits2::eComputeShader;
	computeBarrier.srcAccessMask = vk::AccessFlagBits2::eShaderWrite;
	computeBarrier.dstStageMask  = vk::PipelineStageFlagBits2::eDrawIndirect;
	computeBarrier.dstAccessMask = vk::AccessFlagBits2::eIndirectCommandRead;
	computeBarrier.buffer = m_indirectBuffer.buffer;
	computeBarrier.size   = indirectSize;
	ctx.cmdBuffer.pipelineBarrier2(vk::DependencyInfo().setBufferMemoryBarriers(computeBarrier));
	CpuSegEnd();

	LARGE_INTEGER fw0, fw1;
	QueryPerformanceCounter(&fw0);
	m_renderer->BeginRenderToScreen(ctx.cmdBuffer);
	QueryPerformanceCounter(&fw1);
	MarkFenceWait((double)(fw1.QuadPart - fw0.QuadPart) * m_cpuTimestampPeriod / 1000.0);

	CpuSegBegin();  // segment 2: indirect draw recording (CPU work)
	ctx.cmdBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_graphicsPipeline);
	ctx.cmdBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *m_graphicsPipeline.layout,
		0, 1, &*m_sceneDescriptor, 0, nullptr);

	uint32_t stride3    = 2 * 5 * sizeof(uint32_t);
	uint32_t drawCount3 = (uint32_t)m_chunks.size();

	m_cubeMesh->BindToCommandBuffer(ctx.cmdBuffer);
	ctx.cmdBuffer.drawIndexedIndirect(m_indirectBuffer.buffer, 0, drawCount3, stride3);

	m_sphereMesh->BindToCommandBuffer(ctx.cmdBuffer);
	ctx.cmdBuffer.drawIndexedIndirect(m_indirectBuffer.buffer, 5 * sizeof(uint32_t), drawCount3, stride3);

	m_drawCallCount = 2;

	ctx.cmdBuffer.endRendering();
	CpuSegEnd();
	EndMeasurement();
}

void GPUSceneManagement::BeginMeasurement() {
	if (!m_isRecording && m_benchmarkEnabled && !m_benchmarkComplete && m_currentFrame >= m_benchConfig.warmupFrames) {
		m_isRecording = true;
		m_recordFrameIdx = 0;
	}
	if (!m_isRecording) return;

	QueryPerformanceCounter(&m_frameStartQpc);
	m_cpuRecordAccumUs = 0.0;   // reset per-frame CPU recording accumulator
	m_cpuWaitUs = 0.0;

	FrameContext const& ctx = m_renderer->GetFrameContext();
	uint32_t qBase = 2 * m_recordFrameIdx;
	ctx.cmdBuffer.resetQueryPool(*m_queryPool, qBase, 2);
	ctx.cmdBuffer.writeTimestamp2(vk::PipelineStageFlagBits2::eTopOfPipe, *m_queryPool, qBase);
}

// Begin a CPU recording segment (excludes fence wait between segments).
void GPUSceneManagement::CpuSegBegin() {
	if (!m_isRecording) return;
	QueryPerformanceCounter(&m_segStartQpc);
}

// End a CPU recording segment, accumulating its duration.
void GPUSceneManagement::CpuSegEnd() {
	if (!m_isRecording) return;
	LARGE_INTEGER now;
	QueryPerformanceCounter(&now);
	m_cpuRecordAccumUs += (double)(now.QuadPart - m_segStartQpc.QuadPart)
	                     * m_cpuTimestampPeriod / 1000.0;
}

// Record the fence-wait duration (swapchain acquire), reported separately.
void GPUSceneManagement::MarkFenceWait(double waitUs) {
	if (!m_isRecording) return;
	m_cpuWaitUs += waitUs;
}

void GPUSceneManagement::EndMeasurement() {
	if (!m_isRecording) return;

	LARGE_INTEGER endQpc;
	QueryPerformanceCounter(&endQpc);

	FrameContext const& ctx = m_renderer->GetFrameContext();
	uint32_t qBase = 2 * m_recordFrameIdx;
	ctx.cmdBuffer.writeTimestamp2(vk::PipelineStageFlagBits2::eBottomOfPipe, *m_queryPool, qBase + 1);

	uint32_t visCount = 0;
	for (bool v : m_chunkVisible) if (v) ++visCount;

	FrameStats stats = {};
	stats.frameWallUs = (double)(endQpc.QuadPart - m_frameStartQpc.QuadPart)
	                   * m_cpuTimestampPeriod / 1000.0;
	stats.cpuRecordUs = m_cpuRecordAccumUs;   // cull + submit, excludes fence wait
	stats.cpuWaitUs   = m_cpuWaitUs;          // swapchain acquire fence wait
	stats.drawCalls   = m_drawCallCount;
	stats.visibleInstances = visCount;
	stats.gpuExecUs = 0;  // filled in deferred readback after bulk query read

	m_frameStats.push_back(stats);
	++m_recordFrameIdx;

		if (m_frameStats.size() >= m_benchConfig.recordFrames) {
			m_isRecording = false;
			m_pendingReadback = true;
		}
}

static void computeStats(const std::vector<double>& values, double& avg, double& minVal,
                         double& maxVal, double& p1, double& p99, double& stddev) {
	if (values.empty()) { avg = minVal = maxVal = p1 = p99 = stddev = 0; return; }
	avg = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
	auto [minIt, maxIt] = std::minmax_element(values.begin(), values.end());
	minVal = *minIt;
	maxVal = *maxIt;
	std::vector<double> sorted = values;
	std::sort(sorted.begin(), sorted.end());
	size_t n = sorted.size();
	p1  = sorted[std::max(size_t(1), n / 100) - 1];
	p99 = sorted[std::min(n, n * 99 / 100) - 1];
	stddev = 0.0;
	for (double v : values) stddev += (v - avg) * (v - avg);
	stddev = sqrt(stddev / n);
}

void GPUSceneManagement::WriteCSVSummary() {
	if (m_benchConfig.outputPath.empty()) return;

	// Ensure parent directory exists — ofstream will not create it.
	std::filesystem::path outPath(m_benchConfig.outputPath);
	if (outPath.has_parent_path())
		std::filesystem::create_directories(outPath.parent_path());

	std::ofstream file(m_benchConfig.outputPath);
	if (!file) {
		std::cerr << "[Benchmark] Failed to open " << m_benchConfig.outputPath << " for writing\n";
		return;
	}
	file << "frame,cpu_record_us,cpu_wait_us,gpu_exec_us,frame_wall_us,draw_calls,visible_instances\n";

	std::vector<double> cpuRecord, cpuWait, gpuExec, frameWall;
	for (uint32_t i = 0; i < m_frameStats.size(); ++i) {
		const auto& s = m_frameStats[i];
		file << i << "," << s.cpuRecordUs << "," << s.cpuWaitUs << "," << s.gpuExecUs << ","
		     << s.frameWallUs << "," << s.drawCalls << "," << s.visibleInstances << "\n";
		cpuRecord.push_back(s.cpuRecordUs);
		cpuWait.push_back(s.cpuWaitUs);
		gpuExec.push_back(s.gpuExecUs);
		frameWall.push_back(s.frameWallUs);
	}

	auto writeRow = [&](const char* label, const std::vector<double>& vals) {
		double a, mn, mx, p1, p99, sd;
		computeStats(vals, a, mn, mx, p1, p99, sd);
		file << label << "," << a << "," << mn << "," << mx << "," << p1 << "," << p99 << "," << sd << "\n";
	};

	file << "\ncpu_record_us_summary,avg,min,max,p1,p99,stddev\n";
	writeRow("cpu_record", cpuRecord);
	file << "\ncpu_wait_us_summary,avg,min,max,p1,p99,stddev\n";
	writeRow("cpu_wait", cpuWait);
	file << "\ngpu_exec_us_summary,avg,min,max,p1,p99,stddev\n";
	writeRow("gpu_exec", gpuExec);
	file << "\nframe_wall_us_summary,avg,min,max,p1,p99,stddev\n";
	writeRow("frame_wall", frameWall);

	std::cout << "[Benchmark] CSV written: " << m_benchConfig.outputPath
	          << " (" << m_frameStats.size() << " frames)\n";
}

/** Regenerates instance data for a subset of chunks, simulating local scene edits.
 *  updateCount random non-empty chunks are selected and their instances randomized
 *  (new scaleY, new color). Only the affected SSBO regions are re-uploaded.
 *  Timing is measured via warmup+record epochs identical to the main benchmark.
 */
void GPUSceneManagement::RegenerateChunks(uint32_t updateCount) {
	if (updateCount == 0 || m_chunks.empty()) return;

	std::cout << "[Update] Regenerating " << updateCount << " chunk(s)...\n";

	// Select random non-empty chunks
	std::vector<uint32_t> candidates;
	for (uint32_t i = 0; i < m_chunks.size(); ++i)
		if (m_chunks[i].instanceCount > 0) candidates.push_back(i);

	std::mt19937 rng(m_benchConfig.seed + 9999);
	std::shuffle(candidates.begin(), candidates.end(), rng);
	updateCount = std::min(updateCount, (uint32_t)candidates.size());
	candidates.resize(updateCount);

	// Regenerate instance data for selected chunks
	std::uniform_real_distribution<float> scaleDist(0.0f, 1.0f);
	for (uint32_t ci : candidates) {
		auto& chunk = m_chunks[ci];
		for (uint32_t j = 0; j < chunk.instanceCount; ++j) {
			uint32_t idx = chunk.instanceOffset + j;
			// Randomize scaleY within plausible bounds
			float newScaleY = 1.0f + scaleDist(rng) * 10.0f;
			m_cullingData[idx].halfY = newScaleY * 0.5f;
			m_cullingData[idx].centerY = newScaleY * 0.5f;
			// Randomize color slightly
			m_renderData[idx].r = 0.3f + scaleDist(rng) * 0.7f;
			m_renderData[idx].g = 0.3f + scaleDist(rng) * 0.7f;
			m_renderData[idx].b = 0.3f + scaleDist(rng) * 0.7f;
		}
	}

	// Re-upload affected SSBO regions via staging
	FrameContext const& ctx = m_renderer->GetFrameContext();
	for (uint32_t ci : candidates) {
		const auto& chunk = m_chunks[ci];
		uint32_t offset = chunk.instanceOffset;
		uint32_t count  = chunk.instanceCount;

		auto stgCull = m_memoryManager->CreateBuffer(
			{ .size = sizeof(CullingDatum) * count, .usage = vk::BufferUsageFlagBits::eTransferSrc },
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, "StgCullUpd");
		memcpy(stgCull.Map<CullingDatum>(), &m_cullingData[offset], sizeof(CullingDatum) * count);
		stgCull.Unmap();

		auto stgRender = m_memoryManager->CreateBuffer(
			{ .size = sizeof(RenderDatum) * count, .usage = vk::BufferUsageFlagBits::eTransferSrc },
			vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent, "StgRenderUpd");
		memcpy(stgRender.Map<RenderDatum>(), &m_renderData[offset], sizeof(RenderDatum) * count);
		stgRender.Unmap();

		vk::UniqueCommandBuffer cmd = CmdBufferCreateBegin(ctx.device, ctx.commandPools[CommandType::Graphics], "Update");
		vk::BufferCopy cpCull{0, offset * sizeof(CullingDatum), sizeof(CullingDatum) * count};
		cmd->copyBuffer(stgCull.buffer, m_cullingBuffer.buffer, 1, &cpCull);
		vk::BufferCopy cpRender{0, offset * sizeof(RenderDatum), sizeof(RenderDatum) * count};
		cmd->copyBuffer(stgRender.buffer, m_renderBuffer.buffer, 1, &cpRender);
		CmdBufferEndSubmitWait(*cmd, ctx.device, ctx.queues[CommandType::Graphics]);

		m_memoryManager->DiscardBuffer(stgCull, DiscardMode::Immediate);
		m_memoryManager->DiscardBuffer(stgRender, DiscardMode::Immediate);
	}

	// Force indirect buffer refill for schemes 2 (next frame CPU fill will pick up changes)
	// Scheme 3 GPU compute reads the new SSBO automatically.

	std::cout << "[Update] " << updateCount << " chunks updated, "
	          << "total instances affected: ";
	uint32_t totalAffected = 0;
	for (uint32_t ci : candidates) totalAffected += m_chunks[ci].instanceCount;
	std::cout << totalAffected << "\n";
}

/** Offscreen rendering stubs — not yet implemented.
 *  Headless benchmark currently uses a tiny window + swapchain.
 *  True headless with VK_KHR_display / offscreen-render-to-image is future work.
 */
