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

BenchmarkPanel::BenchmarkPanel() {}

BenchmarkPanel::~BenchmarkPanel() { if (m_genThread.joinable()) m_genThread.join(); }

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
					m_partialTileGrid = std::move(loadedGrid);
					app->SetSceneParams(m_genGridSize, m_genChunkSize, m_genSeed, m_genDensity);
					app->GenerateScene(m_partialTileGrid);
					app->CreateBuffers();
					app->CreateDescriptorSets();
					app->CreateQueryPool();
					m_totalCells = m_genGridSize * m_genGridSize;
					m_genProgress = m_totalCells;
					m_state = PanelState::Ready;
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
        if (m_genThread.joinable()) m_genThread.join();
		app->SetSceneParams(m_genGridSize, m_genChunkSize, m_genSeed, m_genDensity);
        app->GenerateScene(m_partialTileGrid);
        app->CreateBuffers();
        app->CreateDescriptorSets();
        app->CreateQueryPool();
        m_state = PanelState::Ready;
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
        ImGui::RadioButton("Scheme 1 (CPU instanced)", &m_scheme, 1);
        ImGui::RadioButton("Scheme 2 (CPU cull+indirect)", &m_scheme, 2);
        ImGui::RadioButton("Scheme 3 (GPU cull+indirect)", &m_scheme, 3);
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
			m_frameTimes.push_back((float)stats.back().totalTimeUs);
        if (recorded >= m_recordFrames) {
            m_state = PanelState::Results;
			app->SetBenchmarkEnabled(false);
            m_lastResults.assign(stats.begin(), stats.end());
            m_lastScheme = m_scheme;
            auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            std::ostringstream csvName;
            csvName << "results/benchmark_"
                    << std::put_time(std::localtime(&t), "%Y%m%d_%H%M%S") << ".csv";
            m_csvPath = csvName.str();
        }
    } else if (canRun) {
        if (ImGui::Button("Run Benchmark", ImVec2(150, 0))) {
            app->SetScheme((RenderScheme)m_scheme);
            BenchmarkConfig cfg = app->GetBenchmarkConfig();
            cfg.scheme = (RenderScheme)m_scheme;
		cfg.chunkSize = m_genChunkSize;
            cfg.warmupFrames = m_warmupFrames;
            cfg.recordFrames = m_recordFrames;
            m_frameTimes.clear();
            m_csvPath.clear();
            m_state = PanelState::Recording;
		app->SetBenchmarkEnabled(true);
            app->SetBenchmarkConfig(cfg);
        }
    }
}

void BenchmarkPanel::RenderResultsSection(GPUSceneManagement* app) {
    ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "Results");
    ImGui::Separator();
    if (m_lastResults.empty()) {
        ImGui::TextDisabled("No results yet. Run a benchmark.");
        return;
    }
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
    if (!m_csvPath.empty())
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "CSV: %s", m_csvPath.c_str());

    if (ImPlot::BeginPlot("##FrameTimeChart", ImVec2(-1, 200),
                          ImPlotFlags_NoTitle | ImPlotFlags_NoLegend)) {
        ImPlot::SetupAxes("Frame", "us", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
        ImPlot::PlotLine("Total", m_frameTimes.data(), (int)m_frameTimes.size());
        ImPlot::EndPlot();
    }

    if (ImPlot::BeginPlot("##CPUGPUBar", ImVec2(-1, 150), ImPlotFlags_NoTitle)) {
        double gpuAvg = 0.0;
        if (n > 0) {
            double sum = 0.0;
            for (auto& s : m_lastResults) sum += s.gpuTimeUs;
            gpuAvg = sum / n;
        }
        ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
        ImPlot::SetupAxesLimits(0, 3, 0, avg * 1.2, ImGuiCond_Always);
        ImPlot::PlotBars("GPU", &gpuAvg, 1, 0.4, 1.0);
        double cpuAvg = avg - gpuAvg;
        ImPlot::PlotBars("CPU", &cpuAvg, 1, 0.4, 2.0);
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
