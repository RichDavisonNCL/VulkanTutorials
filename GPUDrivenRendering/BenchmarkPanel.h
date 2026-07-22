/** @file BenchmarkPanel.h
 * ImGui benchmark dashboard — scene generation, cache, benchmark recording,
 * real-time ImPlot charts, ChunkMonitor integration.
 */
#pragma once
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <cstdint>
#include <functional>

struct BenchmarkConfig;
struct FrameStats;
enum class RenderScheme : uint32_t;

// Loading sits between "generation finished" and "Ready": the tile grid is in
// hand but the GPU rebuild (GenerateScene/CreateBuffers/CreateDescriptorSets/
// CreateQueryPool) has NOT run yet. That rebuild must happen at a frame boundary
// in RunFrame (before BeginFrame), never inside the ImGui Render callback — doing
// it mid-frame frees buffers/descriptors the already-recorded draw commands still
// reference, causing a device-lost -> abort(). Loading is the one-frame handoff.
enum class PanelState { Idle, Generating, Loading, Ready, Recording, Results };

class BenchmarkPanel {
public:
    BenchmarkPanel();
    ~BenchmarkPanel();
    void Render(class GPUSceneManagement* app);
    void GenerationThread(class GPUSceneManagement* app, uint32_t gridSize,
                          uint32_t seed, uint32_t density);
    PanelState GetState() const { return m_state; }
    bool ShouldRenderPartial() const { return false; }
    const std::vector<uint32_t>& GetPartialTileGrid() const { return m_partialTileGrid; }
    bool GenerationReady() const { return m_generationReady; }

    // Frame-boundary rebuild handoff. When the panel has a tile grid ready to
    // load, it sets m_pendingRebuild instead of touching Vulkan itself.
    // GPUSceneManagement::RunFrame() polls this BEFORE BeginFrame(), performs
    // the GPU rebuild there, then calls RebuildConsumed() to advance the panel
    // to Ready. See PanelState::Loading for why this indirection is required.
    bool HasPendingRebuild() const { return m_pendingRebuild; }
    const std::vector<uint32_t>& GetRebuildGrid() const { return m_partialTileGrid; }
    uint32_t GetGenGridSize() const { return m_genGridSize; }
    uint32_t GetGenChunkSize() const { return m_genChunkSize; }
    uint32_t GetGenSeed() const { return m_genSeed; }
    uint32_t GetGenDensity() const { return m_genDensity; }
    void RebuildConsumed() { m_pendingRebuild = false; m_state = PanelState::Ready; }

private:
    void RenderGenerationSection(class GPUSceneManagement* app);
    void RenderBenchmarkSection(class GPUSceneManagement* app);
    void RenderResultsSection(class GPUSceneManagement* app);

    PanelState m_state = PanelState::Idle;
    uint32_t m_genGridSize = 512;
	uint32_t m_genChunkSize = 16;
    uint32_t m_genSeed = 42;
    uint32_t m_genDensity = 50;
    bool m_useCache = true;
    bool m_showGeneration = true;
    int m_scheme = 3;
    int m_warmupFrames = 60;
    int m_recordFrames = 200;
    std::string m_csvPath;

    std::thread m_genThread;
    std::mutex m_tileGridMutex;
    std::atomic<uint32_t> m_genProgress{0};
    std::atomic<bool> m_generationReady{false};
    std::vector<uint32_t> m_partialTileGrid;
    uint32_t m_totalCells = 0;
    bool m_pendingRebuild = false;   // grid ready; RunFrame owes a GPU rebuild

    std::vector<FrameStats> m_lastResults;
    int m_lastScheme = 0;
    std::vector<float> m_frameTimes;
    // Per-frame GPU timestamps are read back at the end of the frame the
    // benchmark completes (after this panel renders), so the results copy is
    // deferred one frame — otherwise m_lastResults captures gpuExecUs == 0.
    bool m_pendingResultCopy = false;

    // Summary of one finalized benchmark run, kept so the results charts can
    // compare the current run against the previous one (e.g. scheme 1 vs 3).
    struct RunSummary {
        bool valid = false;
        int scheme = 0;
        size_t frames = 0;
        uint32_t drawCalls = 0;
        double cpuAvg = 0, cpuP1 = 0, cpuP99 = 0;
        double gpuAvg = 0, gpuP1 = 0, gpuP99 = 0;
    };
    RunSummary m_curSummary;    // most recent finalized run
    RunSummary m_prevSummary;   // the one before it (invalid until 2 runs done)
};
