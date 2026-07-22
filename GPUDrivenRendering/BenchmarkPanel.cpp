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
#include <numeric>
#include <algorithm>

using namespace NCL::Rendering::Vulkan;

BenchmarkPanel::BenchmarkPanel() { ImPlot::CreateContext(); }

BenchmarkPanel::~BenchmarkPanel() { if (m_genThread.joinable()) m_genThread.join(); ImPlot::DestroyContext(); }

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

void BenchmarkPanel::RenderGenerationSection(GPUSceneManagement* app) {
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Scene Generation");
    ImGui::Separator();

    bool isGenerating = (m_state == PanelState::Generating);
    bool isIdle       = (m_state == PanelState::Idle);

    if (isGenerating) ImGui::BeginDisabled();
	static const uint32_t kGridSizes[] = { 16, 64, 128, 256, 512, 1024, 2048, 4096 };
	static const char* kGridSizeNames[] = { "16", "64", "128", "256", "512", "1024", "2048", "4096" };
	int sizeIdx = 0;
	for (int i = 0; i < IM_ARRAYSIZE(kGridSizes); ++i)
		if (m_genGridSize == kGridSizes[i]) { sizeIdx = i; break; }
	if (ImGui::Combo("Grid Size", &sizeIdx, kGridSizeNames, IM_ARRAYSIZE(kGridSizeNames)))
		m_genGridSize = kGridSizes[sizeIdx];
	if (m_genChunkSize > m_genGridSize) m_genChunkSize = m_genGridSize;
	static const uint32_t kChunkSizes[] = { 4, 8, 16, 32, 64, 128, 256, 512 };
	int chunkIdx = 0;
	for (int i = 0; i < IM_ARRAYSIZE(kChunkSizes); ++i)
		if (m_genChunkSize == kChunkSizes[i]) { chunkIdx = i; break; }
	static const char* kChunkSizeNames[] = { "4", "8", "16", "32", "64", "128", "256", "512" };
	if (ImGui::Combo("Chunk Size", &chunkIdx, kChunkSizeNames, IM_ARRAYSIZE(kChunkSizeNames)))
		m_genChunkSize = kChunkSizes[chunkIdx];
		m_genChunkSize = kChunkSizes[chunkIdx];
	if (m_genChunkSize > m_genGridSize) m_genChunkSize = m_genGridSize;

    ImGui::InputInt("Seed", (int*)&m_genSeed);
    int densityIdx = (m_genDensity == 20) ? 0 : (m_genDensity == 50) ? 1 : 2;
	if (ImGui::Combo("Density", &densityIdx, "20 %%\0 50 %%\0 80 %%\0\0")) {
		m_genDensity = (densityIdx == 0) ? 20u : (densityIdx == 1) ? 50u : 80u;
	}
    if (isGenerating) ImGui::EndDisabled();

    ImGui::Checkbox("Use Cache", &m_useCache);
    ImGui::Checkbox("Show Generation", &m_showGeneration);

    if (isGenerating) {
        if (ImGui::Button("Cancel")) std::exit(0);

        uint32_t progress = m_genProgress.load();
        uint32_t total = (m_totalCells > 0) ? m_totalCells : (m_genGridSize * m_genGridSize);
        float frac = (total > 0) ? (float)progress / (float)total : 0.0f;
        ImGui::ProgressBar(frac, ImVec2(-1.0f, 0.0f));
        ImGui::Text("%u / %u cells (%.1f%%)", progress, total, frac * 100.0f);
    }

	if (isIdle) {
		if (ImGui::Button("Generate")) {
			bool loaded = false;
			if (m_useCache) {
				std::vector<uint32_t> loadedGrid;
				uint32_t loadedSize = 0;
				if (WFCCache::Load(loadedGrid, loadedSize, m_genGridSize, m_genSeed, m_genDensity)) {
					// Cache hit: grid ready immediately. Hand it to RunFrame for a
					// frame-boundary GPU rebuild rather than rebuilding here in the
					// ImGui callback -- a mid-frame rebuild frees resources the
					// current command buffer still references (device-lost -> abort).
					m_partialTileGrid = std::move(loadedGrid);
					m_totalCells = m_genGridSize * m_genGridSize;
					m_genProgress = m_totalCells;
					m_pendingRebuild = true;
					m_state = PanelState::Loading;
					loaded = true;
				}
			}
			if (!loaded) {
				m_totalCells = m_genGridSize * m_genGridSize;
				m_genProgress = 0;
				m_generationReady = false;
				m_state = PanelState::Generating;
				m_genThread = std::thread(&BenchmarkPanel::GenerationThread,
					this, app, m_genGridSize, m_genSeed, m_genDensity);
			}
		}
    }

    if (m_generationReady && m_state == PanelState::Generating) {
        // Background WFC finished. Join the worker and hand the grid to
        // RunFrame for a frame-boundary rebuild -- never rebuild GPU
        // resources from inside this ImGui Render callback (see the
        // PanelState::Loading rationale in BenchmarkPanel.h).
        if (m_genThread.joinable()) m_genThread.join();
        m_pendingRebuild = true;
        m_state = PanelState::Loading;
    }

	if (m_state == PanelState::Ready || m_state == PanelState::Results) {
		ImGui::SameLine();
		if (ImGui::Button("Reset")) {
			m_state = PanelState::Idle;
			m_generationReady = false;
			m_partialTileGrid.clear();
		}
	}

    switch (m_state) {
        case PanelState::Idle:       ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Status: Idle"); break;
        case PanelState::Generating: ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Status: Generating..."); break;
        case PanelState::Loading:    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Status: Loading (building GPU scene)..."); break;
        case PanelState::Ready:      ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Status: Ready"); break;
        case PanelState::Recording:  ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Status: Recording..."); break;
        case PanelState::Results:    ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "Status: Results"); break;
    }
}

void BenchmarkPanel::RenderBenchmarkSection(GPUSceneManagement* app) {
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Benchmark");
    ImGui::Separator();
    bool canRun = (m_state == PanelState::Ready || m_state == PanelState::Results);
    bool isRecording = (m_state == PanelState::Recording);
    if (!isRecording) {
        // Radio clicks hot-switch the live render scheme. SetScheme only writes
        // an enum that RunFrame's scheme switch reads next frame — no buffer or
        // descriptor rebuild — so this is safe to call from the ImGui callback,
        // unlike the scene rebuild path. Independent of the NUM1/2/3 shortcuts.
        bool schemeChanged = false;
        schemeChanged |= ImGui::RadioButton("Scheme 1 (CPU instanced)", &m_scheme, 1);
        schemeChanged |= ImGui::RadioButton("Scheme 2 (CPU cull+indirect)", &m_scheme, 2);
        schemeChanged |= ImGui::RadioButton("Scheme 3 (GPU cull+indirect)", &m_scheme, 3);
        if (schemeChanged) app->SetScheme((RenderScheme)m_scheme);

        // Live confirmation the switch actually took effect: read the ENGINE's
        // active scheme (not the radio) plus the real recorded draw-call count.
        // Draw calls make the switch unambiguous even though the picture looks
        // identical — scheme 1 records one draw per visible chunk (large), while
        // schemes 2/3 record exactly 2 indirect draws.
        uint32_t activeScheme = (uint32_t)app->GetBenchmarkConfig().scheme;
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f),
            "Active: Scheme %u  |  draw calls: %u", activeScheme, app->GetLastDrawCalls());

        ImGui::InputInt("Warmup frames", &m_warmupFrames, 10, 60);
        ImGui::InputInt("Record frames", &m_recordFrames, 50, 200);
        if (m_warmupFrames < 0) m_warmupFrames = 0;
        if (m_recordFrames < 1) m_recordFrames = 1;
    }
    if (!canRun && !isRecording) {
        ImGui::BeginDisabled(true);
        ImGui::Button("Run Benchmark", ImVec2(150, 0));
        ImGui::EndDisabled();
    } else if (isRecording) {
        const auto& stats = app->GetFrameStats();
		int recorded = (int)stats.size();
        ImGui::ProgressBar((float)recorded / (float)m_recordFrames, ImVec2(200, 0));
        ImGui::SameLine(); ImGui::Text("%d / %d", recorded, m_recordFrames);
		if (recorded > (int)m_frameTimes.size())
			m_frameTimes.push_back((float)stats.back().cpuRecordUs);
        if (recorded >= m_recordFrames) {
            m_state = PanelState::Results;
			app->SetBenchmarkEnabled(false);
            m_lastResults.assign(stats.begin(), stats.end());
            m_lastScheme = m_scheme;
            // gpuExecUs is still 0 in this copy - the GPU timestamp readback
            // runs at the end of THIS frame, after the panel renders. Re-copy
            // next frame once IsBenchmarkComplete() reports the readback done.
            m_pendingResultCopy = true;
            auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            std::ostringstream csvName;
            csvName << "results/benchmark_"
                    << std::put_time(std::localtime(&t), "%Y%m%d_%H%M%S") << ".csv";
            m_csvPath = csvName.str();
        }
    } else if (canRun) {
        if (ImGui::Button("Run Benchmark", ImVec2(150, 0))) {
            m_frameTimes.clear();
            m_csvPath.clear();
            m_state = PanelState::Recording;
		app->SetBenchmarkEnabled(true);
            app->ResetBenchmarkState((RenderScheme)m_scheme, m_warmupFrames, m_recordFrames, "");
        }
    }
}

void BenchmarkPanel::RenderResultsSection(GPUSceneManagement* app) {
    ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "Results");
    ImGui::Separator();
    // Deferred re-copy: the completion copy captured gpuExecUs == 0 because the
    // GPU timestamp readback happens at end-of-frame, after the panel renders.
    // Once the app reports the readback done, re-copy so the GPU chart is real,
    // then finalize this run: roll current -> previous and snapshot the new one.
    if (m_pendingResultCopy && app->IsBenchmarkComplete()) {
        const auto& s = app->GetFrameStats();
        if (!s.empty()) m_lastResults.assign(s.begin(), s.end());
        m_pendingResultCopy = false;

        auto statTriple = [&](double FrameStats::* field, double& avg, double& p1, double& p99) {
            std::vector<double> v;
            v.reserve(m_lastResults.size());
            for (auto& r : m_lastResults) v.push_back(r.*field);
            std::sort(v.begin(), v.end());
            size_t m = v.size();
            avg = m ? std::accumulate(v.begin(), v.end(), 0.0) / m : 0.0;
            p1  = m ? v[m / 100] : 0.0;
            p99 = m ? v[m * 99 / 100] : 0.0;
        };
        RunSummary sum;
        sum.valid     = true;
        sum.scheme    = m_lastScheme;
        sum.frames    = m_lastResults.size();
        sum.drawCalls = m_lastResults.empty() ? 0 : m_lastResults[0].drawCalls;
        statTriple(&FrameStats::cpuRecordUs, sum.cpuAvg, sum.cpuP1, sum.cpuP99);
        statTriple(&FrameStats::gpuExecUs,   sum.gpuAvg, sum.gpuP1, sum.gpuP99);
        m_prevSummary = m_curSummary;   // previous finalized run (may be invalid)
        m_curSummary  = sum;
    }

    if (!m_curSummary.valid) {
        ImGui::TextDisabled("No results yet. Run a benchmark.");
        return;
    }

    const RunSummary& cur  = m_curSummary;
    const RunSummary& prev = m_prevSummary;

    ImGui::Text("This run:  Scheme %d | Frames: %zu | Draw calls: %u",
                cur.scheme, cur.frames, cur.drawCalls);
    ImGui::Text("  CPU record: avg=%.1f us  P1=%.1f  P99=%.1f", cur.cpuAvg, cur.cpuP1, cur.cpuP99);
    ImGui::Text("  GPU exec:   avg=%.1f us  P1=%.1f  P99=%.1f", cur.gpuAvg, cur.gpuP1, cur.gpuP99);
    if (prev.valid) {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
            "Prev run:  Scheme %d | CPU avg=%.1f us | GPU avg=%.1f us",
            prev.scheme, prev.cpuAvg, prev.gpuAvg);
    } else {
        ImGui::TextDisabled("Prev run:  (run another benchmark to compare)");
    }
    if (!m_csvPath.empty())
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "CSV: %s", m_csvPath.c_str());

    // Per-frame CPU-record timeline for the current run.
    if (ImPlot::BeginPlot("CPU record per frame", ImVec2(-1, 180),
                          ImPlotFlags_NoLegend)) {
        ImPlot::SetupAxes("frame", "CPU record (us)",
                          ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        ImPlot::PlotLine("cpu_record", m_frameTimes.data(), (int)m_frameTimes.size());
        ImPlot::EndPlot();
    }

    // Cross-run comparison: previous vs current, grouped by metric. CPU and GPU
    // are independent parallel timelines with very different magnitudes, so each
    // gets its own auto-scaled, labeled chart. Legend labels carry the scheme so
    // "scheme 1 vs scheme 3" is readable directly off the bars.
    static const double kPositions[3] = { 0.0, 1.0, 2.0 };
    static const char*  kLabels[3]    = { "avg", "P1", "P99" };

    // PlotBarGroups (ImPlot 0.17) reads values item-major: values[item*groups + g].
    // items = {prev, cur}; groups = {avg, P1, P99}. Prev bars are zero/hidden
    // until a second run exists.
    char prevId[32], curId[32];
    snprintf(prevId, sizeof(prevId), "prev (S%d)", prev.valid ? prev.scheme : 0);
    snprintf(curId,  sizeof(curId),  "this (S%d)", cur.scheme);
    const char* barIds[2] = { prevId, curId };

    const double cpuGroups[6] = {
        prev.valid ? prev.cpuAvg : 0.0, prev.valid ? prev.cpuP1 : 0.0, prev.valid ? prev.cpuP99 : 0.0,
        cur.cpuAvg, cur.cpuP1, cur.cpuP99 };
    const double gpuGroups[6] = {
        prev.valid ? prev.gpuAvg : 0.0, prev.valid ? prev.gpuP1 : 0.0, prev.valid ? prev.gpuP99 : 0.0,
        cur.gpuAvg, cur.gpuP1, cur.gpuP99 };

    float half = ImGui::GetContentRegionAvail().x * 0.5f - 4.0f;
    if (ImPlot::BeginPlot("CPU record", ImVec2(half, 190))) {
        ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
        ImPlot::SetupAxes(nullptr, "us", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        ImPlot::SetupAxisTicks(ImAxis_X1, kPositions, 3, kLabels);
        ImPlot::SetupAxisLimits(ImAxis_X1, -0.5, 2.5, ImGuiCond_Always);
        ImPlot::PlotBarGroups(barIds, cpuGroups, 2, 3, 0.67);
        ImPlot::EndPlot();
    }
    ImGui::SameLine();
    if (ImPlot::BeginPlot("GPU exec", ImVec2(half, 190))) {
        ImPlot::SetupLegend(ImPlotLocation_NorthEast, ImPlotLegendFlags_Outside);
        ImPlot::SetupAxes(nullptr, "us", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        ImPlot::SetupAxisTicks(ImAxis_X1, kPositions, 3, kLabels);
        ImPlot::SetupAxisLimits(ImAxis_X1, -0.5, 2.5, ImGuiCond_Always);
        ImPlot::PlotBarGroups(barIds, gpuGroups, 2, 3, 0.67);
        ImPlot::EndPlot();
    }
}

void BenchmarkPanel::GenerationThread(GPUSceneManagement* app, uint32_t gridSize,
                                      uint32_t seed, uint32_t density) {
    WFCGenerator gen;
    WFCConfig cfg;
    cfg.gridSize = gridSize;
    cfg.seed = seed;
    cfg.emptyWeight = 5.0f;
    cfg.otherWeight = 5.0f;
    switch (density) {
        case 20: cfg.emptyWeight = 8.0f; cfg.otherWeight = 2.0f; break;
        case 80: cfg.emptyWeight = 2.0f; cfg.otherWeight = 10.0f; break;
    }
    cfg.onProgress = [&](uint32_t collapsed, uint32_t total) { m_genProgress = collapsed; };
    cfg.partialGrid = &m_partialTileGrid;
    cfg.gridMutex = &m_tileGridMutex;
    auto tileGrid = gen.Generate(cfg);
    { std::lock_guard<std::mutex> lock(m_tileGridMutex); m_partialTileGrid = std::move(tileGrid); }
    m_genProgress = gridSize * gridSize;
    m_generationReady = true;
    if (m_useCache) { WFCCache::Save(m_partialTileGrid, gridSize, seed, density); }
}
