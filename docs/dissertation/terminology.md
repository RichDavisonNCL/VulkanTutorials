# Dissertation terminology

Use these terms throughout the Chinese master manuscript. Existing code, CLI, and CSV field names remain unchanged; give their mapping at first use.

| Current ambiguous term or raw label | Dissertation term |
|---|---|
| density 20/50/80% | tile-weight preset 20/50/80; report observed non-empty-tile proportion separately |
| scheme 1/2/3 | rendering path S1/S2/S3; use `scheme` only for code, CLI, or CSV fields |
| CPU frame time | CPU preparation-and-command-recording time (`cpu_record`, a project-defined composite metric) |
| GPU execution time | GPU elapsed time measured with timestamp queries (`gpu_exec`) |
| fixed-length indirect records | non-compacted indirect draw buffer with a fixed 2N-record layout |
| two draw calls | two recorded `vkCmdDrawIndexedIndirect` commands; each command executes N indirect draws |
| one workgroup per chunk | one invocation per chunk; 64 invocations per workgroup |
| atomic write | ordinary store to an exclusive indirect-command record |
| occupancy (scene-fill meaning) | observed non-empty-tile proportion |
| geometry composition | scene mesh composition |
| within-run samples/statistics | within-execution frame samples/variation |
| independent-process repeat validation | independent process executions; execution-level repetitions/variation |
| pipeline bottleneck | observed CPU/GPU timing trade-off |
| GPU culling saturation boundary | observed fine-granularity stress-test configuration/result |
| standard/full WFC | simplified WFC implementation |
| standalone buffer-upload test | standalone buffer-update microbenchmark |
| scene representation comparison | shared representation and standalone update-path evaluation |

## Usage boundaries

- A tile-weight preset is an input label. It does not state the observed non-empty-tile proportion.
- `occupancy` remains reserved for GPU occupancy contexts.
- S1 is the CPU-frustum-culling direct-draw path; S2 is the CPU-frustum-culling MDI path; S3 is the compute-frustum-culling MDI path.
- The formal matrix, selected independent process executions, and supplementary sweeps are separate datasets and must retain separate labels in prose, tables, and figures.
