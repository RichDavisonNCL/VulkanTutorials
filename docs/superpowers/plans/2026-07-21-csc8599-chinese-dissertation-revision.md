# CSC8599 Chinese Dissertation Revision Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 将 `D:\D-Code\Code-Essay\thesis.md` 重构为技术事实、指标边界、数据 provenance 与主张强度均可追溯的 CSC8599 中文 dissertation 母稿。

**Architecture:** 先冻结术语、主张和数据来源，再用可重复的只读数据脚本生成证据摘要与图表，随后依次重写 Design & Implementation、Evaluation Method、Results、Related Work、Discussion、Conclusion、Introduction 和 Abstract。核心章节先写，前后 framing 后写，避免摘要和贡献列表先于证据定型。

**Tech Stack:** Markdown、Python 3、Python standard library、pandas、matplotlib、PowerShell、Vulkan/C++20 源码、现有 CSV/cache artifacts、Git。

## Global Constraints

- 当前阶段不考虑 20 页限制，不翻译英文，不做 ACM 压缩。
- 不新增 benchmark，不重新采集 performance data；只读分析现有 formal matrix、repeat-validation data、cache 和 supplementary sweeps。
- 构建、unit tests 与明确标记的 smoke validation 可用于软件正确性检查；其输出不得进入论文结果。
- 正文固定使用 rendering path、multi-draw indirect (MDI)、CPU preparation-and-command-recording time、GPU elapsed time measured with timestamp queries、tile-weight preset、observed non-empty-tile proportion、scene mesh composition、within-execution variation 和 execution-level variation。
- 原始代码、CLI 和 CSV 字段名 `scheme`、`density`、`cpu_record`、`gpu_exec` 保持不变；正文首次出现时建立映射。
- S1、S2、S3 固定称为 CPU-frustum-culling direct-draw path、CPU-frustum-culling MDI path、compute-frustum-culling MDI path。
- 只保留四条已锁定工程主张；任何额外数字先加入 claim ledger 并完成来源核验。
- formal matrix、repeat validation 与 supplementary sweeps 分层报告，不混合计算、不共享 provenance 描述。
- `cpu_record` 不代表完整 CPU frame cost；`gpu_exec` 不提供 stage-level attribution；`frame_wall` 不代表完整端到端 frame time。
- WFC singleton 行为写成 EMPTY compatibility invariant；不写成缺陷修复，也不宣传为性能优化。
- `D:\D-Code\Code-Essay` 根目录没有 Git repository；实施前把当前 `thesis.md` 复制到嵌套 repository 的 baseline 目录并提交。论文每个阶段完成后更新 SHA-256 checkpoint。
- 保留 `D:\D-Code\Code-Essay\VulkanTutorials` 中与本计划无关的已有修改和未跟踪数据；每次提交只暂存任务列出的文件。
- 文稿避免使用无法由 source、data 或 code 支持的因果词、领域排他性措辞和自我评价式语言。

---

## File Structure

### Manuscript

- Modify: `D:\D-Code\Code-Essay\thesis.md` — 中文母稿单一编辑源。
- Create: `D:\D-Code\Code-Essay\VulkanTutorials\docs\dissertation\baselines\thesis-before-rewrite-2026-07-21.md` — 重构前只读快照。

### Evidence-control documents

- Create: `D:\D-Code\Code-Essay\VulkanTutorials\docs\dissertation\claim-ledger.md` — 主张、数字、数据查询、限制和论文位置。
- Create: `D:\D-Code\Code-Essay\VulkanTutorials\docs\dissertation\source-map.md` — 机制到源码行、计时边界到源码行的映射。
- Create: `D:\D-Code\Code-Essay\VulkanTutorials\docs\dissertation\terminology.md` — 论文术语到原始字段/API 的映射。
- Create: `D:\D-Code\Code-Essay\VulkanTutorials\docs\dissertation\figure-register.md` — 每幅图的数据集、过滤条件、metric、caption 和限制。
- Create: `D:\D-Code\Code-Essay\VulkanTutorials\docs\dissertation\citation-audit.md` — 引文元数据、支持范围和正文位置。
- Create: `D:\D-Code\Code-Essay\VulkanTutorials\docs\dissertation\ai-disclosure-draft.md` — AI 使用范围与人工核验责任草案。

### Read-only evidence tooling

- Create: `D:\D-Code\Code-Essay\VulkanTutorials\scripts\dissertation_evidence.py` — 从既有数据重新计算四条锁定主张并输出 machine-readable summary。
- Create: `D:\D-Code\Code-Essay\VulkanTutorials\scripts\tests\test_dissertation_evidence.py` — 对过滤条件、配对键和锁定数值做 regression checks。
- Create: `D:\D-Code\Code-Essay\VulkanTutorials\scripts\make_dissertation_figures.py` — 只从已登记数据生成正文候选图。
- Create: `D:\D-Code\Code-Essay\VulkanTutorials\docs\dissertation\evidence\locked-claims.json` — 由脚本生成的数值摘要。
- Create: `D:\D-Code\Code-Essay\VulkanTutorials\docs\dissertation\evidence\locked-claims.md` — 供人工核验的同内容表格。

### Existing authoritative inputs

- Read: `D:\D-Code\Code-Essay\VulkanTutorials\results\_aggregate.csv`。
- Read: `D:\D-Code\Code-Essay\VulkanTutorials\results\_aggregate_update.csv`。
- Read: `D:\D-Code\Code-Essay\VulkanTutorials\results_repeatvalidation\*.csv`。
- Read: `D:\D-Code\Code-Essay\VulkanTutorials\results_percolation_summary_full.csv`。
- Read: `D:\D-Code\Code-Essay\VulkanTutorials\results_weightsweep_summary_full.csv`。
- Read: `D:\D-Code\Code-Essay\VulkanTutorials\GPUDrivenRendering\GPUSceneManagement.cpp`。
- Read: `D:\D-Code\Code-Essay\VulkanTutorials\GPUDrivenRendering\GPUSceneManagement.h`。
- Read: `D:\D-Code\Code-Essay\VulkanTutorials\GPUDrivenRendering\WFCGenerator.cpp`。
- Read: `D:\D-Code\Code-Essay\VulkanTutorials\GPUDrivenRendering\WFCGenerator.h`。
- Read: `D:\D-Code\Code-Essay\VulkanTutorials\GPUDrivenRendering\Shaders\Cull.comp`。
- Read: `D:\D-Code\Code-Essay\VulkanTutorials\docs\superpowers\specs\2026-07-21-csc8599-dissertation-revision-design.md`。

---

### Task 1: Freeze the baseline and create the evidence-control layer

**Files:**

- Create: `docs/dissertation/baselines/thesis-before-rewrite-2026-07-21.md`
- Create: `docs/dissertation/claim-ledger.md`
- Create: `docs/dissertation/source-map.md`
- Create: `docs/dissertation/terminology.md`
- Create: `docs/dissertation/figure-register.md`
- Create: `docs/dissertation/citation-audit.md`
- Create: `docs/dissertation/ai-disclosure-draft.md`

**Interfaces:**

- Consumes: approved design specification and current `..\thesis.md`.
- Produces: the control documents that every later task must update before changing a claim, metric name, source interpretation or figure.

- [ ] **Step 1: Copy the unversioned manuscript into the repository as a baseline**

Run from `D:\D-Code\Code-Essay\VulkanTutorials`:

    New-Item -ItemType Directory -Force docs\dissertation\baselines | Out-Null
    Copy-Item -LiteralPath ..\thesis.md -Destination docs\dissertation\baselines\thesis-before-rewrite-2026-07-21.md
    Get-FileHash ..\thesis.md,docs\dissertation\baselines\thesis-before-rewrite-2026-07-21.md -Algorithm SHA256

Expected: both SHA-256 values are identical.

- [ ] **Step 2: Create the claim ledger with the four locked claims**

Use `apply_patch`. The initial table must contain these rows:

| ID | Direct observation | Evidence | Required limitation |
|---|---|---|---|
| C1 | 135 matched configurations at grid≥256 give S1/S2 `cpu_record_avg` ratios of 1.7366–9.9405× | `results/_aggregate.csv`; key = grid, chunk, density, seed | whole-path comparison; no isolated MDI causal attribution |
| C2 | grid4096/chunk4/preset50/seed42 gives S3 CPU time 44.745µs and GPU elapsed time 236,173µs | `results/_aggregate.csv` | timestamp span combines fill, barriers, dispatch, indirect processing and graphics; readback is outside `cpu_record` |
| C3 | grid4096/chunk16/preset80/S3 gives seed9999 600,999µs versus seed42/1337 52,879.3/52,910.9µs | formal CSV plus cache histogram and separately labelled supplementary sweep | association with scene mesh composition; no single-stage attribution |
| C4 | 32-chunk update gives batched 87.07µs, per-chunk 1,898.44µs and ratio 21.80× | `results/_aggregate_update.csv` | standalone buffer-update microbenchmark; no full dynamic-frame claim |

- [ ] **Step 3: Create the terminology and source-map tables**

`terminology.md` must include the complete mapping from Section 11 of the approved design. `source-map.md` must begin with these source anchors:

| Mechanism or metric | Source anchor |
|---|---|
| S1 CPU culling and direct draws | `GPUSceneManagement.cpp:637-688` |
| S2 host-populated indirect draw buffer and two MDI commands | `GPUSceneManagement.cpp:690-753` |
| S3 fill, barriers, compute dispatch and MDI | `GPUSceneManagement.cpp:755-823` |
| `cpu_record` segment accumulation | `GPUSceneManagement.cpp:638-685, 692-751, 757-821, 843-856` |
| timestamp-query start | `GPUSceneManagement.cpp:858-871` |
| EMPTY catalogue and adjacency invariant | `WFCGenerator.cpp:12-28` |
| output grid default EMPTY representation | `WFCGenerator.cpp:38-47` |
| propagation singleton behavior | `WFCGenerator.cpp:112-153` |

- [ ] **Step 4: Create the figure, citation and AI-control documents**

`figure-register.md` starts with columns: Figure ID, research question, source dataset, exact filter, metric, sample unit, artifact identity, caption claim, limitation.

`citation-audit.md` starts with columns: Citation key, canonical title, authors, venue, year, DOI/URL, primary source checked, supported sentence, status.

`ai-disclosure-draft.md` contains this scope:

    AI tools were used for structural planning, language revision, code and citation auditing, and consistency checks. The author manually verified source code, datasets, numerical results, bibliographic metadata, and all conclusions, and remains responsible for the submitted work.

- [ ] **Step 5: Validate and commit only the control layer**

Run:

    rg -n "1\.7366|9\.9405|236,173|600,999|21\.80" docs\dissertation
    git -c safe.directory=D:/D-Code/Code-Essay/VulkanTutorials add -f docs/dissertation
    git -c safe.directory=D:/D-Code/Code-Essay/VulkanTutorials diff --cached --check
    git -c safe.directory=D:/D-Code/Code-Essay/VulkanTutorials commit -m "docs: add dissertation evidence controls"

Expected: all five locked numeric tokens appear in `claim-ledger.md`; the commit contains only `docs/dissertation` files.

---

### Task 2: Build a read-only locked-claim extractor

**Files:**

- Create: `scripts/tests/test_dissertation_evidence.py`
- Create: `scripts/dissertation_evidence.py`
- Create: `docs/dissertation/evidence/locked-claims.json`
- Create: `docs/dissertation/evidence/locked-claims.md`

**Interfaces:**

- Consumes: `results/_aggregate.csv` and `results/_aggregate_update.csv`.
- Produces: `load_csv(path) -> list[dict]`, `compute_locked_claims(render_rows, update_rows) -> dict` and `write_outputs(claims, out_dir) -> None`.

- [ ] **Step 1: Write regression tests before the extractor**

Use this complete test module:

```python
import unittest
from pathlib import Path

from scripts.dissertation_evidence import compute_locked_claims, load_csv

ROOT = Path(__file__).resolve().parents[2]


class LockedClaimTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.render = load_csv(ROOT / "results" / "_aggregate.csv")
        cls.update = load_csv(ROOT / "results" / "_aggregate_update.csv")
        cls.claims = compute_locked_claims(cls.render, cls.update)

    def test_s1_s2_matched_range(self):
        c = self.claims["C1"]
        self.assertEqual(c["pair_count"], 135)
        self.assertAlmostEqual(c["ratio_min"], 1.7366, places=4)
        self.assertAlmostEqual(c["ratio_max"], 9.9405, places=4)

    def test_stress_configuration(self):
        c = self.claims["C2"]
        self.assertAlmostEqual(c["s3_cpu_us"], 44.745, places=3)
        self.assertAlmostEqual(c["s3_gpu_us"], 236173.0, places=0)

    def test_scene_composition_timing_ratio(self):
        c = self.claims["C3"]
        self.assertAlmostEqual(c["seed9999_gpu_us"], 600999.0, places=0)
        self.assertAlmostEqual(c["ratio_vs_seed42"], 11.37, places=2)
        self.assertAlmostEqual(c["ratio_vs_seed1337"], 11.36, places=2)

    def test_buffer_update_ratio(self):
        c = self.claims["C4"]
        self.assertAlmostEqual(c["batched_mean_us"], 87.07, places=2)
        self.assertAlmostEqual(c["perchunk_mean_us"], 1898.44, places=2)
        self.assertAlmostEqual(c["ratio"], 21.80, places=2)

if __name__ == "__main__":
    unittest.main()
```

- [ ] **Step 2: Run the tests and verify they fail because the module is absent**

Run:

    python -m unittest scripts.tests.test_dissertation_evidence -v

Expected: import failure for `scripts.dissertation_evidence`.

- [ ] **Step 3: Implement deterministic filtering and matching**

Implementation requirements:

- Convert grid, chunk, density, scheme and seed to integers and metric columns to floats.
- C1: index S2 rows by `(grid, chunk, density, seed)`, pair only S1 rows with `grid >= 256`, and compute `S1 cpu_record_avg / S2 cpu_record_avg`.
- C2: select exactly one row per scheme for grid=4096, chunk=4, density=50, seed=42.
- C3: select S3 rows for grid=4096, chunk=16, density=80 and seeds 42, 1337 and 9999.
- C4: filter update_size=32, group all seeds and rendering paths by mode, take the arithmetic mean of `update_cost_avg`, then divide per-chunk by batched.
- Refuse duplicate keys and missing required rows with `ValueError`.
- Read files only; do not invoke benchmark executables and do not modify any input CSV.

- [ ] **Step 4: Add exact JSON and Markdown output**

The CLI is:

    python scripts/dissertation_evidence.py --render results/_aggregate.csv --update results/_aggregate_update.csv --out docs/dissertation/evidence

The JSON must include `source_files`, `generated_at_utc`, `claims` and `rounding_policy`. The Markdown must show raw values before rounded prose values.

- [ ] **Step 5: Run tests and generate the evidence artifacts**

Run:

    python -m unittest scripts.tests.test_dissertation_evidence -v
    python scripts/dissertation_evidence.py --render results/_aggregate.csv --update results/_aggregate_update.csv --out docs/dissertation/evidence
    Get-FileHash results\_aggregate.csv,results\_aggregate_update.csv -Algorithm SHA256

Expected: four tests pass; the command prints `C1 pair_count=135`, `C2 s3_gpu_us=236173`, `C3 seed9999_gpu_us=600999` and `C4 ratio=21.80`.

- [ ] **Step 6: Commit the extractor and generated evidence**

Run:

    git -c safe.directory=D:/D-Code/Code-Essay/VulkanTutorials add scripts/dissertation_evidence.py scripts/tests/test_dissertation_evidence.py
    git -c safe.directory=D:/D-Code/Code-Essay/VulkanTutorials add -f docs/dissertation/evidence
    git -c safe.directory=D:/D-Code/Code-Essay/VulkanTutorials diff --cached --check
    git -c safe.directory=D:/D-Code/Code-Essay/VulkanTutorials commit -m "analysis: lock dissertation evidence queries"

---

### Task 3: Regenerate figures with canonical terminology and provenance

**Files:**

- Create: `scripts/make_dissertation_figures.py`
- Modify: `docs/dissertation/figure-register.md`
- Modify: `D:\D-Code\Code-Essay\Figures\fig5_1_cpu_record_bar.png`
- Modify: `D:\D-Code\Code-Essay\Figures\fig5_2_content_sensitivity_bar.png`
- Modify: `D:\D-Code\Code-Essay\Figures\fig5_3_triangle_count_scatter.png`
- Create: `D:\D-Code\Code-Essay\Figures\fig5_4_buffer_update_submission.png`

**Interfaces:**

- Consumes: existing aggregate CSVs, cache histograms already used by `make_fig53.py`, and locked-claims JSON.
- Produces: four PNG figures plus a figure register entry for each.

- [ ] **Step 1: Implement a `--check-only` mode before plotting**

`--check-only` validates that every registered source file exists, every filter returns the expected row count, and every plotted locked value equals `locked-claims.json`. It exits non-zero on any mismatch.

- [ ] **Step 2: Implement four evidence-aligned figures**

Use these panels and labels:

1. C1: distribution of matched `S1/S2 CPU preparation-and-command-recording time ratio` for grid≥256; show n=135 and 1.74–9.94×.
2. C2: two aligned panels, one for CPU preparation-and-command-recording time and one for GPU elapsed time measured with timestamp queries; avoid a shared bottleneck axis or `max(cpu,gpu)`.
3. C3: GPU elapsed time against total triangle count or scene mesh composition; distinguish formal data from supplementary evidence in caption and marker style.
4. C4: batched and per-chunk standalone buffer-update time by update size; emphasize the 32-chunk 21.80× comparison.

Axis labels must not contain `Density (%)`, `GPU execution time`, `CPU frame time` or `Scheme`. Use `Tile-weight preset`, `GPU elapsed time from timestamp queries`, `CPU preparation-and-command-recording time` and `Rendering path`.

- [ ] **Step 3: Generate and inspect file metadata**

Run from `VulkanTutorials`:

    python scripts/make_dissertation_figures.py --check-only
    python scripts/make_dissertation_figures.py
    Get-ChildItem ..\Figures\fig5_*.png | Select-Object Name,Length,LastWriteTime

Expected: check-only exits 0; four files are present and each has non-zero length.

- [ ] **Step 4: Complete the figure register**

Each entry records dataset name, exact filters, metric boundary, sample unit, commit/dirty/executable metadata, visible caption claim and one explicit limitation sentence. Formal and supplementary sources receive separate artifact identity fields.

- [ ] **Step 5: Commit only the script and register**

The root-level PNG files are outside the nested repository and remain uncommitted. Commit:

    git -c safe.directory=D:/D-Code/Code-Essay/VulkanTutorials add scripts/make_dissertation_figures.py
    git -c safe.directory=D:/D-Code/Code-Essay/VulkanTutorials add -f docs/dissertation/figure-register.md
    git -c safe.directory=D:/D-Code/Code-Essay/VulkanTutorials commit -m "analysis: generate evidence-aligned dissertation figures"

---

### Task 4: Rewrite Design & Implementation from source code

**Files:**

- Modify: `D:\D-Code\Code-Essay\thesis.md:157-268`
- Modify: `docs/dissertation/source-map.md`
- Modify: `docs/dissertation/claim-ledger.md`

**Interfaces:**

- Consumes: source map, WFC sources, render-path sources and approved terminology.
- Produces: a source-verifiable Design & Implementation chapter with no performance conclusions.

- [ ] **Step 1: Replace the current chapter outline**

Use this exact order:

1. System Requirements and Scope
2. Simplified WFC Scene Generator and Cache
3. Split-SSBO Scene Representation
4. Three Rendering Paths
5. Synchronisation and Memory Flow
6. Local Buffer-Update Path
7. Benchmark Automation, Tests and Personal Contribution

- [ ] **Step 2: Write the WFC and cache subsection**

Cover the six-tile catalogue, weighted selection, adjacency propagation, deterministic seed use, cache key and cache provenance. State that output `grid` begins with EMPTY ID 0, EMPTY is compatible with every tile in both adjacency directions, and propagation-only singleton states therefore remain `{EMPTY}` under the current catalogue. State that this is an implementation invariant and add no performance claim.

- [ ] **Step 3: Write scene representation and rendering-path subsections**

Define:

- S1: CPU frustum culling followed by per-chunk direct indexed draw-command recording.
- S2: CPU frustum culling, host population of a fixed 2N-record indirect draw buffer, host-to-indirect barrier, and two recorded `vkCmdDrawIndexedIndirect` commands.
- S3: buffer clear, transfer-to-compute barrier, compute frustum culling, compute-to-indirect barrier, GPU population of the same fixed 2N-record layout, and the same two MDI commands.

Explain that invisible records use `instanceCount=0` and no command compaction or indirect-count buffer is implemented. Record `local_size_x=64`, one invocation per chunk, and the grid4096/chunk4 values 16,384 workgroups and 1,048,576 invocations.

- [ ] **Step 4: Write synchronisation, update and contribution subsections**

Describe only barriers and memory properties visible in source. For local updates, list staging allocation, memcpy, copy-command recording, submit and wait inside the timer, and CPU-side data modification before the timer. Add a contribution map separating course framework, WFC generator, GPU scene management, compute shader, CLI/automation, tests and analysis.

- [ ] **Step 5: Run a terminology and source-coverage check**

Run:

    rg -n "cylinder|atomic write|fixed-length|two draw calls|one workgroup per chunk|full WFC|standard WFC" ..\thesis.md
    rg -n "S1|S2|S3|vkCmdDrawIndexedIndirect|2N-record|instanceCount=0|local_size_x" ..\thesis.md

Expected: the first command has no matches; the second finds all required implementation facts.

- [ ] **Step 6: Update source-map anchors and create a manuscript checkpoint**

Run:

    Get-FileHash ..\thesis.md -Algorithm SHA256
    git -c safe.directory=D:/D-Code/Code-Essay/VulkanTutorials add -f docs/dissertation/source-map.md docs/dissertation/claim-ledger.md
    git -c safe.directory=D:/D-Code/Code-Essay/VulkanTutorials commit -m "docs: verify dissertation implementation chapter"

Record the manuscript SHA-256 and chapter completion date in `source-map.md` before committing.

---

### Task 5: Rewrite Evaluation Method around actual measurement boundaries

**Files:**

- Modify: `D:\D-Code\Code-Essay\thesis.md:268-376`
- Modify: `docs/dissertation/claim-ledger.md`
- Modify: `docs/dissertation/source-map.md`

**Interfaces:**

- Consumes: metric source anchors, formal/repeat/supplementary metadata and evidence artifacts.
- Produces: an Evaluation Method chapter that defines what each metric includes and excludes.

- [ ] **Step 1: Replace the experimental chapter outline**

Use:

1. Research Questions and Comparison Logic
2. Parameter Matrix
3. Hardware, Build and Artifact Identity
4. Scene Generation and Cache Protocol
5. Metric Boundaries
6. Frame Sampling and Within-Execution Variation
7. Independent Process Executions and Execution-Level Variation
8. Dataset Provenance
9. Threats to Validity

- [ ] **Step 2: State the dataset counts and hierarchy**

Report 702 steady-state rendering configurations, 108 standalone buffer-update configurations, 120 warm-up frames and 1200 measurement frames per formal configuration, and 75 independent process executions across five selected regimes. Explain that 1200 frames estimate within-execution variation and the 75 runs inspect execution-level variation only for selected regimes.

- [ ] **Step 3: Define every metric with inclusions and exclusions**

Use these display names:

- `cpu_record` → CPU preparation-and-command-recording time.
- `cpu_wait` → CPU wait time.
- `gpu_exec` → GPU elapsed time measured with timestamp queries.
- `frame_wall` → recorded frame wall-clock span.
- `update_cost` → standalone buffer-update time.

State that `cpu_record` is a sum of timed CPU segments, `gpu_exec` spans top-of-pipe to bottom-of-pipe commands and has no per-stage breakdown, `frame_wall` omits work outside its markers, and `update_cost` is isolated from rendering integration.

- [ ] **Step 4: Add provenance and validity threats**

Include RTX 4080 SUPER, driver 610.47.0.0, Release build, formal commit `407efde`, `dirty=true` and executable hash `e415726b9b02c3e5`. State that formal rows share one executable identity while the current checkout cannot recreate the dirty executable byte-for-byte. Separate supplementary sweep identities.

Threats must include single GPU, top-down all-visible camera, one resolution, simple meshes/materials, naïve CPU culling, host-visible indirect buffer, non-compacted fixed 2N-record layout, readback outside `cpu_record`, no stage-level timestamps, selected-only execution repetitions and dirty artifact provenance.

- [ ] **Step 5: Delete invalid metric language**

Run:

    rg -n "CPU/GPU bottleneck|CPU-bound|GPU-bound|max\(cpu_record|complete frame time|end-to-end frame|density [0-9]+%" ..\thesis.md

Expected: no claims use those expressions. A limitation sentence may mention why end-to-end frame time is unavailable, but it must not assign a bottleneck.

- [ ] **Step 6: Checkpoint the chapter**

Update claim-ledger metric definitions, record the manuscript SHA-256, force-add only the two control documents and commit with:

    git -c safe.directory=D:/D-Code/Code-Essay/VulkanTutorials commit -m "docs: lock dissertation evaluation method"

---

### Task 6: Rebuild Results around the four locked claims

**Files:**

- Modify: `D:\D-Code\Code-Essay\thesis.md:376-532`
- Modify: `docs/dissertation/claim-ledger.md`
- Modify: `docs/dissertation/figure-register.md`

**Interfaces:**

- Consumes: locked-claims JSON/Markdown, figure register and Evaluation Method definitions.
- Produces: a Results chapter with four evidence-backed observations and selected-regime repetition analysis.

- [ ] **Step 1: Replace the chapter structure**

Use:

1. Reading Guide and Evidence Levels
2. S1→S2 CPU Preparation-and-Command-Recording Time
3. CPU–GPU Timing Trade-off in the 236.2ms Stress-Test Configuration
4. Scene Mesh Composition and GPU Workload
5. Standalone Buffer-Update Submission Granularity
6. Selected-Regime Execution-Level Variation
7. Results Summary

Every subsection follows: evaluation question, data/filter, direct observation, supported interpretation, unresolved mechanism.

- [ ] **Step 2: Write C1 and C2 with locked wording**

C1 must state n=135, grid≥256, matched grid/chunk/tile-weight preset/seed, and 1.74–9.94× lower CPU preparation-and-command-recording time for S2 relative to S1. It must say the comparison covers both complete rendering paths.

C2 must report the three CPU times 44,540.6µs, 7,422.7µs and 44.745µs, plus the three GPU elapsed times 38,723.6µs, 44,730.5µs and 236,173µs. It must identify the result as an observed stress-test configuration and state the timestamp-span and readback limitations in the same subsection.

- [ ] **Step 3: Write C3 and C4 with dataset separation**

C3 reports seed42 52,879.3µs, seed1337 52,910.9µs and seed9999 600,999µs; report cube/sphere counts from the cache audit and label supplementary sweep support separately. Use association language.

C4 reports all-seed/all-path means of 87.07µs and 1,898.44µs, ratio 21.80×, plus seed42 ratio 21.93× and path range 20.93–23.24×. State that CPU data modification precedes the timer and that the result is not a dynamic-frame measurement.

- [ ] **Step 4: Add selected-regime repetition results without upgrading scope**

Use the five independent executions per regime/path to discuss execution-level variation. Do not treat the 1200 frames as independent process repetitions and do not generalize the selected regimes to all 702 formal configurations.

- [ ] **Step 5: Insert figures and captions from the register**

Each caption includes source dataset, configuration, metric boundary and one limitation. Do not place provenance only in surrounding prose.

- [ ] **Step 6: Run locked-number and banned-claim checks**

Run:

    python scripts/dissertation_evidence.py --render results/_aggregate.csv --update results/_aggregate_update.csv --out docs/dissertation/evidence
    rg -n "1\.74|9\.94|236\.2|11\.36|11\.37|21\.80|21\.93" ..\thesis.md
    rg -n "proves|证明了|caused by compute|saturation boundary|bottleneck crossover|frames equivalent" ..\thesis.md

Expected: every locked rounded value appears; the banned-claim scan has no matches.

- [ ] **Step 7: Update the claim ledger and checkpoint**

For C1–C4, fill the manuscript subsection, figure ID, exact sentence and limitation-present columns. Commit the updated ledger and figure register only.

---

### Task 7: Rebuild Background, Related Work and References from primary sources

**Files:**

- Modify: `D:\D-Code\Code-Essay\thesis.md:65-157`
- Modify: `D:\D-Code\Code-Essay\thesis.md:612-634`
- Modify: `docs/dissertation/citation-audit.md`

**Interfaces:**

- Consumes: primary papers, Vulkan specification/reference pages and WFC repository.
- Produces: a related-work chapter whose comparison claims are supported by cited sources and a fully audited reference list.

- [ ] **Step 1: Use the academic citation-check workflow before rewriting**

Invoke `academic-research-skills:source-command-ars-citation-check` against the current manuscript and record each issue in `citation-audit.md`. Check title, authors, venue, year, DOI/URL and the exact sentence supported.

- [ ] **Step 2: Use this chapter order**

1. Vulkan command recording and direct/indirect drawing
2. CPU/GPU visibility culling
3. GPU-driven rendering and non-compacted/compacted indirect draw buffers
4. WFC and simple tiled models for procedural workloads
5. Performance measurement methodology
6. Adjacent systems and this project’s implementation-specific position

- [ ] **Step 3: Correct known reference issues**

- Describe indirect draw/dispatch as Vulkan 1.0 capabilities; identify the experiment itself as Vulkan 1.3.
- Record Aokana as `Proceedings of the ACM on Computer Graphics and Interactive Techniques, 8(1), I3D 2025`.
- Re-check Xylem and Gonakhchyan against their primary documents before describing their evaluation routes.
- State Unterguggenberger’s cullable condition separately from any observed culled proportion.
- Use Khronos reference pages for `vkCmdDrawIndexedIndirect` and `vkCmdWriteTimestamp2`.
- Use the original WFC repository for observation, propagation, entropy, contradiction and simple tiled model terminology.
- Use Kalibera and Jones for within-execution and execution-level variation.

- [ ] **Step 4: Remove unsupported gap claims**

Run:

    rg -n "first study|only study|唯一|尚无研究|没有工作|no prior work|novel algorithm" ..\thesis.md

Expected: no matches. Position the contribution as an implementation-specific comparison on a shared procedural workload.

- [ ] **Step 5: Verify citation/reference closure**

List in-text citation keys and reference-list keys, compare both sets, and resolve every missing or unused entry. Every citation-audit row must end in `verified`, `reworded` or `removed`; no blank status is allowed.

- [ ] **Step 6: Commit the citation audit**

Record the manuscript SHA-256 and commit only `docs/dissertation/citation-audit.md` with message:

    docs: audit dissertation sources and related work

---

### Task 8: Rewrite Discussion, Conclusion, Reflection and Future Work

**Files:**

- Modify: `D:\D-Code\Code-Essay\thesis.md:532-612`
- Modify: `docs/dissertation/claim-ledger.md`

**Interfaces:**

- Consumes: Results chapter and claim ledger.
- Produces: a direct answer to the research question, a smart-pipeline-usage discussion and a personal critical reflection.

- [ ] **Step 1: Organize Discussion around decision rules**

Use:

1. When CPU-side draw-command reduction helped
2. Why moving culling work to the GPU changed the measured timing balance
3. Why fine chunk granularity amplified a non-compacted 2N-record path
4. Why PCG evaluation must record realized scene mesh composition
5. Why batched submit-and-wait outperformed per-chunk submit-and-wait
6. What the measurements cannot attribute

Tie the discussion to the supervisor’s request to measure the cost of changing rendering path and the benefit of smarter path use.

- [ ] **Step 2: Write a bounded conclusion**

Answer the RQ in one paragraph, then restate C1–C4 without introducing any new number. Scope the answer to the tested RTX 4080 SUPER, camera, scene representation, mesh set, timing boundaries and artifacts.

- [ ] **Step 3: Add personal reflection**

Discuss why the project narrowed away from GPU-assisted WFC, what the implementation and measurement audit taught, what would be designed earlier in a second iteration, and how the dirty executable, timestamp boundaries and repeat hierarchy affected confidence.

- [ ] **Step 4: Keep future work measurable**

Include stage-level timestamp queries, visibility-ratio camera paths, compacted indirect draws or indirect-count buffers, device-local indirect buffers, multiple GPUs, richer mesh/material workloads and GPU-assisted WFC. For each item, state which present limitation it resolves.

- [ ] **Step 5: Verify no new evidence enters Discussion or Conclusion**

Compare every numeric token in Chapters 6–7 against `locked-claims.md` and the already approved methods metadata. Remove any number without a ledger row.

- [ ] **Step 6: Checkpoint and commit the ledger**

Record the manuscript SHA-256, mark C1–C4 as represented in Results, Discussion and Conclusion, then commit the ledger.

---

### Task 9: Rewrite Introduction and Abstract after the evidence is stable

**Files:**

- Modify: `D:\D-Code\Code-Essay\thesis.md:1-65`
- Modify: `docs/dissertation/claim-ledger.md`

**Interfaces:**

- Consumes: completed core chapters.
- Produces: title, abstract, research question, objectives, contributions and thesis map that match the evidence.

- [ ] **Step 1: Set the working title and exact research question**

Title:

    A Vulkan-based Evaluation of GPU-Driven Scene Management for PCG Modular Scenes

Research question:

    How do three implementation-specific CPU- and GPU-driven rendering paths affect CPU preparation-and-command-recording time and GPU elapsed time measured with timestamp queries when rendering modular scenes produced by a simplified WFC implementation, and how do these trade-offs vary with grid size, chunk size, and scene mesh composition?

- [ ] **Step 2: Write objectives that map to completed chapters**

Objectives cover implementation of the simplified WFC workload, shared Split-SSBO representation, three rendering paths, measurement infrastructure, existing-data evaluation across scale/chunk/preset/seed, standalone buffer-update evaluation, and critical analysis of limitations. GPU-assisted WFC is explicitly a contingent extension that was not pursued.

- [ ] **Step 3: Write contributions in three groups**

- System: WFC/cache, Split-SSBO representation, S1–S3, update path and automation.
- Evaluation: 702 rendering configurations, 108 update configurations and 75 selected-regime process executions.
- Engineering observations: the four ledger claims with bounded wording.

- [ ] **Step 4: Write the abstract last**

The abstract contains problem, implementation, method, C1–C4 headline findings, limitations and conclusion. It must not contain references, unexplained field names, complete-frame claims or a general statement that GPU-driven rendering is faster.

- [ ] **Step 5: Verify front-to-back alignment**

Create a matrix in `claim-ledger.md` with rows RQ/subquestions and columns Introduction, Method, Results, Discussion and Conclusion. Every row must have one location in each applicable column.

- [ ] **Step 6: Checkpoint**

Record the manuscript SHA-256 and commit the updated ledger.

---

### Task 10: Run integrated manuscript QA and a strict reviewer pass

**Files:**

- Modify: `D:\D-Code\Code-Essay\thesis.md`
- Modify: `docs/dissertation/claim-ledger.md`
- Modify: `docs/dissertation/source-map.md`
- Modify: `docs/dissertation/terminology.md`
- Modify: `docs/dissertation/figure-register.md`
- Modify: `docs/dissertation/citation-audit.md`

**Interfaces:**

- Consumes: the complete Chinese manuscript and all control documents.
- Produces: an evidence-locked Chinese master ready for a separate English/ACM plan.

- [ ] **Step 1: Run mechanical scans**

Run:

    rg -n "CPU frame time|GPU execution time|combined GPU|weight tier|fixed-length|Scheme [123]|density [0-9]+%|saturation boundary|bottleneck crossover|frames equivalent" ..\thesis.md
    rg -n "1\.74|9\.94|236\.2|11\.36|11\.37|21\.80" ..\thesis.md
    rg -n "formal|repeat|supplementary|dirty=true|e415726b9b02c3e5" ..\thesis.md

Expected: the outdated-term scan has no matches; locked values and provenance vocabulary are present.

- [ ] **Step 2: Re-run evidence and figure checks**

Run:

    python -m unittest scripts.tests.test_dissertation_evidence -v
    python scripts/dissertation_evidence.py --render results/_aggregate.csv --update results/_aggregate_update.csv --out docs/dissertation/evidence
    python scripts/make_dissertation_figures.py --check-only

Expected: all commands exit 0 and no input data hash changes.

- [ ] **Step 3: Run the strict academic reviewer workflow**

Invoke `academic-research-skills:source-command-ars-reviewer` with the CSC8599 brief, approved design, manuscript, control documents, code anchors and existing datasets. Ask for a hostile assessment of unsupported claims, AI-like generic prose, missing justification, metric ambiguity, citation drift, implementation/evaluation mismatch and 90-band weaknesses.

- [ ] **Step 4: Triage reviewer findings by evidence risk**

Fix in this order:

1. False technical statement or wrong number.
2. Unsupported causal or generalization claim.
3. Metric-boundary ambiguity.
4. Citation metadata or support mismatch.
5. Missing design justification or personal reflection.
6. Repetition, templated prose and presentation weakness.

Every accepted finding updates at least one control document. Rejected findings receive a one-sentence evidence-based rationale in `claim-ledger.md`.

- [ ] **Step 5: Perform a fresh self-review against the approved design**

Check every section of `docs/superpowers/specs/2026-07-21-csc8599-dissertation-revision-design.md` against the manuscript. Add any uncovered requirement before freezing the draft. Scan all plan and control documents for unfinished markers and blank status cells.

- [ ] **Step 6: Freeze the Chinese master**

Run:

    Get-FileHash ..\thesis.md -Algorithm SHA256
    git -c safe.directory=D:/D-Code/Code-Essay/VulkanTutorials diff --check
    git -c safe.directory=D:/D-Code/Code-Essay/VulkanTutorials status --short

Record the final Chinese-master SHA-256, date, reviewer-pass status and evidence-script commit in `claim-ledger.md`. Commit only the control documents and scripts touched by the QA pass. Keep unrelated code and data changes unstaged.

---

## Completion Gate

The Chinese master is complete when:

- the RQ, objectives, Results and Conclusion align;
- C1–C4 are the only headline empirical claims and each has a ledger row;
- every number is traceable to a source file and filter;
- every implementation statement has a source-map anchor;
- formal, repeat and supplementary evidence remain visibly separated;
- metric names and boundaries follow `terminology.md`;
- the WFC singleton behavior is described as the current EMPTY invariant;
- all references have verified metadata and support scope;
- figures include provenance and limitation captions;
- the strict reviewer pass has no unresolved high-risk finding;
- the final SHA-256 is recorded.

After this gate, create a separate implementation plan for English translation, ACM conversion, 20-page compression, software packaging and technical video delivery.
