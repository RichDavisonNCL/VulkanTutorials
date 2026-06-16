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
    ImGui::InputInt("Grid Size", (int*)&m_genGridSize);
    ImGui::InputInt("Seed", (int*)&m_genSeed);
    ImGui::Combo("Density", (int*)&m_genDensity, "20 %%\0 50 %%\0 80 %%\0\0");
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
            if (m_useCache) {
                std::vector<uint32_t> loadedGrid;
                uint32_t loadedSize = 0;
                if (WFCCache::Load(loadedGrid, loadedSize, m_genGridSize, m_genSeed, m_genDensity)) {
                    m_partialTileGrid = std::move(loadedGrid);
                    m_totalCells = m_genGridSize * m_genGridSize;
                    m_genProgress = m_totalCells;
                    app->GenerateScene(m_partialTileGrid);
                    app->CreateBuffers();
                    app->CreateDescriptorSets();
                    app->CreateQueryPool();
                    m_state = PanelState::Ready;
                } else {
                    m_totalCells = m_genGridSize * m_genGridSize;
                    m_genProgress = 0;
                    m_generationReady = false;
                    m_state = PanelState::Generating;
                    m_genThread = std::thread(&BenchmarkPanel::GenerationThread,
                        this, app, m_genGridSize, m_genSeed, m_genDensity);
                }
            } else {
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
        app->GenerateScene(m_partialTileGrid);
        app->CreateBuffers();
        app->CreateDescriptorSets();
        app->CreateQueryPool();
        m_state = PanelState::Ready;
    }

    switch (m_state) {
        case PanelState::Idle:       ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Status: Idle"); break;
        case PanelState::Generating: ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Status: Generating..."); break;
        case PanelState::Ready:      ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Status: Ready"); break;
        case PanelState::Recording:  ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Status: Recording..."); break;
        case PanelState::Results:    ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "Status: Results"); break;
    }
}

void BenchmarkPanel::RenderBenchmarkSection(GPUSceneManagement*) {
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "Benchmark");
    ImGui::Separator();
    ImGui::TextDisabled("(Task 8)");
}

void BenchmarkPanel::RenderResultsSection(GPUSceneManagement*) {
    ImGui::TextColored(ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "Results");
    ImGui::Separator();
    ImGui::TextDisabled("(Task 8)");
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
        case 50: cfg.emptyWeight = 5.0f; cfg.otherWeight = 5.0f; break;
        case 80: cfg.emptyWeight = 2.0f; cfg.otherWeight = 10.0f; break;
    }
    cfg.onProgress = [&](uint32_t collapsed, uint32_t total) { m_genProgress = collapsed; };
    auto tileGrid = gen.Generate(cfg);
    { std::lock_guard<std::mutex> lock(m_tileGridMutex); m_partialTileGrid = std::move(tileGrid); }
    m_genProgress = gridSize * gridSize;
    m_generationReady = true;
    if (m_useCache) { WFCCache::Save(m_partialTileGrid, gridSize, seed, density); }
}
