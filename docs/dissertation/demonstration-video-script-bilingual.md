# Dissertation Demonstration Video: Bilingual Script and Shot List

Target duration: approximately 2 minutes 30 seconds. Record at 1920×1080 and 30 fps. The English narration contains 317 words, corresponding to roughly 127 words per minute.

The final voice-over will use the Minimax Speech-2.8-hd model.

## English narration

### 0:00–0:10 — Research question

This project evaluates how CPU and GPU work allocation affects the rendering of procedurally generated modular scenes in Vulkan.

### 0:10–0:35 — Scene generation

The scene is created by a seeded, simplified Wave Function Collapse generator. Here I use a grid size of 128, a chunk size of 8, tile-weight preset 50, and seed 42. The seed and preset determine the generated grid, while chunk size controls the granularity used for culling and command generation.

### 0:35–1:10 — Three rendering paths

All three rendering paths use the same generated scene, meshes, camera, and chunk metadata. In S1, the CPU performs chunk-frustum culling and records direct indexed draw commands for visible chunks. In S2, the CPU performs the same culling, then populates a fixed indirect-command buffer and submits the scene with multi-draw indirect. In S3, a compute shader performs chunk culling and generates the indirect records on the GPU. Switching paths leaves the rendered output consistent while moving work between the CPU and GPU.

### 1:10–1:40 — Benchmark automation

The artifact also provides an automated benchmark mode. This demonstration records ten warm-up frames and thirty measured frames for one configuration. Its CSV output stores configuration and build metadata, followed by per-frame CPU recording time, GPU timestamp span, draw calls, and visible chunks. This short run confirms the measurement pipeline; the dissertation results come from the larger controlled dataset.

### 1:40–2:15 — Main findings

Across that dataset, grid size 128 was the first tested scale where S2 recorded lower CPU command-recording time than S1 in every matched configuration, although its measured GPU span was usually longer. The S2-to-S3 comparison showed a further CPU-GPU trade-off. With fine chunk sizes, the fixed two-records-per-chunk layout creates many indirect records, and S3's additional GPU span becomes more pronounced. Rendering-path selection therefore needs to consider both scale and chunk granularity.

### 2:15–2:30 — Contribution

The project contributes a controlled Vulkan framework for comparing three rendering paths over reproducible procedural scenes, together with benchmark automation and generated-content records. The dissertation and source code provide the full methodology, results, and limitations.

## 中文参考稿

### 0:00–0:10 — 研究问题

本项目评估在 Vulkan 中将工作分配给 CPU 与 GPU 的不同方式，会如何影响程序化生成模块场景的渲染性能。

### 0:10–0:35 — 场景生成

该场景由带固定随机种子的简化 Wave Function Collapse 生成器创建。这里使用 grid size 128、chunk size 8、tile-weight preset 50 和 seed 42。seed 与 preset 决定生成的网格内容，chunk size 控制 culling 与 command generation 的粒度。

### 0:35–1:10 — 三条渲染路径

三条渲染路径使用同一生成场景、meshes、camera 和 chunk metadata。在 S1 中，CPU 执行 chunk-frustum culling，并为可见 chunks 记录 direct indexed draw commands。在 S2 中，CPU 执行相同的 culling，随后填充固定的 indirect-command buffer，并通过 multi-draw indirect 提交场景。在 S3 中，compute shader 执行 chunk culling，并在 GPU 上生成 indirect records。切换路径时，rendered output 保持一致，同时 CPU 与 GPU 之间的工作分配发生变化。

### 1:10–1:40 — Benchmark 自动化

项目还提供 automated benchmark mode。本次演示针对一个 configuration 记录 10 个 warm-up frames 和 30 个 measured frames。CSV output 保存 configuration 与 build metadata，随后记录每帧的 CPU recording time、GPU timestamp span、draw calls 和 visible chunks。这次短运行用于确认 measurement pipeline，论文结果来自规模更大的受控数据集。

### 1:40–2:15 — 主要结果

在该数据集中，grid size 128 是首个在每组匹配 configuration 中都由 S2 取得较低 CPU command-recording time 的测试规模，不过其 measured GPU span 通常较长。S2 与 S3 的比较显示了进一步的 CPU-GPU 权衡。使用较细 chunk size 时，fixed two-records-per-chunk layout 会生成大量 indirect records，S3 的额外 GPU span 也会更明显。因此，rendering-path selection 需要同时考虑 scale 与 chunk granularity。

### 2:15–2:30 — 项目贡献

本项目提供一套受控 Vulkan framework，可在可复现的 procedural scenes 上比较三条 rendering paths，并配套 benchmark automation 和 generated-content records。论文与源代码给出了完整的方法、结果和限制。

## Shot list and on-screen text

| Time | Screen action | On-screen text |
|---|---|---|
| 0:00–0:10 | Begin with a two-second title card, then cut to a clean full-screen scene view. | `Evaluating CPU- and GPU-Driven Rendering Paths for WFC-Generated Modular Scenes in Vulkan`<br>`Jianran Yu · Newcastle University` |
| 0:10–0:35 | Show the interactive launch command. In the Benchmark Dashboard, display grid 128, chunk 8, density 50, and seed 42. To capture an actual generation pass without deleting cache data, uncheck `Use Cache`, click `Generate`, and retain the progress/ready transition plus the resulting scene. | `Grid 128 · Chunk 8 · Preset 50 · Seed 42`<br>`Seeded simplified WFC workload` |
| 0:35–1:10 | Keep the camera fixed. Press `1`, `2`, and `3`, allowing approximately eight seconds of footage for each path. Add the path labels during editing because keyboard switching does not update the dashboard radio selection. | `S1 — CPU culling + direct draws`<br>`S2 — CPU culling + multi-draw indirect`<br>`S3 — compute culling + compute-generated indirect records` |
| 1:10–1:40 | Cut to the terminal. Show the complete smoke command, a short portion of execution, and the first metadata and frame rows from `video_demo_scheme2.csv`. Add a small disclosure line throughout this shot. | `Artifact demonstration run: 10 warm-up + 30 measured frames`<br>`Formal results use the dissertation dataset` |
| 1:40–2:15 | Show a readable crop of Figure 5.1, highlighting grid 128, followed by Figure 5.2, highlighting the fine-chunk S3 GPU span. Keep each figure still long enough to read its axes. | `S2: lower CPU recording time from the grid-128 transition`<br>`S3: chunk-sensitive CPU/GPU trade-off` |
| 2:15–2:30 | Return to the running scene, then fade to a closing card. | `Controlled paths · Reproducible scenes · Automated measurements`<br>`Dissertation and source code accompany this video` |

## Commands to record

Run from the `VulkanTutorials` repository root.

Interactive demonstration:

```powershell
.\build\GPUDrivenRendering\Release\GPUDrivenRendering.exe `
  -gridsize 128 -chunksize 8 -density 50 -seed 42 -scheme 1
```

Hold `Alt` to expose the cursor for the Benchmark Dashboard. Use `Esc` to exit after recording the path-switching clip.

Smoke benchmark:

```powershell
.\build\GPUDrivenRendering\Release\GPUDrivenRendering.exe `
  -benchmark -gridsize 128 -chunksize 8 -density 50 -scheme 2 -seed 42 `
  -warmupframes 10 -recordframes 30 -output video_demo_scheme2.csv
```

Show a compact preview of the resulting file:

```powershell
Get-Content .\video_demo_scheme2.csv | Select-Object -First 18
```

## Recording and editing checklist

- Record the application, terminal, figures, and title cards as separate clips.
- Keep the application camera fixed while switching between S1, S2, and S3.
- Capture one genuine Generate-button interaction with `Use Cache` disabled; do not delete or overwrite existing cache files for the recording.
- Remove inactive waiting time with direct cuts while retaining benchmark startup and completion.
- Import `../Figures/fig5_1_cpu_record_bar.png` and `../Figures/fig5_2_content_sensitivity_bar.png` directly into the editor; avoid screen-capturing them from a PDF viewer.
- Crop terminal output so personal paths, unrelated windows, notifications, and account details are absent.
- Generate the narration with the Minimax Speech-2.8-hd model after the picture edit so pauses can follow the visible actions.
- Export the film as 1920×1080 H.264 video with AAC audio in an MP4 container.
- Upload with access settings that allow assessors to open the link without requesting permission.
- Do not rebuild the submission PDFs until the hosted film link is available.
- Add the hosted film link immediately below the dissertation title in both PDF variants, as required by the project brief.
- Confirm that the generative-AI declaration names the Minimax Speech-2.8-hd model before building the PDFs.
