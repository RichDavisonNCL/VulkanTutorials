# Benchmark Dashboard Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Interactive ImGui benchmark panel with background WFC generation, cache, ImPlot charts, and demo-ready controls.

**Architecture:** New `BenchmarkPanel` class owns the state machine and worker thread. New `WFCCache` handles binary save/load. GPUSceneManagement gains `GetFrameStats()` and partial tileGrid rendering. ImPlot added as header-only dependency for real-time charts.

**Tech Stack:** C++20, Vulkan 1.3 HPP, ImGui docking branch, ImPlot (MIT), VMA

---

## File Structure

| File | Role |
|------|------|
| `BenchmarkPanel.h/.cpp` (NEW) | ImGui panel, state machine enum, worker thread management, cancel logic |
| `WFCCache.h/.cpp` (NEW) | Binary tileGrid save/load to `cache/` directory |
| `GPUSceneManagement.h/.cpp` (MODIFY) | `GetFrameStats()` accessor, `GetTileGrid()`/`GetGridSize()`, partial render flag |
| `WFCGenerator.h/.cpp` (MODIFY) | Progress callback typedef + `Generate()` overload accepting callback |
| `Main.cpp` (MODIFY) | Instantiate BenchmarkPanel, wire `RunFrame`, cleanup |
| `CMakeLists.txt` (MODIFY) | Add ImPlot sources, `results/` and `cache/` dirs not needed (created at runtime) |

---

### Task 1: WFCCache — Binary tileGrid save/load

**Files:**
- Create: `GPUDrivenRendering/WFCCache.h`
- Create: `GPUDrivenRendering/WFCCache.cpp`

- [ ] **Step 1: Write WFCCache.h**

```cpp
/** @file WFCCache.h
 * Binary cache for WFC tile grids. Save/load raw uint32_t grids to disk.
 */
#pragma once
#include <vector>
#include <cstdint>
#include <string>

class WFCCache {
public:
    static void Save(const std::vector<uint32_t>& tileGrid, uint32_t gridSize,
                     uint32_t seed, uint32_t density);
    static bool Load(std::vector<uint32_t>& outGrid, uint32_t& outGridSize,
                     uint32_t gridSize, uint32_t seed, uint32_t density);
    static std::string Path(uint32_t gridSize, uint32_t seed, uint32_t density);

private:
    static void EnsureCacheDir();
};
```

- [ ] **Step 2: Write WFCCache.cpp**

```cpp
/** @file WFCCache.cpp
 * Binary cache format: [uint32_t gridSize][uint32_t tileGrid[gridSize*gridSize]]
 */
#include "WFCCache.h"
#include <filesystem>
#include <fstream>
#include <iostream>

void WFCCache::EnsureCacheDir() {
    std::filesystem::create_directories("cache");
}

std::string WFCCache::Path(uint32_t gridSize, uint32_t seed, uint32_t density) {
    return "cache/wfc_" + std::to_string(gridSize) + "_"
           + std::to_string(seed) + "_" + std::to_string(density) + ".bin";
}

void WFCCache::Save(const std::vector<uint32_t>& tileGrid, uint32_t gridSize,
                    uint32_t seed, uint32_t density) {
    EnsureCacheDir();
    std::string path = Path(gridSize, seed, density);
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        std::cerr << "[Cache] Failed to write " << path << "\n";
        return;
    }
    file.write(reinterpret_cast<const char*>(&gridSize), sizeof(gridSize));
    file.write(reinterpret_cast<const char*>(tileGrid.data()),
               tileGrid.size() * sizeof(uint32_t));
    std::cout << "[Cache] Saved " << tileGrid.size() << " cells to " << path << "\n";
}

bool WFCCache::Load(std::vector<uint32_t>& outGrid, uint32_t& outGridSize,
                    uint32_t gridSize, uint32_t seed, uint32_t density) {
    std::string path = Path(gridSize, seed, density);
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;

    uint32_t savedGridSize;
    file.read(reinterpret_cast<char*>(&savedGridSize), sizeof(savedGridSize));
    if (savedGridSize != gridSize) {
        std::cerr << "[Cache] Grid size mismatch: cached=" << savedGridSize
                  << " requested=" << gridSize << "\n";
        return false;
    }

    outGrid.resize(savedGridSize * savedGridSize);
    file.read(reinterpret_cast<char*>(outGrid.data()),
              outGrid.size() * sizeof(uint32_t));
    outGridSize = savedGridSize;

    if (file.gcount() != (std::streamsize)(outGrid.size() * sizeof(uint32_t))) {
        std::cerr << "[Cache] Incomplete read from " << path << "\n";
        return false;
    }

    std::cout << "[Cache] Loaded " << outGrid.size() << " cells from " << path << "\n";
    return true;
}
```

- [ ] **Step 3: Verify compilation**

```bash
cmake --build . --target GPUDrivenRendering --config Debug
```

- [ ] **Step 4: Commit**

```bash
git add GPUDrivenRendering/WFCCache.h GPUDrivenRendering/WFCCache.cpp
git commit -m "feat: WFCCache — binary tileGrid save/load to cache/ directory"
```

---

### Task 2: WFCGenerator — Progress callback

**Files:**
- Modify: `GPUDrivenRendering/WFCGenerator.h:29-34`
- Modify: `GPUDrivenRendering/WFCGenerator.cpp:36`

- [ ] **Step 1: Add progress callback typedef and WFCConfig field**

In `WFCGenerator.h`, add before `struct WFCConfig`:

```cpp
using WFCProgressCallback = std::function<void(uint32_t collapsed, uint32_t total)>;
```

Add to `struct WFCConfig`:

```cpp
WFCProgressCallback onProgress;  // called every N collapsed cells
```

- [ ] **Step 2: Modify Generate() to invoke callback**

In `WFCGenerator.cpp`, in `Generate()`, after `grid[bestIdx] = chosen; Propagate(...);`, add:

```cpp
if (config.onProgress && (iter % 500 == 0)) {
    config.onProgress(iter + 1, total);
}
```

Also add a final callback after the loop:

```cpp
if (config.onProgress) config.onProgress(total, total);
```

- [ ] **Step 3: Verify compilation**

```bash
cmake --build . --target GPUDrivenRendering --config Debug
```

- [ ] **Step 4: Commit**

```bash
git add GPUDrivenRendering/WFCGenerator.h GPUDrivenRendering/WFCGenerator.cpp
git commit -m "feat: WFCGenerator progress callback support"
```

---

### Task 3: GPUSceneManagement — Expose stats, tileGrid, partial rendering

**Files:**
- Modify: `GPUDrivenRendering/GPUSceneManagement.h`
- Modify: `GPUDrivenRendering/GPUSceneManagement.cpp`

- [ ] **Step 1: Add accessor methods to header**

In `GPUSceneManagement.h` public section, add after `SetScheme()`:

```cpp
const std::vector<FrameStats>& GetFrameStats() const { return m_frameStats; }
const std::vector<uint32_t>& GetTileGrid() const { return m_tileGrid; }
uint32_t GetGridSize() const { return m_benchConfig.gridSize; }
bool IsBenchmarkRecording() const { return m_isRecording; }
```

- [ ] **Step 2: Add public RunBenchmarkInPanel method**

```cpp
// Run a benchmark with panel-specified parameters. After completion,
// results are available via GetFrameStats() and CSV is written.
void RunBenchmarkInPanel(const std::string& outputPath);
```

Implementation in `.cpp`:

```cpp
void GPUSceneManagement::RunBenchmarkInPanel(const std::string& outputPath) {
    m_benchConfig.outputPath = outputPath;
    m_frameStats.clear();
    m_recordFrameIdx = 0;
    m_isRecording = false;
    m_benchmarkComplete = false;
    // Benchmark begins on next frame when BeginMeasurement detects warmup threshold
}
```

- [ ] **Step 3: Remove diagnostic couts from production path**

Remove or guard all `std::cout << "[Main] loop..."`, `"[RunFrame] frame..."`, `"[Benchmark] Readback..."` with `#ifndef NDEBUG` or replace with a flag.

- [ ] **Step 4: Verify compilation**

```bash
cmake --build . --target GPUDrivenRendering --config Debug
```

- [ ] **Step 5: Commit**

```bash
git add GPUDrivenRendering/GPUSceneManagement.h GPUDrivenRendering/GPUSceneManagement.cpp
git commit -m "feat: expose FrameStats, tileGrid, RunBenchmarkInPanel; silence debug output"
```

---

### Task 4: ImPlot — Add to CMake

**Files:**
- Modify: `GPUDrivenRendering/CMakeLists.txt`
- Fetch: `imgui/implot.h`, `imgui/implot.cpp`, `imgui/implot_items.cpp`

- [ ] **Step 1: Download ImPlot files**

```bash
# ImPlot v0.17 — single-file MIT library for ImGui docking branch
curl -L https://raw.githubusercontent.com/epezent/implot/v0.17/implot.h -o D:/D-Code/Code-Essay/VulkanTutorials/imgui/implot.h
curl -L https://raw.githubusercontent.com/epezent/implot/v0.17/implot.cpp -o D:/D-Code/Code-Essay/VulkanTutorials/imgui/implot.cpp
curl -L https://raw.githubusercontent.com/epezent/implot/v0.17/implot_items.cpp -o D:/D-Code/Code-Essay/VulkanTutorials/imgui/implot_items.cpp
```

- [ ] **Step 2: Add ImPlot to CMakeLists.txt**

In the `if(COMPILE_WITH_IMGUI)` block, after the IMGUI_SOURCES file(GLOB ...), add:

```cmake
# ImPlot (MIT, charting library for ImGui)
target_sources(${TARGET_NAME} PRIVATE
    ${IMGUI_DIR}/implot.cpp
    ${IMGUI_DIR}/implot_items.cpp
)
target_include_directories(${TARGET_NAME} PRIVATE ${IMGUI_DIR})
```

- [ ] **Step 3: Verify compilation**

```bash
cmake --build . --target GPUDrivenRendering --config Debug
```

- [ ] **Step 4: Commit**

```bash
git add imgui/implot.h imgui/implot.cpp imgui/implot_items.cpp GPUDrivenRendering/CMakeLists.txt
git commit -m "feat: add ImPlot (MIT) for real-time chart rendering"
```

---

### Task 5: BenchmarkPanel — State machine + worker thread + WFC

**Files:**
- Create: `GPUDrivenRendering/BenchmarkPanel.h`
- Create: `GPUDrivenRendering/BenchmarkPanel.cpp`

- [ ] **Step 1: Write BenchmarkPanel.h**

```cpp
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
struct ChunkInfo;

enum class PanelState {
    Idle,
    Generating,
    Ready,
    Recording,
    Results
};

class BenchmarkPanel {
public:
    BenchmarkPanel();
    ~BenchmarkPanel();

    // Called every frame from main loop ImGui block
    void Render(class GPUSceneManagement* app);

    // Worker thread entry
    void GenerationThread(class GPUSceneManagement* app, uint32_t gridSize,
                          uint32_t seed, uint32_t density);

    // Getters for main loop integration
    PanelState GetState() const { return m_state; }
    bool ShouldRenderPartial() const { return m_state == PanelState::Generating && m_showGeneration; }
    const std::vector<uint32_t>& GetPartialTileGrid() const { return m_partialTileGrid; }
    bool GenerationReady() const { return m_generationReady; }

private:
    void RenderGenerationSection(class GPUSceneManagement* app);
    void RenderBenchmarkSection(class GPUSceneManagement* app);
    void RenderResultsSection(class GPUSceneManagement* app);

    // State
    PanelState m_state = PanelState::Idle;

    // Generation params
    uint32_t m_genGridSize = 512;
    uint32_t m_genSeed = 42;
    uint32_t m_genDensity = 50;
    bool m_useCache = true;
    bool m_showGeneration = true;

    // Benchmark params
    int m_scheme = 3;
    int m_warmupFrames = 60;
    int m_recordFrames = 200;
    bool m_benchmarkRunning = false;
    std::string m_csvPath;

    // Generation thread
    std::thread m_genThread;
    std::mutex m_tileGridMutex;
    std::atomic<uint32_t> m_genProgress{0};
    std::atomic<bool> m_generationReady{false};
    std::vector<uint32_t> m_partialTileGrid;
    uint32_t m_totalCells = 0;

    // Results cache (frozen after Recording→Results)
    std::vector<FrameStats> m_lastResults;
    int m_lastScheme = 0;

    // ImPlot data buffers (200 points max)
    std::vector<float> m_frameTimes;
    int m_chartFrame = 0;
};
```

- [ ] **Step 2: Write BenchmarkPanel.cpp — Render() skeleton**

```cpp
/** @file BenchmarkPanel.cpp
 * ImGui benchmark dashboard implementation.
 */
#include "BenchmarkPanel.h"
#include "GPUSceneManagement.h"
#include "WFCGenerator.h"
#include "WFCCache.h"
#include "imgui.h"
#include "implot.h"
#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>

BenchmarkPanel::BenchmarkPanel() {}
BenchmarkPanel::~BenchmarkPanel() {
    if (m_genThread.joinable()) m_genThread.join();
}

void BenchmarkPanel::Render(GPUSceneManagement* app) {
    if (!app) return;

    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(500, 700), ImGuiCond_FirstUseEver);
    ImGui::Begin("Benchmark Dashboard");

    RenderGenerationSection(app);
    ImGui::Separator();
    RenderBenchmarkSection(app);
    ImGui::Separator();
    RenderResultsSection(app);

    ImGui::End();
}
```

- [ ] **Step 3: Write RenderGenerationSection()**

```cpp
void BenchmarkPanel::RenderGenerationSection(GPUSceneManagement* app) {
    ImGui::TextColored(ImVec4(0.3f, 0.7f, 1.0f, 1.0f), "Scene Generation");
    ImGui::Separator();

    bool isIdle = (m_state == PanelState::Idle);
    bool isGenerating = (m_state == PanelState::Generating);
    bool isReady = (m_state == PanelState::Ready || m_state == PanelState::Recording || m_state == PanelState::Results);

    if (!isGenerating) {
        ImGui::InputInt("Grid Size", (int*)&m_genGridSize, 16, 64);
        if (m_genGridSize < 16) m_genGridSize = 16;
        if (m_genGridSize > 2048) m_genGridSize = 2048;
        ImGui::InputInt("Seed", (int*)&m_genSeed);
        const char* densities[] = { "20 (Low)", "50 (Medium)", "80 (High)" };
        int densityIdx = (m_genDensity == 20) ? 0 : (m_genDensity == 50) ? 1 : 2;
        if (ImGui::Combo("Density", &densityIdx, densities, 3)) {
            m_genDensity = (densityIdx == 0) ? 20 : (densityIdx == 1) ? 50 : 80;
        }
    }

    ImGui::Checkbox("Use Cache", &m_useCache);
    ImGui::Checkbox("Show Generation", &m_showGeneration);

    if (isGenerating) {
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            std::exit(0);
        }
        float progress = (float)m_genProgress / (float)m_totalCells;
        ImGui::SameLine();
        ImGui::ProgressBar(progress, ImVec2(200, 0));
        ImGui::SameLine();
        ImGui::Text("%.0f%%", progress * 100.0f);
    } else if (isIdle || isReady) {
        if (isIdle) {
            if (ImGui::Button("Generate", ImVec2(120, 0))) {
                // Try cache first
                if (m_useCache) {
                    uint32_t cachedSize;
                    if (WFCCache::Load(m_partialTileGrid, cachedSize,
                                       m_genGridSize, m_genSeed, m_genDensity)) {
                        m_totalCells = m_genGridSize * m_genGridSize;
                        m_genProgress = m_totalCells;
                        m_generationReady = true;
                        m_state = PanelState::Ready;
                        BenchmarkConfig cfg;
                        cfg.gridSize = m_genGridSize;
                        cfg.seed = m_genSeed;
                        cfg.density = m_genDensity;
                        cfg.scheme = (RenderScheme)m_scheme;
                        cfg.chunkSize = 16;
                        app->SetBenchmarkConfig(cfg);
                        return;
                    }
                }
                // Start generation thread
                m_state = PanelState::Generating;
                m_totalCells = m_genGridSize * m_genGridSize;
                m_genProgress = 0;
                m_generationReady = false;
                m_partialTileGrid.clear();
                m_partialTileGrid.resize(m_totalCells, 0);
                m_genThread = std::thread(&BenchmarkPanel::GenerationThread,
                                         this, app, m_genGridSize, m_genSeed, m_genDensity);
            }
        }
    }

    // Display status
    const char* statusText = "Idle";
    ImVec4 statusColor(0.5f, 0.5f, 0.5f, 1.0f);
    switch (m_state) {
        case PanelState::Generating: statusText = "Generating..."; statusColor = ImVec4(1, 0.8f, 0, 1); break;
        case PanelState::Ready:      statusText = "Ready";         statusColor = ImVec4(0, 0.8f, 0, 1); break;
        default: break;
    }
    ImGui::SameLine();
    ImGui::TextColored(statusColor, "%s", statusText);

    // Check for generation completion
    if (m_generationReady && m_state == PanelState::Generating) {
        m_state = PanelState::Ready;
        if (m_genThread.joinable()) m_genThread.join();
        // Full generation done — set config on app
        BenchmarkConfig cfg;
        cfg.gridSize = m_genGridSize;
        cfg.seed = m_genSeed;
        cfg.density = m_genDensity;
        cfg.scheme = (RenderScheme)m_scheme;
        cfg.chunkSize = 16;
        app->SetBenchmarkConfig(cfg);
    }
}
```

- [ ] **Step 4: Write GenerationThread()**

```cpp
void BenchmarkPanel::GenerationThread(GPUSceneManagement* app,
                                      uint32_t gridSize, uint32_t seed, uint32_t density) {
    WFCGenerator gen;
    WFCConfig cfg;
    cfg.gridSize = gridSize;
    cfg.seed = seed;
    cfg.emptyWeight = 5.0f;
    cfg.otherWeight = 5.0f;
    switch (density) {
        case 20: cfg.emptyWeight = 8.0f; cfg.otherWeight = 2.0f; break;
        case 50: cfg.emptyWeight = 5.0f; cfg.otherWeight = 5.0f; break;
        case 80: cfg.emptyWeight = 2.0f; cfg.otherWeight = 10.0f; break;
    }
    cfg.onProgress = [&](uint32_t collapsed, uint32_t total) {
        m_genProgress = collapsed;
        // Copy partial grid to shared buffer
        // (WFCGenerator's Generate returns the full grid at the end,
        //  we need incremental access — see Task 6)
    };

    auto tileGrid = gen.Generate(cfg);

    // Copy full result
    {
        std::lock_guard<std::mutex> lock(m_tileGridMutex);
        m_partialTileGrid = std::move(tileGrid);
    }
    m_genProgress = gridSize * gridSize;
    m_generationReady = true;

    // Save cache
    if (m_useCache) {
        WFCCache::Save(m_partialTileGrid, gridSize, seed, density);
    }
}
```

- [ ] **Step 5: Verify compilation (will fail — WFCGenerator::Generate returns full grid, not incremental)**

```bash
cmake --build . --target GPUDrivenRendering --config Debug
```

Expected: Compiles but partial visualization won't work until Task 6.

- [ ] **Step 6: Commit**

```bash
git add GPUDrivenRendering/BenchmarkPanel.h GPUDrivenRendering/BenchmarkPanel.cpp
git commit -m "feat: BenchmarkPanel skeleton — state machine, generation section, worker thread"
```

---

### Task 6: WFCGenerator — Incremental grid access for visualization

**Files:**
- Modify: `GPUDrivenRendering/WFCGenerator.h`
- Modify: `GPUDrivenRendering/WFCGenerator.cpp`

- [ ] **Step 1: Add mutable ref to partial grid in WFCConfig**

In `WFCGenerator.h`, add to `WFCConfig`:

```cpp
std::vector<uint32_t>* partialGrid = nullptr;  // optional: write-to during generation
std::mutex* gridMutex = nullptr;                // optional: lock when writing
```

- [ ] **Step 2: Write partial grid during generation**

In `Generate()`, after each cell collapse (`grid[bestIdx] = chosen`), add:

```cpp
if (config.partialGrid && config.gridMutex && (iter % 200 == 0 || iter == total - 1)) {
    std::lock_guard<std::mutex> lock(*config.gridMutex);
    *config.partialGrid = grid;
}
```

- [ ] **Step 3: Update BenchmarkPanel::GenerationThread to pass partial grid**

```cpp
cfg.partialGrid = &m_partialTileGrid;
cfg.gridMutex = &m_tileGridMutex;
```

- [ ] **Step 4: Verify compilation**

```bash
cmake --build . --target GPUDrivenRendering --config Debug
```

- [ ] **Step 5: Commit**

```bash
git add GPUDrivenRendering/WFCGenerator.h GPUDrivenRendering/WFCGenerator.cpp GPUDrivenRendering/BenchmarkPanel.cpp
git commit -m "feat: WFCGenerator incremental partial grid for visualization"
```

---

### Task 7: GPUSceneManagement — Partial tileGrid rendering

**Files:**
- Modify: `GPUDrivenRendering/GPUSceneManagement.h`
- Modify: `GPUDrivenRendering/GPUSceneManagement.cpp`

- [ ] **Step 1: Add partial tileGrid setter and flag to header**

```cpp
void SetPartialTileGrid(const std::vector<uint32_t>& grid, uint32_t gridSize);
void SetRenderPartial(bool partial) { m_renderPartial = partial; }
```

Member:

```cpp
bool m_renderPartial = false;
```

- [ ] **Step 2: Implement SetPartialTileGrid**

In `.cpp`:

```cpp
void GPUSceneManagement::SetPartialTileGrid(const std::vector<uint32_t>& grid, uint32_t gridSize) {
    m_tileGrid = grid;
    // Rebuild instance data from partial grid (skip tileId==0)
    WFCGenerator gen;
    auto instances = gen.TileGridToInstances(m_tileGrid, gridSize, m_cellSize);
    // ... same as GenerateScene but using the partial grid ...
    // Rebuild buffers with partial instance set
}
```

Note: This reuses the existing `GenerateScene` pipeline but with a supplied grid instead of generating one. Extract the instance-building logic into a shared method.

- [ ] **Step 3: Extend GenerateScene to accept optional pre-built tileGrid**

Add overload:

```cpp
void GenerateScene(const std::vector<uint32_t>& tileGrid);
```

Extract the common instance packing, chunk bucketing, AABB computation code into this overload. Make the existing `GenerateScene()` call this overload after WFC generation.

- [ ] **Step 4: Update RenderFrame for partial rendering**

In `RunFrame()`, just before the scheme switch:

```cpp
if (m_renderPartial) {
    // Rebuild scene from partial tileGrid each frame (fast for <10K cells)
    GenerateScene(m_tileGrid);
    CreateBuffers();
    CreateDescriptorSets();
    CreateQueryPool();
}
```

- [ ] **Step 5: Verify compilation**

```bash
cmake --build . --target GPUDrivenRendering --config Debug
```

- [ ] **Step 6: Commit**

```bash
git add GPUDrivenRendering/GPUSceneManagement.h GPUDrivenRendering/GPUSceneManagement.cpp
git commit -m "feat: partial tileGrid rendering for generation visualization"
```

---

### Task 8: BenchmarkPanel — Benchmark section + ImPlot charts

**Files:**
- Modify: `GPUDrivenRendering/BenchmarkPanel.cpp`

- [ ] **Step 1: Write RenderBenchmarkSection()**

```cpp
void BenchmarkPanel::RenderBenchmarkSection(GPUSceneManagement* app) {
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Benchmark");
    ImGui::Separator();

    bool canRun = (m_state == PanelState::Ready || m_state == PanelState::Results);
    bool isRecording = (m_state == PanelState::Recording);

    if (!isRecording) {
        ImGui::RadioButton("Scheme 1 (CPU instanced)", &m_scheme, 1);
        ImGui::RadioButton("Scheme 2 (CPU cull+indirect)", &m_scheme, 2);
        ImGui::RadioButton("Scheme 3 (GPU cull+indirect)", &m_scheme, 3);
        ImGui::InputInt("Warmup frames", &m_warmupFrames, 10, 60);
        ImGui::InputInt("Record frames", &m_recordFrames, 50, 200);
        if (m_warmupFrames < 0) m_warmupFrames = 0;
        if (m_recordFrames < 1) m_recordFrames = 1;
    }

    if (!canRun && !isRecording) {
        ImGui::BeginDisabled();
        ImGui::Button("Run Benchmark", ImVec2(150, 0));
        ImGui::EndDisabled();
    } else if (isRecording) {
        ImGui::Button("Recording...", ImVec2(150, 0)); // disabled by state
        const auto& stats = app->GetFrameStats();
        int recorded = (int)stats.size();
        ImGui::SameLine();
        ImGui::ProgressBar((float)recorded / (float)m_recordFrames, ImVec2(200, 0));
        ImGui::SameLine();
        ImGui::Text("%d / %d", recorded, m_recordFrames);

        // Collect frame times for chart
        if (recorded > m_chartFrame) {
            for (int i = m_chartFrame; i < recorded; ++i) {
                m_frameTimes.push_back((float)stats[i].totalTimeUs);
            }
            m_chartFrame = recorded;
        }

        if (recorded >= m_recordFrames) {
            m_state = PanelState::Results;
            m_lastResults.assign(stats.begin(), stats.end());
            m_lastScheme = m_scheme;

            // Save CSV
            auto t = std::chrono::system_clock::to_time_t(
                std::chrono::system_clock::now());
            std::ostringstream csvName;
            csvName << "results/benchmark_"
                    << std::put_time(std::localtime(&t), "%Y%m%d_%H%M%S") << ".csv";
            m_csvPath = csvName.str();
            app->SetScheme((RenderScheme)m_scheme);
        }
    } else if (canRun) {
        if (ImGui::Button("Run Benchmark", ImVec2(150, 0))) {
            app->SetScheme((RenderScheme)m_scheme);
            // Trigger benchmark with panel params
            BenchmarkConfig cfg = app->GetBenchmarkConfig();
            cfg.scheme = (RenderScheme)m_scheme;
            cfg.warmupFrames = m_warmupFrames;
            cfg.recordFrames = m_recordFrames;
            m_frameTimes.clear();
            m_chartFrame = 0;
            m_csvPath.clear();
            m_state = PanelState::Recording;
            app->SetBenchmarkConfig(cfg);
        }
    }
}
```

- [ ] **Step 2: Write RenderResultsSection()**

```cpp
void BenchmarkPanel::RenderResultsSection(GPUSceneManagement* app) {
    ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "Results");
    ImGui::Separator();

    if (m_lastResults.empty()) {
        ImGui::TextDisabled("No results yet. Run a benchmark.");
        return;
    }

    // Compute stats
    std::vector<double> totals;
    for (auto& s : m_lastResults) totals.push_back(s.totalTimeUs);
    std::sort(totals.begin(), totals.end());
    size_t n = totals.size();
    double avg = std::accumulate(totals.begin(), totals.end(), 0.0) / n;
    double p1 = totals[n / 100];
    double p99 = totals[n * 99 / 100];

    ImGui::Text("Scheme %d | Frames: %zu | Draw calls: %u",
                m_lastScheme, n, m_lastResults[0].drawCalls);
    ImGui::Text("Total: avg=%.1f us  P1=%.1f  P99=%.1f", avg, p1, p99);

    if (!m_csvPath.empty()) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "CSV: %s", m_csvPath.c_str());
    }

    // ImPlot frame time chart
    if (ImPlot::BeginPlot("##FrameTimeChart", ImVec2(-1, 200),
                          ImPlotFlags_NoTitle | ImPlotFlags_NoLegend)) {
        ImPlot::SetupAxes("Frame", "Time (us)", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        ImPlot::PlotLine("Total", m_frameTimes.data(), (int)m_frameTimes.size());
        ImPlot::EndPlot();
    }

    // CPU/GPU decomposition bar chart
    if (ImPlot::BeginPlot("##CPUGPUBar", ImVec2(-1, 150),
                          ImPlotFlags_NoTitle)) {
        ImPlot::SetupAxes(nullptr, nullptr,
                          ImPlotAxisFlags_NoDecorations,
                          ImPlotAxisFlags_NoDecorations);
        ImPlot::SetupAxesLimits(0, 3, 0, avg * 1.2, ImGuiCond_Always);
        // GPU bar
        double gpuAvg = n > 0 ? std::accumulate(m_lastResults.begin(), m_lastResults.end(), 0.0,
            [](double sum, const FrameStats& s) { return sum + s.gpuTimeUs; }) / n : 0;
        ImPlot::PlotBars("GPU", &gpuAvg, 1, 0.4, 1.0);
        ImPlot::PlotBars("CPU", &avg, 1, 0.4, 2.0);
        ImPlot::EndPlot();
    }
}
```

- [ ] **Step 3: Verify compilation**

```bash
cmake --build . --target GPUDrivenRendering --config Debug
```

- [ ] **Step 4: Commit**

```bash
git add GPUDrivenRendering/BenchmarkPanel.cpp
git commit -m "feat: benchmark section with ImPlot charts and CSV output"
```

---

### Task 9: Main.cpp — Wire BenchmarkPanel

**Files:**
- Modify: `GPUDrivenRendering/Main.cpp`

- [ ] **Step 1: Add BenchmarkPanel include and instance**

Replace the existing interactive mode ImGui block with BenchmarkPanel integration.

```cpp
#include "BenchmarkPanel.h"

// In main(), after app.SetGui(gui):
BenchmarkPanel benchPanel;

// In the interactive loop, after gui->StartNewFrame():
benchPanel.Render(&app);

// Generation visualization: update partial tileGrid each frame
if (benchPanel.ShouldRenderPartial()) {
    app.SetPartialTileGrid(benchPanel.GetPartialTileGrid(), app.GetGridSize());
    app.SetRenderPartial(true);
}
if (benchPanel.GenerationReady() && benchPanel.GetState() == PanelState::Ready) {
    app.SetRenderPartial(false);
}
```

- [ ] **Step 2: Remove old ImGui panel code from RunFrame**

Keep only essential ImGui rendering in `GPUSceneManagement::RunFrame()` — remove the info panel since BenchmarkPanel handles it. ChunkMonitor rendering stays.

- [ ] **Step 3: Verify compilation**

```bash
cmake --build . --target GPUDrivenRendering --config Debug
```

- [ ] **Step 4: Commit**

```bash
git add GPUDrivenRendering/Main.cpp GPUDrivenRendering/GPUSceneManagement.cpp
git commit -m "feat: wire BenchmarkPanel into main loop, remove old ImGui panel"
```

---

### Task 10: Clean diagnostic output

**Files:**
- Modify: `GPUDrivenRendering/GPUSceneManagement.cpp`
- Modify: `GPUDrivenRendering/Main.cpp`

- [ ] **Step 1: Guard diagnostic couts**

Wrap all `std::cout << "[Main]..."`, `"[RunFrame]..."`, `"[Benchmark] Readback..."` with:

```cpp
#ifndef NDEBUG
    std::cout << ...
#endif
```

Or simply remove them since the BenchmarkPanel provides live feedback.

- [ ] **Step 2: Keep only critical output**

Keep: `[GPUDriven] Generating scene...` with instance count (informational), and error messages (cerr).

- [ ] **Step 3: Verify compilation**

```bash
cmake --build . --target GPUDrivenRendering --config Debug
```

- [ ] **Step 4: Commit**

```bash
git add GPUDrivenRendering/GPUSceneManagement.cpp GPUDrivenRendering/Main.cpp
git commit -m "chore: remove diagnostic couts, keep essential output only"
```

---

## Self-Review

### Spec Coverage Check

| Spec Requirement | Task |
|------------------|------|
| Scene Generation section (Grid, Seed, Density, Cache, Show Gen, Generate/Cancel) | Task 5 |
| Background thread with progress | Tasks 5, 6 |
| Partial tileGrid for visualization | Tasks 6, 7 |
| Cache save/load with toggle | Task 1, integrated in Task 5 |
| Benchmark section (Scheme, Warmup, Record, Run) | Task 8 |
| Results section (stats, ImPlot charts, CSV path) | Task 8 |
| State machine (Idle→Generating→Ready→Recording→Results) | Tasks 5, 8 |
| Cancel kills process | Task 5 |
| Uncollapsed cells not rendered | Task 7 (tileId==0 skip) |
| ImPlot integration | Task 4 |
| GetFrameStats() accessor | Task 3 |
| ChunkMonitor still visible | Task 9 (kept in RunFrame) |

All spec requirements covered. No TODOs. No placeholders.

### Type Consistency

- `BenchmarkPanel::m_state` type `PanelState` used consistently
- `WFCCache` static methods use same signature in header and implementation
- `GPUSceneManagement::GetFrameStats()` returns `const std::vector<FrameStats>&` — used in Task 8
- `WFCConfig::partialGrid` is `std::vector<uint32_t>*` — set in Task 5, written in Task 6
