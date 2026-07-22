# Dissertation Demonstration Video: Bilingual Script and Shot List

Target duration: approximately 2 minutes 45 seconds. Record at 1920×1080 and 30 fps. The English narration contains 309 words, corresponding to roughly 112 words per minute and leaving room for visible panel interactions.

The final voice-over will use the Minimax Speech-2.8-hd model.

## English narration

### 0:00–0:10 — Research question

This project evaluates how CPU and GPU work allocation affects the rendering of procedurally generated modular scenes in Vulkan.

### 0:10–0:35 — Scene generation through the ImGui panel

The demonstration begins in an ImGui benchmark panel, which controls scene generation and measurement. I select grid size 128, chunk size 8, tile-weight preset 50, and seed 42, then generate the scene. These values define a repeatable WFC workload and the chunk granularity used for culling and command generation.

### 0:35–1:15 — Three-path benchmark at grid size 128

Using the same scene and fixed camera, I select S1, S2, and S3 in turn and run the benchmark for each path. S1 performs chunk-frustum culling on the CPU and records direct indexed draws. S2 uses the same CPU culling, then submits host-populated indirect commands through multi-draw indirect. S3 moves chunk culling and indirect-record generation to a compute shader. The panel shows benchmark progress and reports CPU recording time, GPU timestamp span, draw calls, and visible chunks.

### 1:15–1:45 — Larger grid demonstration

I then change the grid size to 2048 and generate a much larger scene. For this second demonstration, I run S3 and then S1; S2 is not included in this clip. These short panel runs demonstrate that both paths execute at the larger scale. They do not replace the controlled, configuration-matched dissertation dataset.

### 1:45–2:25 — Main findings

Across that dataset, grid size 128 was the first tested scale where S2 recorded lower CPU command-recording time than S1 in every matched configuration, although its measured GPU span was usually longer. The S2-to-S3 comparison showed a further CPU-GPU trade-off. With fine chunk sizes, the fixed two-records-per-chunk layout creates many indirect records, and S3's additional GPU span becomes more pronounced. Rendering-path selection therefore needs to consider both scale and chunk granularity.

### 2:25–2:45 — Contribution

The project contributes a controlled Vulkan framework for comparing three rendering paths over reproducible procedural scenes, together with an ImGui benchmark interface, automated data collection, and generated-content records. The dissertation and source code provide the full methodology, results, and limitations.

## 中文参考稿

### 0:00–0:10 — 研究问题

本项目评估在 Vulkan 中将工作分配给 CPU 与 GPU 的不同方式，会如何影响程序化生成模块场景的渲染性能。

### 0:10–0:35 — 通过 ImGui panel 生成场景

演示从 ImGui benchmark panel 开始，该 panel 用于控制场景生成和测量。我选择 grid size 128、chunk size 8、tile-weight preset 50 和 seed 42，随后生成场景。这些参数定义了可重复的 WFC workload，也决定了 culling 和 command generation 使用的 chunk granularity。

### 0:35–1:15 — Grid size 128 下的三路径 benchmark

在同一场景和固定 camera 下，我依次选择 S1、S2、S3，并分别运行 benchmark。S1 在 CPU 上执行 chunk-frustum culling 并记录 direct indexed draws。S2 执行相同的 CPU culling，随后通过 multi-draw indirect 提交由 host 填充的 indirect commands。S3 将 chunk culling 和 indirect-record generation 移至 compute shader。panel 显示 benchmark progress，并报告 CPU recording time、GPU timestamp span、draw calls 和 visible chunks。

### 1:15–1:45 — 更大 grid 的运行演示

随后，我把 grid size 改为 2048 并生成规模更大的场景。在第二段演示中，我先运行 S3，再运行 S1；这一片段没有运行 S2。这些短时间的 panel runs 用于展示两条路径可以在更大规模下执行，论文结论仍以受控且 configuration-matched 的数据集为依据。

### 1:45–2:25 — 主要结果

在该数据集中，grid size 128 是首个在每组匹配 configuration 中都由 S2 取得较低 CPU command-recording time 的测试规模，不过其 measured GPU span 通常较长。S2 与 S3 的比较显示了进一步的 CPU-GPU 权衡。使用较细 chunk size 时，fixed two-records-per-chunk layout 会生成大量 indirect records，S3 的额外 GPU span 也会更明显。因此，rendering-path selection 需要同时考虑 scale 与 chunk granularity。

### 2:25–2:45 — 项目贡献

本项目提供一套受控 Vulkan framework，可在可复现的 procedural scenes 上比较三条 rendering paths，并配套 ImGui benchmark interface、automated data collection 和 generated-content records。论文与源代码给出了完整的方法、结果和限制。

## Shot list and on-screen text

| Time | Screen action | On-screen text |
|---|---|---|
| 0:00–0:10 | Begin with a two-second title card, then cut to a clean full-screen scene view. | `Evaluating CPU- and GPU-Driven Rendering Paths for WFC-Generated Modular Scenes in Vulkan`<br>`Jianran Yu · Newcastle University` |
| 0:10–0:35 | In the ImGui panel, show grid 128, chunk 8, tile-weight preset 50, and seed 42. Click `Generate`, retain the progress/ready transition, and show the resulting scene. | `ImGui benchmark panel`<br>`Grid 128 · Chunk 8 · Preset 50 · Seed 42` |
| 0:35–1:15 | Keep the generated scene and camera fixed. Select S1, S2, and S3 in turn in the panel and run one short benchmark for each. Retain the beginning, progress, and result state of every run; shorten inactive waits with cuts. | `S1 — CPU culling + direct draws`<br>`S2 — CPU culling + multi-draw indirect`<br>`S3 — compute culling + compute-generated indirect records` |
| 1:15–1:45 | Change only grid size to 2048, generate the larger scene, then run S3 followed by S1. Do not add an S2 result to this clip. | `Grid 2048 · S3 then S1`<br>`Operational demonstration; formal conclusions use the full dataset` |
| 1:45–2:25 | Show a readable crop of Figure 5.1, highlighting grid 128, followed by Figure 5.2, highlighting the fine-chunk S3 GPU span. Keep each figure still long enough to read its axes. | `S2: lower CPU recording time from the grid-128 transition`<br>`S3: chunk-sensitive CPU/GPU trade-off` |
| 2:25–2:45 | Return to the running scene, then fade to a closing card. | `Controlled paths · Reproducible scenes · Automated measurements`<br>`Dissertation and source code accompany this video` |

## Commands to record

Run from the `VulkanTutorials` repository root.

Interactive demonstration:

```powershell
.\build\GPUDrivenRendering\Release\GPUDrivenRendering.exe `
  -gridsize 128 -chunksize 8 -density 50 -seed 42 -scheme 1
```

Hold `Alt` to expose the cursor for the ImGui panel. Use the panel's `Generate` and `Run Benchmark` controls for the recorded workflow, and use `Esc` to exit after capturing both grid sizes. The CLI smoke-benchmark command is not part of this video.

## Recording and editing checklist

- Record the application, terminal, figures, and title cards as separate clips.
- At grid size 128, use chunk size 8, tile-weight preset 50, and seed 42; run S1, S2, and S3 through the ImGui panel.
- Change grid size to 2048, regenerate the scene, and run S3 followed by S1; do not imply that this clip contains an S2 run.
- Keep the application camera fixed within each configuration while switching rendering paths.
- Remove inactive generation and benchmark waiting time with direct cuts while retaining each run's startup, progress, and result state.
- Import `../Figures/fig5_1_cpu_record_bar.png` and `../Figures/fig5_2_content_sensitivity_bar.png` directly into the editor; avoid screen-capturing them from a PDF viewer.
- Crop the application capture so unrelated windows, notifications, and account details are absent.
- Generate the narration with the Minimax Speech-2.8-hd model after the picture edit so pauses can follow the visible actions.
- Export the film as 1920×1080 H.264 video with AAC audio in an MP4 container.
- Upload with access settings that allow assessors to open the link without requesting permission.
- Do not rebuild the submission PDFs until the hosted film link is available.
- Add the hosted film link immediately below the dissertation title in both PDF variants, as required by the project brief.
- Confirm that the generative-AI declaration names the Minimax Speech-2.8-hd model before building the PDFs.
