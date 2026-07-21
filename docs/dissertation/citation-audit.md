# Dissertation citation audit

Audit date: 2026-07-21  
Audited artifact: `D:\D-Code\Code-Essay\thesis.md`  
Audited manuscript SHA-256: `B803CDE67213B21CA6045C6ADE4D538F87CE97E27640ABC0976C6B3D2CE3A47C`
Rewrite scope: Chapter 2 and the reference list  
Source policy: primary papers, publisher or institutional records, Khronos documentation, and the original WFC repository

## Outcome

The rewritten Chapter 2 uses six sections and 17 reference keys. The reference list contains the same 17 keys. Chapter-2-to-reference exact-key closure reports zero orphan citations and zero unused references. A separate whole-manuscript scan finds two legacy year-only forms in unchanged chapters: Unterguggenberger et al. `[2021]` and Haar and Aaltonen `[2015]`. Their full sources are present in the reference list, but the bracket strings are not full citation keys; Task 8 must normalise them. RayBench, RenderBench, and GPU Zen 3 were removed because they did not directly support the revised argument. All source-register rows below end in `verified`, `reworded`, or `removed`.

The audit corrects the earlier Vulkan version error, the Aokana venue, the Xylem compute-route description, the Gonakhchyan evidence boundary, and the configured-versus-observed proportions in Unterguggenberger et al. The second review pass also corrects indirect-count semantics, consolidates the whole-path inference boundary, and pins the WFC repository to an identified revision. It removes literature-exclusivity claims and uses official sources for direct drawing, indirect drawing, indirect-count drawing, compute dispatch, timestamp queries, Vulkan 1.3 core promotions, and the Khronos MDI sample.

## Final source register

| Citation key | Primary or official record checked | Claim retained in Chapter 2 | Status |
|---|---|---|---|
| Gumin 2016 | Original `mxgmn/WaveFunctionCollapse` repository, revision `de7d22e705e816b62b4d613199d0463820fcaef3` dated 22 March 2026 | Observation, propagation, Shannon entropy, contradiction, and simple tiled model terminology | reworded |
| Karth & Smith 2022 | IEEE Transactions on Games record and author manuscript; DOI 10.1109/TG.2021.3076368 | WFC as a family of content-generation methods interpretable through constraint solving and machine learning | verified |
| Haar & Aaltonen 2015 | Official SIGGRAPH course slides | Pipeline stages and the Xbox One 250,000-object benchmark values | reworded |
| Fang et al. 2025 | ACM DOI record and author paper; DOI 10.1145/3728299 | SVDAG chunks, LOD, streaming, and Unity 6 RenderGraph implementation with Vulkan as the rendering backend | reworded |
| Unterguggenberger et al. 2021 | Eurographics/TU Wien author paper; DOI 10.1111/cgf.14401 | Separate configured backface-cullable mesh share, observed culled meshlet share, and relative performance change | reworded |
| Li et al. 2023 | Elsevier article record; DOI 10.1016/j.displa.2023.102533 | GPU-resident hybrid occlusion culling, one indirect multidraw command, and no CPU readback | verified |
| Galajda 2020 | Czech Technical University institutional thesis PDF | Three-platform API comparison and the 3.10--4.89 ratio in the one-million-draw single-threaded test | reworded |
| Gonakhchyan 2018 | IADIS paper page and official proceedings contents | Per-frame, pre-recorded, and hierarchy-node command-buffer strategies | reworded |
| Sundararaman 2026 | Cal Poly repository record and thesis PDF | Donut/NVRHI with D3D12, traditional route, compute-driven GPU culling plus indirect rendering, and meshlet route | reworded |
| Kalibera & Jones 2013 | University of Kent repository and corrected author manuscript; DOI 10.1145/2464157.2464160 | Hierarchical variation and repetition at iteration, execution, and compilation levels | verified |
| Khronos-vkCmdDrawIndexed n.d. | Khronos Vulkan Reference Pages | `vkCmdDrawIndexed` is a Vulkan 1.0 core command whose draw parameters are host call arguments | verified |
| Khronos-vkCmdDrawIndexedIndirect n.d. | Khronos Vulkan Reference Pages | Vulkan 1.0 availability, device reads from a command buffer, and `drawCount` records per API command | verified |
| Khronos-vkCmdDispatch n.d. | Khronos Vulkan Reference Pages | `vkCmdDispatch` is a Vulkan 1.0 core command | verified |
| Khronos-vkCmdDrawIndexedIndirectCount n.d. | Khronos Vulkan Reference Pages | Vulkan 1.2 core availability, `VK_KHR_draw_indirect_count` alias, device-sourced count, and `maxDrawCount` boundary | verified |
| Khronos-MDI-Sample n.d. | Khronos Vulkan Samples documentation | Fixed records can toggle `instanceCount` between 0 and 1; an alternative removes the command from the array | verified |
| Khronos-vkCmdWriteTimestamp2 n.d. | Khronos Vulkan Reference Pages | Vulkan 1.3 core availability, timestamp write, and stage-scoped execution dependency | verified |
| Khronos-Vulkan-versions n.d. | Khronos Vulkan Specification, Core Revisions | Dynamic rendering and synchronization2 were promoted into Vulkan 1.3 core | verified |
| Wang & Yu 2023a | MDPI publisher page; DOI 10.3390/electronics12194124 | No claim retained after the Chapter 2 rewrite | removed |
| Wang & Yu 2023b | MDPI publisher page; DOI 10.3390/electronics12194153 | No claim retained after the Chapter 2 rewrite | removed |
| Engel 2024 | Available book metadata and contents records | No claim retained because editor/publisher metadata and the prior cross-chapter benchmark statement were not fully supported | removed |

## Metadata corrections

| Key | Earlier record | Audited record | Status |
|---|---|---|---|
| Fang et al. 2025 | *ACM Transactions on Graphics (SIGGRAPH Asia 2025)* | *Proceedings of the ACM on Computer Graphics and Interactive Techniques* 8(1), Article 10, 1--17; I3D 2025 | reworded |
| Fang et al. 2025 implementation layer | Wording could be read as a native Vulkan framework | Unity 6 RenderGraph API implementation with Vulkan at the rendering-backend layer | reworded |
| Gumin 2016 repository state | Floating repository URL with only the original-release year | Originally released in 2016; cited content pinned to revision `de7d22e705e816b62b4d613199d0463820fcaef3`, dated 22 March 2026 | reworded |
| Gonakhchyan 2018 | Abbreviated proceedings entry with unverified 55.5 ms and 0.03 ms values | Full IADIS conference title, pages 397--402, IADIS Press, ISBN 978-989-8533-79-1; unsupported timings removed | reworded |
| Sundararaman 2026 | Compute route described as lacking GPU compute culling | Compute route described as GPU instance frustum/Hi-Z culling plus indirect rendering within Donut/NVRHI using D3D12 | reworded |
| Unterguggenberger et al. 2021 | Configured cullable share blended with observed culled share | 0/20/100% configured backface-cullable mesh share separated from 0.0/3.8/15.7% observed culled meshlet share | reworded |
| Vulkan command versions | Indirect drawing and compute dispatch attributed to Vulkan 1.3 | Direct indexed draw, indexed indirect draw, and dispatch identified as Vulkan 1.0; dynamic rendering, synchronization2, and `vkCmdWriteTimestamp2` identified with Vulkan 1.3 | reworded |

## Claim decisions

The revised command-recording discussion distinguishes an API command recorded in a Vulkan command buffer from the N indirect-command records consumed by that API command. It also distinguishes the implemented non-compacted fixed 2N-record layout from a compacted command buffer and count-based drawing. Count-based drawing does not require compaction: valid records may already occupy the first K positions while a count buffer supplies K. Compaction is one way to form that prefix and entails additional count maintenance and record reordering. Source inspection confirms that S2 and S3 each record two `vkCmdDrawIndexedIndirect` API commands, each traverses N records, invisible records use `instanceCount=0`, and each S3 invocation writes its two assigned records without atomics. Status: `reworded`.

The revised WFC discussion uses Gumin's terminology while preserving the implementation boundary. The project selects a cell by candidate count rather than Shannon entropy and has no tile rotation, backtracking, or contradiction recovery. The catalogue is EMPTY plus three cube tiles and two sphere tiles. Source inspection additionally confirms the propagation control flow: a neighbour whose candidate set shrinks but remains non-singleton only receives a new heap entry, while recursive propagation continues when the neighbour becomes a singleton. The chapter treats WFC as workload generation and makes no repair or optimisation claim. Status: `reworded`.

The revised methodology distinguishes within-execution frame samples from independent process executions using Kalibera and Jones. It defines `gpu_exec` as a top-to-bottom timestamp-query span and states that the available queries cannot produce stage-level attribution. Status: `verified`.

The revised positioning makes no field-wide absence, priority, or exclusivity claim. It describes an implementation-specific comparison of three Vulkan rendering paths under a shared procedural workload and representation, plus a standalone update arm. It also lists the host mapping, record population, clear, dispatch, barrier, indirect processing, and readback differences that prevent clean single-stage causal attribution. Status: `reworded`.

## Citation and reference closure

The final reference keys are:

`Fang et al. 2025`; `Galajda 2020`; `Gonakhchyan 2018`; `Gumin 2016`; `Haar & Aaltonen 2015`; `Kalibera & Jones 2013`; `Karth & Smith 2022`; `Khronos-MDI-Sample n.d.`; `Khronos-Vulkan-versions n.d.`; `Khronos-vkCmdDispatch n.d.`; `Khronos-vkCmdDrawIndexed n.d.`; `Khronos-vkCmdDrawIndexedIndirect n.d.`; `Khronos-vkCmdDrawIndexedIndirectCount n.d.`; `Khronos-vkCmdWriteTimestamp2 n.d.`; `Li et al. 2023`; `Sundararaman 2026`; `Unterguggenberger et al. 2021`.

Closure result for Chapter 2 plus References: 17 distinct in-text keys, 17 reference-list keys, zero orphan Chapter 2 keys, and zero unused reference entries. Living Khronos records use `n.d.` and carry an access date of 21 July 2026; the WFC repository entry states its 2016 original release and pins the audited 22 March 2026 revision. The full-manuscript scan additionally reports the two legacy year-only bracket forms described above; they are deferred because Chapters 1 and 3--7 are byte-preserved in Task 7.

## Residual cross-chapter risks outside the Task 7 write boundary

The byte-preservation requirement left Chapters 1 and 3--7 unchanged. Several pre-existing sentences there still use wording that this audit does not support: Chapter 1 attributes project-specific generation duration and scale to Gumin, attributes Vulkan 1.0 commands to Vulkan 1.3, and contains field-wide gap language; Chapter 6 reverses or compresses parts of the Unterguggenberger comparison and uses the legacy year-only form `[2021]`; Chapter 7 generalises Haar and Aaltonen beyond the documented 250,000-object benchmark and uses the legacy year-only form `[2015]`. These sentences are recorded for later whole-manuscript revision. They do not change the Chapter 2 source decisions or its 17/17 reference closure.

## Retraction and access note

The checked publisher, institutional, specification, sample, and repository pages showed no retraction or expression-of-concern notice on 21 July 2026. This is a page-level verification, not a database-wide certification. Final ACM ordering and BibTeX conversion remain part of the later English-formatting stage.
