# Source map

Anchors below were rechecked against the current `WFCFix` worktree at Task BASE `abd3408d6abb2fe069ee8203390b50104d950c90` on 2026-07-21. `GPUDrivenRendering/GPUSceneManagement.cpp`, `.h`, and `Main.cpp` contain pre-existing unstaged changes; the anchors intentionally describe the current source read for Chapter 3. No source file was modified for the manuscript rewrite.

## Design and implementation mechanisms

| Mechanism | Current source anchor | Verified interpretation and boundary |
|---|---|---|
| Rendering-path names and benchmark configuration | `GPUDrivenRendering/GPUSceneManagement.h:52-113`; `GPUDrivenRendering/Main.cpp:54-75` | Code/CLI `scheme` values 1/2/3 map to dissertation rendering paths S1/S2/S3. Grid, chunk, preset label, seed, measurement, update, and weight-override controls enter one configuration. |
| Six-tile catalogue and adjacency | `GPUDrivenRendering/WFCGenerator.cpp:12-28` | Catalogue is EMPTY, LOW, MID, HIGH, SPHERE_S, and SPHERE_L. No additional mesh family is present. EMPTY is compatible with every tile in both matrix directions. |
| Seeded weighted simplified WFC | `GPUDrivenRendering/WFCGenerator.h:31-50, 65-78`; `GPUDrivenRendering/WFCGenerator.cpp:38-110` | Output is seeded per generation; every possibility set starts with six tiles; a count/index heap selects cells; weighted choice and four-neighbour propagation are implemented. No Shannon-entropy, backtracking, rotation, or contradiction-recovery mechanism is visible. |
| EMPTY output invariant | `GPUDrivenRendering/WFCGenerator.cpp:43-56, 112-155` | Output `grid` starts at ID 0. Because propagation cannot remove EMPTY under the current matrix, a propagation-only singleton is `{EMPTY}` and is already represented by the default grid. This is an implementation invariant, with no defect-fix or performance claim. |
| Tile-to-instance conversion | `GPUDrivenRendering/WFCGenerator.cpp:157-188` | EMPTY is skipped; non-empty tiles map to positioned, scaled, coloured cube/sphere instances. Scale sampling occurs in this conversion and the cache stores no instance array. |
| WFC presets, cache selection, and override bypass | `GPUDrivenRendering/GPUSceneManagement.cpp:205-228, 406-425` | Normal cache lookup uses grid, seed, and raw `density` preset label. Cube/sphere/empty weight overrides bypass the cache. |
| Cache key, payload, and provenance boundary | `GPUDrivenRendering/WFCCache.cpp:10-60`; `GPUDrivenRendering/WFCCache.h:11-20` | Filename key is grid/seed/preset. Binary payload is grid-size header plus tile IDs; loader checks grid size and short read. Seed, preset, commit, build identity, and checksum are not embedded in the binary. |
| Chunk construction and mesh ordering | `GPUDrivenRendering/GPUSceneManagement.cpp:428-479, 502-516` | Instances are bucketed by spatial chunk, stable-partitioned cube then sphere, and assigned contiguous offsets/counts and an AABB. |
| Split-SSBO host layouts | `GPUDrivenRendering/GPUSceneManagement.h:58-78, 212-223` | `CullingDatum` is 24 bytes, `RenderDatum` is 16 bytes, and `ChunkInfo` is 48 bytes. Host vectors remain distinct from GPU buffers. |
| Split-SSBO device memory and initial upload | `GPUDrivenRendering/GPUSceneManagement.cpp:519-584` | Culling/render/chunk buffers request device-local memory. Separate host-visible, host-coherent staging buffers are copied and submitted with a wait. The indirect buffer requests storage/indirect/transfer-destination usages and host-visible, host-coherent memory. |
| Shader consumers and shared instance indexing | `GPUDrivenRendering/Shaders/Scene.vert:9-31`; `GPUDrivenRendering/GPUSceneManagement.cpp:587-628` | Vertex shader reads per-instance culling/transform and colour buffers using `gl_InstanceIndex`; compute descriptors bind chunk metadata and indirect records. |
| Fixed indirect-command representation | `GPUDrivenRendering/GPUSceneManagement.cpp:539-543, 698-718, 739-746, 760-761, 809-816`; `GPUDrivenRendering/Tests/TestMain.cpp:188-200` | The implementation is a non-compacted indirect draw buffer with a fixed 2N-record layout. Each 20-byte command has five fields; cube/sphere streams use 40-byte stride and offsets 0/20. No compaction or indirect-count buffer is implemented. |
| S1 | `GPUDrivenRendering/GPUSceneManagement.cpp:636-688` | CPU frustum culling is followed by per-chunk direct indexed draw-command recording for each present mesh family. |
| S2 | `GPUDrivenRendering/GPUSceneManagement.cpp:690-753` | CPU frustum culling is followed by host population of all ten fields per chunk, a host-to-indirect barrier, and two recorded `vkCmdDrawIndexedIndirect` API commands; each command consumes N records. Invisible mesh records use `instanceCount=0`. |
| S3 host command sequence | `GPUDrivenRendering/GPUSceneManagement.cpp:755-823` | Buffer clear, transfer-to-compute barrier, compute dispatch, compute-to-indirect barrier, and the same two N-record MDI commands are recorded in order. |
| S3 compute work and exclusive writes | `GPUDrivenRendering/Shaders/Culling.comp:4-58`; `GPUDrivenRendering/GPUSceneManagement.cpp:775-786` | `local_size_x=64`; one invocation handles one chunk and writes ordinary stores into its exclusive two records. Dispatch is `ceil(N/64)`. Grid4096/chunk4 gives 1,048,576 invocations and 16,384 workgroups. No atomic operation is present. |
| S2 memory dependency | `GPUDrivenRendering/GPUSceneManagement.cpp:698-725` | Host-visible/coherent map-write-unmap is followed by host-write to indirect-command-read memory barrier recording. |
| S3 memory dependencies | `GPUDrivenRendering/GPUSceneManagement.cpp:759-795` | Transfer write is made visible to compute shader write after fill; compute shader write is made visible to indirect-command read before MDI. Both buffer barriers cover the indirect range. |
| Visibility readback boundary | `GPUDrivenRendering/GPUSceneManagement.cpp:259-275, 306-324` | S3 maps and scans the 2N records after `BeginFrame` and before rendering-path measurement. The O(N) visibility readback is outside `cpu_record`. |
| CPU timing boundary | `GPUDrivenRendering/GPUSceneManagement.cpp:640-651, 661-685, 694-726, 734-751, 759-796, 804-821, 825-860` | `cpu_record` is accumulated from several QPC segments. Swapchain acquire/fence wait is timed separately, so the metric is neither a continuous interval nor complete CPU frame cost. |
| GPU timestamp and wall-clock boundaries | `GPUDrivenRendering/GPUSceneManagement.cpp:247-257, 373-390, 825-840, 863-890` | Top/bottom timestamp queries cover each path command span and provide no stage breakdown. `frame_wall` spans `BeginMeasurement` to `EndMeasurement` only. |
| Local chunk selection and CPU modification | `GPUDrivenRendering/GPUSceneManagement.cpp:1021-1051` | Non-empty chunks are deterministically shuffled with `seed+9999`; host culling/render data modifications occur before the update timer. |
| Local-update timer and submission modes | `GPUDrivenRendering/GPUSceneManagement.cpp:1053-1137`; `GPUDrivenRendering/Main.cpp:116-157` | Timer includes staging allocation, memcpy, copy-command recording, submit, wait, and staging disposal. Batched uses one command buffer and submit/wait; reference mode uses one per chunk. CLI runs 10 warm-ups and 100 recorded standalone updates. |
| Benchmark CLI and rendering loop | `GPUDrivenRendering/Main.cpp:54-75, 82-95, 112-164` | CLI configures the executable; rendering benchmark uses fixed delta and update mode bypasses the render loop. |
| Formal automation and aggregation | `scripts/run_benchmarks.py:10-15, 17-118, 138-206` | Runner enumerates rendering/update arms, resumes from existing CSVs, records failures, and writes separate rendering and update aggregates. |
| Independent-process runner | `scripts/run_repeat_validation.py:26-81` | Selected regimes are executed in separately launched processes with explicit run indices. |
| C++ implementation tests | `GPUDrivenRendering/Tests/TestMain.cpp:28-267`; `GPUDrivenRendering/CMakeLists.txt:92-101` | Tests cover WFC determinism/rules/signatures, instance conversion, host layout sizes, and indirect stride without a Vulkan device. They do not exercise the Vulkan paths, cache provenance, barriers, or the EMPTY invariant as a dedicated assertion. |
| Evidence-pipeline tests | `scripts/tests/test_dissertation_evidence.py:20-178`; `scripts/dissertation_evidence.py:28-230` | Python regressions cover claim extraction keys, missing/duplicate rows, denominators, source-file provenance, and paired timestamps. They validate existing evidence handling and do not create benchmark observations. |
| Analysis/figure validation | `scripts/make_dissertation_figures.py:86-299, 445-552` | Existing artifacts are validated, filtered, and rendered with locked-claim and label checks. Check-only/self-test paths do not generate benchmark data. |

## Contribution ownership boundary

The approved project design identifies Vulkan 1.3 rendering, VMA memory management, mesh loading, and camera/uniform support as existing repository infrastructure (`docs/2026-05-15-gpu-driven-scene-management-design-cn.md:191-201`). Chapter 3 therefore keeps those facilities in the course/tutorial framework row. WFC/cache, GPU scene management, compute shader, CLI/automation, tests, and analysis are labelled repository-verifiable project-delivery scope. Repository history does not establish exclusive line-level authorship for every component, so the manuscript makes no sole-authorship claim.

## Claim boundaries carried into Design and Implementation

- S1/S2 and S2/S3 remain whole-rendering-path comparisons because their host work, mappings, writes, clears, barriers, dispatches, and readback boundaries differ.
- Timestamp-query GPU elapsed time spans combined commands and cannot identify a responsible stage.
- S3 visibility readback is outside CPU preparation-and-command-recording time.
- Local-update time is an isolated upload-path timer; CPU data modification precedes it.
- Chapter 3 records mechanisms and limitations only. Empirical observations remain governed by `claim-ledger.md`.

## External manuscript checkpoint

- Chapter: `# 3. Design and Implementation`
- Completion date: 2026-07-21
- External manuscript: `D:\\D-Code\\Code-Essay\\thesis.md`
- SHA-256: `50727D4A807F4069BB26AF8E1E7FC2DC4E4B6811721F8688A24D62C2B100A4A7`
