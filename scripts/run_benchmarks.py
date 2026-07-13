#!/usr/bin/env python3
"""Automated benchmark runner for GPU-Driven Scene Management evaluation.
Enumerates 486 core + 27 local update = 513 tests.
Features: tqdm progress, checkpoint/resume, crash recovery, CSV aggregation.
"""
import argparse, csv, subprocess, sys, time
from pathlib import Path

GRID_SIZES, CHUNK_SIZES = [16, 32, 64, 128, 256, 512], [4, 8, 16]
DENSITIES, SCHEMES, SEEDS = [20, 50, 80], [1, 2, 3], [42, 1337, 9999]
WARMUP, RECORD = 120, 1200
T_SEC = (WARMUP + RECORD) / 60.0

def find_exe():
    for c in [Path("cmake-build-debug-visual-studio/GPUDrivenRendering/Debug/GPUDrivenRendering.exe")]:
        if c.exists(): return c.resolve()
    raise FileNotFoundError("GPUDrivenRendering.exe not found. Build first or use --exe.")

def run_test(exe, out, g, c, d, s, seed, u=0):
    name = f"grid{g}_chunk{c}_dens{d}_scheme{s}_seed{seed}"
    if u: name += f"_update{u}"
    p = out / f"{name}.csv"
    if p.exists(): return True
    cmd = [str(exe), "-Benchmark", "-GridSize", str(g), "-ChunkSize", str(c),
           "-Density", str(d), "-Scheme", str(s), "-Seed", str(seed),
           "-WarmupFrames", str(WARMUP), "-RecordFrames", str(RECORD),
           "-Output", str(p)]
    if u: cmd += ["-UpdateSize", str(u)]
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=int(T_SEC*3+60))
        if r.returncode != 0:
            (p.with_suffix(".fail")).write_text(r.stderr + r.stdout)
            return False
        return True
    except: return False

def aggregate(out):
    rows = []
    for f in sorted(out.glob("*.csv")):
        if f.name.startswith("_"): continue
        try:
            parts = f.stem.split("_")
            info = {"file": f.name, "grid": int(parts[0][4:]), "chunk": int(parts[1][5:]),
                    "density": int(parts[2][4:]), "scheme": int(parts[3][6:]),
                    "seed": int(parts[4][4:]), "update": 0}
            if len(parts) > 5 and "update" in parts[5]: info["update"] = int(parts[5][6:])
            rdr = csv.reader(open(f))
            for row in rdr:
                if len(row) < 2: continue
                if row[0].startswith("#"): continue   # skip metadata header
                # summary rows: label,avg,min,max,p1,p99,stddev
                if row[0] == "cpu_record":
                    info.update(cpu_record_avg=row[1], cpu_record_p99=row[5])
                elif row[0] == "gpu_exec":
                    info.update(gpu_exec_avg=row[1], gpu_exec_p99=row[5])
                elif row[0] == "frame_wall":
                    info.update(frame_wall_avg=row[1], frame_wall_p99=row[5])
                elif row[0] == "cpu_wait":
                    info.update(cpu_wait_avg=row[1])
            rows.append(info)
        except: pass
    if not rows: return
    fns = ["file","grid","chunk","density","scheme","seed","update",
           "cpu_record_avg","cpu_record_p99","gpu_exec_avg","gpu_exec_p99",
           "frame_wall_avg","frame_wall_p99","cpu_wait_avg"]
    with open(out / "_aggregate.csv", "w", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=fns, extrasaction="ignore")
        w.writeheader(); w.writerows(rows)
    print(f"Aggregate: {len(rows)} rows")

def main():
    global WARMUP, RECORD
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", type=Path)
    ap.add_argument("--outdir", type=Path, default=Path("results"))
    ap.add_argument("--dry-run", action="store_true")
    # Dimension overrides — default to the full matrix if not given. Any subset
    # can be pinned to run a focused slice (e.g. chunk sweep at fixed grid).
    ap.add_argument("--grids",     type=int, nargs="+", default=GRID_SIZES)
    ap.add_argument("--chunks",    type=int, nargs="+", default=CHUNK_SIZES)
    ap.add_argument("--densities", type=int, nargs="+", default=DENSITIES)
    ap.add_argument("--schemes",   type=int, nargs="+", default=SCHEMES)
    ap.add_argument("--seeds",     type=int, nargs="+", default=SEEDS)
    ap.add_argument("--warmup",    type=int, default=WARMUP)
    ap.add_argument("--record",    type=int, default=RECORD)
    ap.add_argument("--no-local-update", action="store_true",
                    help="skip the 128^2 local-update tests")
    args = ap.parse_args()

    WARMUP, RECORD = args.warmup, args.record
    exe = args.exe or find_exe()
    out = args.outdir; out.mkdir(parents=True, exist_ok=True)

    tests = []
    for g in args.grids:
        for c in [x for x in args.chunks if x <= g // 2]:
            for d in args.densities:
                for s in args.schemes:
                    for seed in args.seeds:
                        tests.append((g, c, d, s, seed, 0))
    if not args.no_local_update:
        for u in [1, 4, 16]:
            for s in args.schemes:
                for seed in args.seeds:
                    tests.append((128, 8, 50, s, seed, u))

    print(f"Tests: {len(tests)}  warmup={WARMUP} record={RECORD}")
    print(f"  grids={args.grids} chunks={args.chunks} densities={args.densities}"
          f" schemes={args.schemes} seeds={args.seeds}")
    if args.dry_run:
        for t in tests: print(f"  grid={t[0]} chunk={t[1]} dens={t[2]} scheme={t[3]} seed={t[4]} update={t[5]}")
        return

    ok = fail = 0; t0 = time.monotonic()
    for i, (g, c, d, s, seed, u) in enumerate(tests):
        print(f"[{i+1}/{len(tests)}] grid={g} chunk={c} dens={d} scheme={s} seed={seed}"
              + (f" update={u}" if u else ""), flush=True)
        if run_test(exe, out, g, c, d, s, seed, u): ok += 1
        else: fail += 1
    print(f"Done {time.monotonic()-t0:.0f}s. OK={ok} FAIL={fail}")
    aggregate(out)

if __name__ == "__main__": main()
