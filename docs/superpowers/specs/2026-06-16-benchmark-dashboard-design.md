# Benchmark Dashboard — Design Spec

**Date:** 2026-06-16
**Branch:** WFCFix
**Goal:** Interactive ImGui benchmark panel for live demo, replacing CLI batch workflow.

---

## 1. ImGui Panel Layout

### Scene Generation section

| Control | Type | Values | Default |
|---------|------|--------|---------|
| Grid Size | InputInt | 16–2048 | 512 |
| Seed | InputInt | any | 42 |
| Density | Combo | 20/50/80 | 50 |
| Use Cache | Checkbox | on/off | on |
| **Show Generation** | Checkbox | on/off | on |
| **[Generate]** | Button | → **[Cancel]** during generation | — |
| Status text | — | "Idle" / "Generating 45%..." / "Ready" | — |

- [Generate] → starts worker thread. Button text changes to **[Cancel]** while GENERATING.
- **[Cancel]** → calls `std::exit(0)` immediately (kills whole process).
- **Show Generation**: when on, uncollapsed cells (tileId==0) are simply NOT rendered — scene fills in as generation progresses. When off, show nothing until generation complete.
- **Use Cache**: when on and cache file exists, skip generation entirely, load binary tileGrid from `cache/`.

### Benchmark section

| Control | Type | Values | Default |
|---------|------|--------|---------|
| Scheme | Radio | 1/2/3 | 3 |
| Warmup | InputInt | ≥0 | 60 |
| Record | InputInt | ≥1 | 200 |
| **[Run Benchmark]** | Button | disabled during recording | — |
| Status text | — | "Recording 95/200..." / "Done" | — |
| **CSV saved to** | ReadOnly Text | auto-generated path | — |

- [Run Benchmark] enabled only in READY or RESULTS state. Disabled during GENERATING and RECORDING.
- CSV auto-saved to `results/benchmark_{YYYYMMDD}_{HHMMSS}.csv` on completion.
- Path displayed in panel after benchmark completes.
- Warmup/Record editable in all states except RECORDING.

### Results section

- Summary stats: total, CPU, GPU — avg, P1, P99
- Draw call count, visible chunks
- CPU/GPU time decomposition: ImPlot horizontal bar chart
- Frame time line chart: ImPlot real-time plot (200 data points, auto-scaling Y axis)
- [Reset Results] clears stats and chart data

### Chunk Monitor section

- N×N colored matrix: green=visible, red=culled, violet=empty
- Updates live during benchmark recording
- Visible during READY and RECORDING states

---

## 2. State Machine

```
IDLE ──[Generate]──→ GENERATING ──[done]──→ READY ──[Run Benchmark]──→ RECORDING ──[done]──→ RESULTS
                       │    [Cancel] → exit(0)
                       │
                       └── Show Gen=off: render nothing until done
                       └── Show Gen=on:  render collapsed cells only
```

- **IDLE**: No scene. Empty viewport.
- **GENERATING**: Worker thread runs WFC. Show Gen on → render collapsed cells; off → empty viewport. [Generate] button becomes **[Cancel]**.
- **READY**: Scene rendered. Benchmark controls enabled. ChunkMonitor shows full scene.
- **RECORDING**: Warmup + record frames. Real-time ImPlot chart updates. ChunkMonitor live. [Run Benchmark] disabled.
- **RESULTS**: Recording complete. Stats + charts frozen. CSV path shown. Can edit params and [Run Benchmark] again.

---

## 3. Background Generation Thread

```
std::thread m_genThread
std::mutex m_tileGridMutex
std::atomic<uint32_t> m_genProgress   // 0–total cells
std::atomic<bool> m_generationReady

Worker:  WFC::Generate() with progress callback
         Every 500 collapsed cells:
           lock → write tileGrid row → unlock
           m_genProgress += 500

Render:  lock → copy tileGrid → unlock → render
         tileId==0 → skip (don't render)
         tileId>0 → render normally

Cancel:  std::exit(0) — kill entire process immediately
```

---

## 4. Cache Layer

- Path: `cache/wfc_{gridSize}_{seed}_{density}.bin`
- Format: raw binary: [uint32_t gridSize][uint32_t[] tileGrid]
- Save after first generation
- Load if file exists AND Use Cache checkbox is on

---

## 5. Benchmark Recording (In-Panel)

- Uses existing BeginMeasurement/EndMeasurement infrastructure
- Warmup + Record: configurable from panel
- Per-frame data: cpu_us, gpu_us, total_us, draw_calls, visible_instances
- Real-time ImPlot line chart: frame time per frame, auto-scaling Y axis
- On completion:
  - Auto-save CSV to `results/benchmark_YYYYMMDD_HHMMSS.csv`
  - CSV path displayed in panel
  - Stats frozen in Results section
- `GPUSceneManagement::GetFrameStats()` exposes recorded data to panel

---

## 6. File Changes

| Action | File | Purpose |
|--------|------|---------|
| NEW | BenchmarkPanel.h/.cpp | ImGui panel, state machine, worker thread, cancel |
| NEW | WFCCache.h/.cpp | Binary cache save/load (`cache/` directory) |
| MODIFY | GPUSceneManagement.h/.cpp | `GetFrameStats()` accessor, tileGrid exposure, partial rendering |
| MODIFY | WFCGenerator.h/.cpp | Progress callback per N collapsed cells |
| MODIFY | Main.cpp | Create BenchmarkPanel, wire into main loop |
| MODIFY | CMakeLists.txt | Add ImPlot (implot.h + implot.cpp + implot_items.cpp) |

### ImPlot Integration

- Source: `imgui/implot.h`, `imgui/implot.cpp`, `imgui/implot_items.cpp`
- Single-file MIT library, compatible with ImGui docking branch
- Functions used: `ImPlot::BeginPlot/EndPlot`, `ImPlot::PlotLine`, `ImPlot::PlotBars`

---

## 7. Demo Flow

1. Launch → panel shows default params (512, seed=42, density=50, cache=on)
2. Click [Generate] → cache hit → scene appears instantly (READY)
3. Click [Run Benchmark] → 260 frames fly by → ImPlot charts populate → CSV saved, path shown
4. Switch scheme to 1 or 2 → [Run Benchmark] again → compare results side by side
5. Disable cache, change seed → [Generate] → watch cells fill in as WFC runs (2-5 min)
6. If generation stalls → [Cancel] → process exits cleanly
