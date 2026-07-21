# Claim ledger

This ledger locks the direct observations used by the dissertation. Later work must update this document before changing a numerical claim, its evidence, or its limitation.

| ID | Direct observation | Evidence | Required limitation |
|---|---|---|---|
| C1 | 135 matched configurations at grid≥256 give S1/S2 `cpu_record_avg` ratios of 1.7366–9.9405× | `results/_aggregate.csv`; key = grid, chunk, density, seed | whole-path comparison; no isolated MDI causal attribution |
| C2 | grid4096/chunk4/preset50/seed42 gives S3 CPU time 44.745µs and GPU elapsed time 236,173µs | `results/_aggregate.csv` | timestamp span combines fill, barriers, dispatch, indirect processing and graphics; readback is outside `cpu_record` |
| C3 | grid4096/chunk16/preset80/S3 gives seed9999 600,999µs versus seed42/1337 52,879.3/52,910.9µs | formal CSV plus cache histogram and separately labelled supplementary sweep | association with scene mesh composition; no single-stage attribution |
| C4 | 32-chunk update gives batched 87.07µs, per-chunk 1,898.44µs and ratio 21.80× | `results/_aggregate_update.csv` | standalone buffer-update microbenchmark; no full dynamic-frame claim |

## Claim-use controls

- Refer to S1, S2, and S3 as rendering paths. Reserve `scheme` for source code, CLI, or CSV fields.
- Name `cpu_record` as CPU preparation-and-command-recording time. It is a project-defined accumulated CPU metric.
- Name `gpu_exec` as GPU elapsed time measured with timestamp queries. It supports no single-stage attribution.
- Name `frame_wall` as a project wall-clock diagnostic. Its partial boundary excludes work outside the recorded span, so it is not complete end-to-end frame time and must not support a complete throughput claim.
- Treat C1 and C2 as formal-matrix observations. Treat C3 cache interpretation and its supplementary sweep as separately labelled evidence. Treat C4 as a standalone buffer-update microbenchmark observation.
- Record both within-execution frame variation and execution-level variation where available. Do not use selected independent process executions as repetitions for the formal matrix.
