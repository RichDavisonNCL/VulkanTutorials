# Claim ledger

This ledger locks the direct observations used by the dissertation. Later work must update this document before changing a numerical claim, its evidence, or its limitation.

| ID | Manuscript subsection | Figure ID | Exact direct-observation sentence | Evidence and exact filter | Limitation present in subsection |
|---|---|---|---|---|---|
| C1 | 5.2 S1→S2 CPU Preparation-and-Command-Recording Time | F5.1 | “在这 135 对匹配配置中，S1/S2 的 `cpu_record_avg` 比值为 1.74–9.94；`cpu_record_avg` 表示项目定义的 CPU 准备与命令录制时间均值。” | `results/_aggregate.csv`; `update=0`, `grid>=256`; match `grid`, `chunk`, `density`, `seed`; n=135 pairs | Present: complete S1/S2 rendering paths are compared; mapping, ten-field writes, barrier recording, and direct/MDI command recording differ, so no isolated MDI attribution is made. |
| C2 | 5.3 CPU–GPU Timing Trade-off in the 236.2ms Stress-Test Configuration | F5.2 | “S1、S2、S3 的 CPU 准备与命令录制时间分别为 44,540.6 µs、7,422.7 µs 和 44.745 µs；对应的 GPU 时间戳跨度分别为 38,723.6 µs、44,730.5 µs 和 236,173 µs。” | `results/_aggregate.csv`; `grid=4096`, `chunk=4`, `density=50`, `seed=42`, `update=0`, S1/S2/S3 | Present: one observed stress-test configuration; timestamp span combines fill, barriers, dispatch, indirect processing and graphics; S3 visibility readback is outside `cpu_record`; no stage attribution or complete CPU-frame claim. |
| C3 | 5.4 Scene Mesh Composition and GPU Workload | F5.3 | “三个正式 S3 配置的 `gpu_exec_avg` 分别为 seed42 52,879.3 µs、seed1337 52,910.9 µs 和 seed9999 600,999 µs。seed9999 相对 seed42 为 11.37×，相对 seed1337 为 11.36×。” | Formal timing: `results/_aggregate.csv`, `grid=4096`, `chunk=16`, `density=80`, `scheme=3`, `update=0`, seeds 42/1337/9999. Supplementary composition source binaries: `cache/wfc_4096_{42,1337,9999}_80.bin`. Parsing assigns tile IDs 1–3 to cube family and 4–5 to sphere family. Exact cube/sphere counts are seed42 15,235,807/500; seed1337 15,236,138/510; seed9999 2,363/14,703,079. | Present: association language only; seed-paired cross-artifact display has no pooled fit or causal/stage attribution. Cache binaries contain only a grid header and tile-ID payload, with no embedded commit, dirty-state, or executable-hash identity; no universal family-locking claim. |
| C4 | 5.5 Standalone Buffer-Update Submission Granularity | F5.4 | “在 update size 32 下，九项配置的 batched 均值为 87.07 µs，raw mode `perchunk` 的均值为 1,898.44 µs，两条整路径的比值为 21.80×。只取 seed42 并跨三个路径标签平均时，比值为 21.93×；按路径标签分别跨三个 seeds 汇总时，比值范围为 20.93–23.24×。” | `results/_aggregate_update.csv`; `grid=128`, `chunk=8`, `density=50`, `update_size=32`, all three seeds and path labels, both modes | Present: standalone whole-update arms differ in allocations, command-buffer behavior, submissions, waits, and disposal; CPU selection/data modification precedes the timer; samples repeat a fixed logical update; no sole submission-granularity attribution, dynamic-frame inference, or isolated submit/wait inference. |

## Execution-level numeric summary

RV1 is a descriptive result in Section 5.6 and is not promoted into C1--C4. Its only source is the 75 raw files matching `results_repeatvalidation/regime{A--E}_grid*_chunk*_dens*_scheme*_seed*_run*.csv`. For every regime/path cell, each of the five independent process executions is reduced to the arithmetic mean of its 1200 `gpu_exec_us` frame rows. Execution-level CV is the sample standard deviation of those five execution means, using denominator n-1, divided by their arithmetic mean and multiplied by 100.

| Regime | S1 CV (%) | S2 CV (%) | S3 CV (%) |
|---|---:|---:|---:|
| A: 4096/4/preset50/seed42 | 0.0167 | 0.0135 | 0.5726 |
| B: 1024/4/preset50/seed42 | 0.0372 | 0.0451 | 0.1103 |
| C: 16/4/preset50/seed42 | 0.2606 | 0.1438 | 0.2059 |
| D: 4096/16/preset80/seed9999 | 0.1247 | 0.0138 | 0.3542 |
| E: 1024/16/preset50/seed42 | 0.2561 | 0.1168 | 0.1297 |

The registered range is 0.0135%–0.5726% across 15 selected regime/path cells. All 75 raw headers record commit `407efde`, `dirty=true`, and executable hash `e415726b9b02c3e5`. This summary does not use 1200 frames as independent process repetitions, does not cover the remaining formal configurations, and supports no significance or confidence-interval claim.

## Claim-use controls

- Refer to S1, S2, and S3 as rendering paths. Reserve `scheme` for source code, CLI, or CSV fields.
- Name `cpu_record` as CPU preparation-and-command-recording time. It is a project-defined sum of timed CPU segments; the separately marked `BeginRenderToScreen` interval, S3 visibility readback, and other work outside the markers are excluded, so it is not complete CPU frame cost.
- Name `cpu_wait` as CPU wait time while stating its project-specific raw boundary. The field wraps the whole `BeginRenderToScreen` call and includes `waitForFences`, image-transition command recording, viewport/scissor setup, and `beginRendering`; it is not a pure wait metric. The earlier timeline-semaphore wait, `acquireNextImageKHR`, command-buffer reset, and command-buffer begin in `BeginFrame` are outside its markers. Never attribute this field solely to waiting.
- Name `gpu_exec` as GPU elapsed time measured with timestamp queries. Its top-of-pipe to bottom-of-pipe command span combines stages and supports no single-stage attribution.
- Name `frame_wall` as recorded frame wall-clock span and retain its project-diagnostic status. Its partial boundary excludes work outside the recorded markers, so it is not complete end-to-end frame time and must not support a complete throughput claim.
- Name `update_cost` as standalone buffer-update time. CPU candidate selection and data modification precede the timer; staging allocation/copy, command-buffer behavior, submission, wait, and disposal are inside. Every call reconstructs `mt19937(seed+9999)`, so the 10 warm-ups and 100 records repeat one fixed logical chunk selection and replacement-value sequence, may reflect warmed/reused state, and do not sample heterogeneous edit contents. Batched/perchunk is a whole-update-path comparison isolated from rendering integration.
- Describe the fixed top-down camera as a high-visibility overview. Recorded `visible_chunks` can be slightly below total chunks, so do not label the recorded configurations universally all-visible.
- Treat C1 and C2 as formal-matrix observations. Treat C3 cache interpretation and its supplementary sweep as separately labelled evidence. Treat C4 as a standalone whole-update-path comparison, not an isolated submission or wait effect.
- Record both within-execution frame variation and execution-level variation where available. Do not use selected independent process executions as repetitions for the formal matrix.

## Results checkpoint

- Chapter writing-quality correction completed: `# 5. Results \& Analysis` on 2026-07-21.
- External manuscript SHA-256: `798B8E86F4CEDD5A700C133ED46BE62AD90C076F11ED5C71AFA34C5939BCDC52`.
- C1--C4 appear in Sections 5.2--5.5 with F5.1--F5.4 respectively. Every subsection states the evaluation question, exact data/filter, direct observation, supported interpretation, and unresolved mechanism or inference limit.
- Section 5.6 uses one mean from each of five independent process executions per selected regime/path cell. Its CV calculation and all 15 displayed values are registered under RV1 above; it supplies no repetition claim for the 702-row formal matrix.
- Formal timing and update captions carry `407efde`/`dirty=true`/`e415726b9b02c3e5`. F5.3 separately states that its cache binaries lack embedded commit, dirty-state, and executable-hash metadata; the percolation direction check remains a separate `e945014`/`dirty=true`/`e72c6eedf521222c` supplementary tier.

## Discussion and conclusion checkpoint

- Chapters completed: `# 6. Discussion` and `# 7. Conclusion and Critical Reflection` on 2026-07-21.
- External manuscript SHA-256: `7B667C75A4625D0DA1234BB3E30B506325A399B67EAFD3DD9389E2D82784EED4`.
- No benchmark was run and no new observation was added. Chapters 6--7 interpret C1--C4 and approved implementation or method metadata only.

| Claim | Results location | Discussion location | Conclusion location | Retained inference boundary |
|---|---|---|---|---|
| C1 | §5.2 / F5.1 | §6.1 | §7.1 | S1/S2 is a complete-rendering-path comparison; no isolated MDI effect. |
| C2 | §5.3 / F5.2 | §6.2 | §7.1 | One observed stress-test configuration; combined GPU timestamp span and partial CPU metric. |
| C3 | §5.4 / F5.3 | §6.3 | §7.1 | Formal timing and cache composition are associated; no single-factor cause, universal locking, or probability claim. |
| C4 | §5.5 / F5.4 | §6.4 | §7.1 | Batched and per-chunk are complete standalone update arms; no isolated submit or wait effect. |

- §6.5 consolidates metric, repetition, artifact and external-validity boundaries. It identifies the camera as a high-visibility top-down overview rather than an all-visible condition.
- §7.2 retains the single-GPU, coupled-path, partial-timing, provenance and simplified-WFC limitations.
- §7.3 records personal scope and measurement lessons, including why GPU-assisted WFC remained outside the implemented evaluation.
- §7.4 maps each future-work direction to a current evidence gap: stage timestamps and full CPU timing; replayable visibility paths; compacted/count/device-local indirect paths; cross-hardware and richer workloads; integrated update paths; GPU-assisted WFC; and provenance-aware PCG feedback.

## Front-matter and research-question alignment checkpoint

- Front matter and Introduction completed on 2026-07-21.
- External manuscript SHA-256: `FFB5AF8189726145A8BA1ABDE8A8CEF422543F59CE537036EC7A14119E604C80`.
- Working title: `Evaluating CPU- and GPU-Driven Rendering Paths for WFC-Generated Modular Scenes in Vulkan`.
- The primary question asks how the implemented CPU- and GPU-driven paths differ in CPU preparation-and-command-recording time and timestamp-query GPU elapsed time in WFC-generated modular scenes. Grid size, chunk size and scene mesh composition are analysis dimensions; the standalone update path is a separate subquestion.
- The abstract uses C1--C4 headline observations and approved method metadata without merging rendering, update or repeat evidence. Selected-regime process executions remain in the method and results chapters.

| Question or objective | Introduction | Evaluation Method | Results | Discussion | Conclusion |
|---|---|---|---|---|---|
| Complete S1/S2 and S2/S3 rendering-path timing comparisons | §1.2; Objective 3 | §4.1 | §5.2--§5.3 | §6.1--§6.2 | §7.1 |
| Variation with grid size, chunk size and scene mesh composition | §1.2; Objective 3 | §4.1--§4.2 | §5.2--§5.4 | §6.2--§6.3 | §7.1 |
| Batched versus per-chunk standalone update paths | §1.2 subquestion; Objective 4 | §4.1; §4.4 | §5.5 | §6.4 | §7.1 |
| Metric, repetition, artifact and external-validity boundaries | Objectives 2 and 4 | §4.3--§4.6 | §5.1; §5.6--§5.7 | §6.5 | §7.2--§7.4 |

- The title is followed by a technical-video URL placeholder; the submission version must replace it with the real hosted URL.
- GPU-assisted WFC is identified as an unimplemented contingent extension and appears only in scope, reflection and future work.

## Evaluation Method checkpoint

- Chapter completed: `# 4. Evaluation Method` on 2026-07-21.
- External manuscript SHA-256: `CAA47DBFC91902602A245AA339DF92105BF6CF34C4BB71F529B4F2DF91C527DF`.
- Dataset hierarchy: 702 formal steady-state rendering configurations; 108 standalone update configurations; 120 warm-up plus 1200 recorded frames for every formal rendering configuration; 75 independent process executions across five selected regimes.
- Sample-unit boundary: formal frame rows quantify within-execution variation. Execution-level variation is available only for regimes A--E and cannot represent all 702 formal rows. Standalone update records repeatedly execute one deterministic logical edit per configuration and do not vary edit content.
- Provenance boundary: formal rendering, update, and repeat raw files share `407efde`/`dirty=true`/`e415726b9b02c3e5`; weight-sweep raw files share that identity but remain supplementary; percolation raw files use `e945014`/`dirty=true`/`e72c6eedf521222c`; cache binaries embed none of commit, dirty state, or executable hash. Each dirty flag proves only that whole-worktree `git status --porcelain` was non-empty at build time. It does not identify a specific uncommitted file that affected the executable. The exact non-clean state was not archived; the hash identifies the recorded artifact and cannot recover its source state.

## Design and Implementation checkpoint

- Chapter completed: `# 3. Design and Implementation` on 2026-07-21.
- External manuscript SHA-256: `60AD2EB5101D569B1031D74DE90597180CCD3E34679D95D4684C72F6DE5B70F9`.
- Mechanism anchors and conservative ownership boundaries are recorded in `docs/dissertation/source-map.md` under “Design and implementation mechanisms” and “Contribution ownership boundary.”
- Chapter 3 introduces no empirical observation. C1–C3 remain unchanged; C4's required limitation now cross-references the whole-update-path boundary. These controls govern later results-oriented prose.
