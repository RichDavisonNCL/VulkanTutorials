| Mechanism or metric | Source anchor |
|---|---|
| S1 CPU culling and direct draws | `GPUDrivenRendering/GPUSceneManagement.cpp:636-688` |
| S2 host-populated indirect draw buffer and two MDI commands | `GPUDrivenRendering/GPUSceneManagement.cpp:690-753` |
| S3 fill, barriers, compute dispatch and MDI | `GPUDrivenRendering/GPUSceneManagement.cpp:755-823` |
| `cpu_record` segment accumulation | `GPUDrivenRendering/GPUSceneManagement.cpp:640-651, 661-685, 694-726, 734-751, 759-796, 804-821, 842-855` |
| timestamp-query start | `GPUDrivenRendering/GPUSceneManagement.cpp:825-840` |
| EMPTY catalogue and adjacency invariant | `GPUDrivenRendering/WFCGenerator.cpp:12-28` |
| output grid default EMPTY representation | `GPUDrivenRendering/WFCGenerator.cpp:38-56` |
| propagation singleton behavior | `GPUDrivenRendering/WFCGenerator.cpp:112-155` |

# Source map

Anchors were checked against the current worktree at baseline `001b215159709490b7d1beec1a631d282351df93`. `GPUDrivenRendering/GPUSceneManagement.cpp` had unstaged modifications that pre-existed the Task 1 control-layer commit `c68017a5997563fbd34b9a6461d19bcdc779f8e3`. These anchors describe that current source and must be rechecked before manuscript claims cite them.

## Measurement controls

| Topic | Verified source interpretation | Claim boundary |
|---|---|---|
| `cpu_record` | `CpuSegBegin`/`CpuSegEnd` accumulates several CPU segments; swapchain acquire fence wait is recorded separately. | CPU preparation-and-command-recording time is not a continuous interval or complete CPU frame cost. |
| Timestamp queries | Start query is written at top of pipe in `BeginMeasurement`; end query is written at bottom of pipe in `EndMeasurement` (`GPUDrivenRendering/GPUSceneManagement.cpp:863-871`). | GPU elapsed time measured with timestamp queries covers the path command span and cannot isolate a stage. |
| S2 commands | The host writes two indirect records per chunk and records two `drawIndexedIndirect` commands. | S1/S2 is a whole-rendering-path comparison. |
| S3 commands | The path fills the fixed indirect buffer, records barriers and dispatch, then records two indirect-draw commands. | The stress-test result has no single-stage causal attribution. |

## WFC invariant control

The output grid starts with EMPTY tile ID 0. EMPTY is compatible with every catalogue tile. Propagation therefore preserves EMPTY in each possibility set; a singleton produced through propagation is `{EMPTY}`, already represented by the grid default. This is a current compatibility invariant, with no performance claim and no software fix recorded by this control layer.
