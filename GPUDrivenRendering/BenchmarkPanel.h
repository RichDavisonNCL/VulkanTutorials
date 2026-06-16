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

enum class PanelState { Idle, Generating, Ready, Recording, Results };

class BenchmarkPanel {
public:
    BenchmarkPanel();
    ~BenchmarkPanel();
    void Render(class GPUSceneManagement* app);
    void GenerationThread(class GPUSceneManagement* app, uint32_t gridSize,
                          uint32_t seed, uint32_t density);
    PanelState GetState() const { return m_state; }
    bool ShouldRenderPartial() const { return m_state == PanelState::Generating && m_showGeneration; }
    const std::vector<uint32_t>& GetPartialTileGrid() const { return m_partialTileGrid; }
    bool GenerationReady() const { return m_generationReady; }

private:
    void RenderGenerationSection(class GPUSceneManagement* app);
    void RenderBenchmarkSection(class GPUSceneManagement* app);
    void RenderResultsSection(class GPUSceneManagement* app);

    PanelState m_state = PanelState::Idle;
    uint32_t m_genGridSize = 512;
    uint32_t m_genSeed = 42;
    uint32_t m_genDensity = 50;
    bool m_useCache = true;
    bool m_showGeneration = true;
    int m_scheme = 3;
    int m_warmupFrames = 60;
    int m_recordFrames = 200;
    bool m_benchmarkRunning = false;
    std::string m_csvPath;

    std::thread m_genThread;
    std::mutex m_tileGridMutex;
    std::atomic<uint32_t> m_genProgress{0};
    std::atomic<bool> m_generationReady{false};
    std::vector<uint32_t> m_partialTileGrid;
    uint32_t m_totalCells = 0;

    std::vector<FrameStats> m_lastResults;
    int m_lastScheme = 0;
    std::vector<float> m_frameTimes;
    int m_chartFrame = 0;
};
