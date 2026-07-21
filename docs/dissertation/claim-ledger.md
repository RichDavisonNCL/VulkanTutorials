# Claim ledger

This ledger locks the direct observations used by the dissertation. Later work must update this document before changing a numerical claim, its evidence, or its limitation.

| ID | Direct observation | Evidence | Required limitation |
|---|---|---|---|
| C1 | 135 matched configurations at grid≥256 give S1/S2 `cpu_record_avg` ratios of 1.7366–9.9405× | `results/_aggregate.csv`; key = grid, chunk, density, seed | whole-path comparison; no isolated MDI causal attribution |
| C2 | grid4096/chunk4/preset50/seed42 gives S3 CPU time 44.745µs and GPU elapsed time 236,173µs | `results/_aggregate.csv` | timestamp span combines fill, barriers, dispatch, indirect processing and graphics; readback is outside `cpu_record` |
| C3 | grid4096/chunk16/preset80/S3 gives seed9999 600,999µs versus seed42/1337 52,879.3/52,910.9µs | formal CSV plus cache histogram and separately labelled supplementary sweep | association with scene mesh composition; no single-stage attribution |
| C4 | 32-chunk update gives batched 87.07µs, per-chunk 1,898.44µs and ratio 21.80× | `results/_aggregate_update.csv` | standalone whole-update-path comparison; arms differ in command-buffer creation, staging lifetime/disposal, submission and wait counts; no isolated submit/wait attribution or full dynamic-frame claim |

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
