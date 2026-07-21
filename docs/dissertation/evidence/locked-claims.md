# Locked dissertation evidence

Generated UTC: 2026-07-21T04:40:34.105641Z

Source files: `results/_aggregate.csv`, `results/_aggregate_update.csv`

## C1 — S1/S2 CPU preparation-and-command-recording comparison

Raw extracted values: 135 matched rendering-path pairs; ratio range 1.7366206011401966 to 9.94048076184625.

Rounded prose value: Across 135 matched configurations at grid ≥256, S1/S2 `cpu_record_avg` ratios span 1.7366–9.9405×. This is a whole-rendering-path comparison.

## C2 — Fine-granularity stress-test configuration

Raw extracted values: S1 CPU/GPU 44540.6/38723.6 µs; S2 CPU/GPU 7422.7/44730.5 µs; S3 CPU/GPU 44.7452/236173.0 µs.

Rounded prose value: At grid4096/chunk4/tile-weight preset50/seed42, S3 records 44.745µs CPU preparation-and-command-recording time and 236,173µs GPU elapsed time measured with timestamp queries.

## C3 — Scene mesh composition and GPU workload

Raw extracted values: seed42 52879.3 µs; seed1337 52910.9 µs; seed9999 600999.0 µs; ratios 11.365487062045071 and 11.35869924722505.

Rounded prose value: At grid4096/chunk16/tile-weight preset80 on S3, seed9999 records 600,999µs GPU elapsed time, 11.37× seed42 and 11.36× seed1337. This is a configuration observation.

## C4 — Buffer-update submission granularity

Raw extracted values: batched mean 87.06644444444444 µs; per-chunk mean 1898.4366666666667 µs; ratio 21.804458408520695.

Rounded prose value: For the 32-chunk standalone buffer-update microbenchmark across all seeds and rendering paths, batched submit-and-wait records 87.07µs and the per-chunk reference path records 1,898.44µs, a 21.80× ratio.
