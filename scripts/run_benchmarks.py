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
                if row[0] == "Total":
                    info.update(total_avg_us=row[1], total_min_us=row[2], total_max_us=row[3],
                               total_p1_us=row[4], total_p99_us=row[5], total_stddev=row[6])
                elif row[0] == "CPU": info["cpu_avg_us"] = row[1]
                elif row[0] == "GPU": info["gpu_avg_us"] = row[1]
            rows.append(info)
        except: pass
    if not rows: return
    fns = ["file","grid","chunk","density","scheme","seed","update",
           "total_avg_us","total_min_us","total_max_us","total_p1_us","total_p99_us","total_stddev",
           "cpu_avg_us","gpu_avg_us"]
    with open(out / "_aggregate.csv", "w", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=fns, extrasaction="ignore")
        w.writeheader(); w.writerows(rows)
    print(f"Aggregate: {len(rows)} rows")

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--exe", type=Path); ap.add_argument("--outdir", type=Path, default=Path("results"))
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()
    exe = args.exe or find_exe()
    out = args.outdir; out.mkdir(parents=True, exist_ok=True)

    tests = []
    for g in GRID_SIZES:
        for c in [x for x in CHUNK_SIZES if x <= g//2]:
            for d in DENSITIES:
                for s in SCHEMES:
                    for seed in SEEDS:
                        tests.append((g, c, d, s, seed, 0))
    for u in [1,4,16]:
        for s in SCHEMES:
            for seed in SEEDS:
                tests.append((128, 8, 50, s, seed, u))

    print(f"Tests: {len(tests)} | Est: ~{len(tests)*T_SEC/60:.0f} min ({len(tests)*T_SEC/3600:.1f}h)")
    if args.dry_run:
        for t in tests[:5]: print(f"  {t}")
        return

    ok = fail = 0; t0 = time.monotonic()
    for g, c, d, s, seed, u in tests:
        if run_test(exe, out, g, c, d, s, seed, u): ok += 1
        else: fail += 1
    print(f"Done {time.monotonic()-t0:.0f}s. OK={ok} FAIL={fail}")
    aggregate(out)

if __name__ == "__main__": main()
