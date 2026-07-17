#!/usr/bin/env python3
"""Automated benchmark runner for GPU-Driven Scene Management evaluation.
Enumerates 702 render + 108 local-update = 810 tests (9 grids x 3 chunks x 3
densities x 3 schemes x 3 seeds, minus chunk>grid/2; plus update arm).
Features: per-config timeout, checkpoint/resume, failure recording, CSV aggregation.
"""
import argparse, csv, subprocess, sys, time
from pathlib import Path

GRID_SIZES, CHUNK_SIZES = [16, 32, 64, 128, 256, 512, 1024, 2048, 4096], [4, 8, 16]
DENSITIES, SCHEMES, SEEDS = [20, 50, 80], [1, 2, 3], [42, 1337, 9999]
WARMUP, RECORD = 120, 1200
T_SEC = (WARMUP + RECORD) / 60.0
# Local-update arm (standalone RegenerateChunks cost), fixed 128^2/chunk8/dens50.
UPDATE_SIZES, UPDATE_MODES = [1, 2, 4, 8, 16, 32], ["batched", "perchunk"]

def est_timeout(g, c, u):
    """Per-config subprocess timeout (s). Cost has TWO drivers, both matter:
      - CPU cull/draw recording ~ chunk count = (grid/chunk)^2  (small chunks slow)
      - GPU exec ~ instance count ~ grid^2 * density              (large dense grids slow)
    The second was learned the hard way: seed9999/dens80/4096/chunk16 hit 614ms/frame
    (~811s full run) despite having the FEWEST chunks — dense scenes pack more
    instances per chunk, and GPU work scales with instances, not chunks. A
    chunk-count-only formula under-sized it (283s) and dropped it. So take the max
    of a chunk term and a grid-area term. Over-waiting is harmless (timeout is a
    ceiling; healthy runs return early); under-waiting silently drops data."""
    if u:
        return 120                      # standalone update pilot: tiny, seconds
    frames = WARMUP + RECORD
    chunks = (g / c) ** 2               # CPU cost driver
    area = g * g                        # instance/GPU cost driver
    chunk_term = frames * chunks * 1.2e-6
    area_term  = frames * area   * 4.0e-7   # 4096^2*1320*4e-7 ~= 8850s cap-bound; gives
                                            # 4096 grids generous room regardless of chunk
    secs = 180 + max(chunk_term, area_term)
    return int(min(secs, 5400))         # cap 90min guards a genuinely hung run

def find_exe():
    for c in [Path("cmake-build-debug-visual-studio/GPUDrivenRendering/Debug/GPUDrivenRendering.exe")]:
        if c.exists(): return c.resolve()
    raise FileNotFoundError("GPUDrivenRendering.exe not found. Build first or use --exe.")

def run_test(exe, out, g, c, d, s, seed, u=0, batched=True):
    name = f"grid{g}_chunk{c}_dens{d}_scheme{s}_seed{seed}"
    if u: name += f"_update{u}_{'batched' if batched else 'perchunk'}"
    p = out / f"{name}.csv"
    if p.exists(): return True
    cmd = [str(exe), "-Benchmark", "-GridSize", str(g), "-ChunkSize", str(c),
           "-Density", str(d), "-Scheme", str(s), "-Seed", str(seed),
           "-Output", str(p)]
    if u:
        # Local update is a standalone measurement (RegenerateChunks in isolation);
        # no render-frame flags. The exe runs the pilot and exits after writing CSV.
        cmd += ["-UpdateSize", str(u), "-updatebatched", "1" if batched else "0"]
    else:
        cmd += ["-WarmupFrames", str(WARMUP), "-RecordFrames", str(RECORD)]
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=est_timeout(g, c, u))
        if r.returncode != 0:
            p.with_suffix(".fail").write_text(f"exit={r.returncode}\n\n{r.stderr}\n{r.stdout}")
            return False
        return True
    except subprocess.TimeoutExpired as e:
        # Explicit: record the timeout and remove any partial CSV so a rerun retries it.
        # (Previously a bare `except` swallowed this, leaving no CSV and no marker.)
        if p.exists(): p.unlink()
        p.with_suffix(".fail").write_text(
            f"TIMEOUT after {e.timeout}s\ncmd={' '.join(map(str, cmd))}\n")
        print(f"    TIMEOUT ({e.timeout}s) — recorded .fail", flush=True)
        return False
    except Exception as ex:
        p.with_suffix(".fail").write_text(f"EXCEPTION: {ex!r}\ncmd={' '.join(map(str, cmd))}\n")
        return False

def aggregate(out):
    # Two arms produce different metrics — route to separate clean tables.
    # Render: grid{G}_chunk{C}_dens{D}_scheme{S}_seed{SEED}.csv
    # Update: grid{G}_chunk{C}_dens{D}_scheme{S}_seed{SEED}_update{U}_{batched|perchunk}.csv
    render_rows, update_rows = [], []
    for f in sorted(out.glob("*.csv")):
        if f.name.startswith("_"): continue
        try:
            parts = f.stem.split("_")
            base = {"file": f.name, "grid": int(parts[0][4:]), "chunk": int(parts[1][5:]),
                    "density": int(parts[2][4:]), "scheme": int(parts[3][6:]),
                    "seed": int(parts[4][4:])}
            is_update = len(parts) > 5 and parts[5].startswith("update")
            rdr = csv.reader(open(f))
            if is_update:
                info = dict(base, update_size=int(parts[5][6:]),
                            mode=parts[6] if len(parts) > 6 else "")
                for row in rdr:
                    if len(row) < 2 or row[0].startswith("#"): continue
                    if row[0] == "update_cost":
                        info.update(update_cost_avg=row[1], update_cost_p99=row[5],
                                    update_cost_stddev=row[6])
                update_rows.append(info)
            else:
                info = dict(base, update=0)
                for row in rdr:
                    if len(row) < 2 or row[0].startswith("#"): continue
                    # summary rows: label,avg,min,max,p1,p99,stddev
                    if row[0] == "cpu_record":
                        info.update(cpu_record_avg=row[1], cpu_record_p99=row[5])
                    elif row[0] == "gpu_exec":
                        info.update(gpu_exec_avg=row[1], gpu_exec_p99=row[5])
                    elif row[0] == "frame_wall":
                        info.update(frame_wall_avg=row[1], frame_wall_p99=row[5])
                    elif row[0] == "cpu_wait":
                        info.update(cpu_wait_avg=row[1])
                render_rows.append(info)
        except: pass
    if render_rows:
        fns = ["file","grid","chunk","density","scheme","seed","update",
               "cpu_record_avg","cpu_record_p99","gpu_exec_avg","gpu_exec_p99",
               "frame_wall_avg","frame_wall_p99","cpu_wait_avg"]
        with open(out / "_aggregate.csv", "w", newline="") as fh:
            w = csv.DictWriter(fh, fieldnames=fns, extrasaction="ignore")
            w.writeheader(); w.writerows(render_rows)
        print(f"Aggregate (render): {len(render_rows)} rows")
    if update_rows:
        fns = ["file","grid","chunk","density","scheme","seed","update_size","mode",
               "update_cost_avg","update_cost_p99","update_cost_stddev"]
        with open(out / "_aggregate_update.csv", "w", newline="") as fh:
            w = csv.DictWriter(fh, fieldnames=fns, extrasaction="ignore")
            w.writeheader(); w.writerows(update_rows)
        print(f"Aggregate (update): {len(update_rows)} rows")

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
    ap.add_argument("--no-render", action="store_true",
                    help="skip the render matrix (run only the local-update arm)")
    ap.add_argument("--update-sizes", type=int, nargs="+", default=UPDATE_SIZES)
    ap.add_argument("--update-modes", nargs="+", default=UPDATE_MODES,
                    choices=["batched", "perchunk"])
    args = ap.parse_args()

    WARMUP, RECORD = args.warmup, args.record
    exe = args.exe or find_exe()
    out = args.outdir; out.mkdir(parents=True, exist_ok=True)

    # Each test: (grid, chunk, density, scheme, seed, update_size, batched)
    tests = []
    if not args.no_render:
        for g in args.grids:
            for c in [x for x in args.chunks if x <= g // 2]:
                for d in args.densities:
                    for s in args.schemes:
                        for seed in args.seeds:
                            tests.append((g, c, d, s, seed, 0, True))
    if not args.no_local_update:
        for u in args.update_sizes:
            for mode in args.update_modes:
                for s in args.schemes:
                    for seed in args.seeds:
                        tests.append((128, 8, 50, s, seed, u, mode == "batched"))

    print(f"Tests: {len(tests)}  warmup={WARMUP} record={RECORD}")
    print(f"  grids={args.grids} chunks={args.chunks} densities={args.densities}"
          f" schemes={args.schemes} seeds={args.seeds}")
    print(f"  update_sizes={args.update_sizes} update_modes={args.update_modes}")
    if args.dry_run:
        for t in tests:
            tag = (f" update={t[5]} mode={'batched' if t[6] else 'perchunk'}" if t[5] else "")
            print(f"  grid={t[0]} chunk={t[1]} dens={t[2]} scheme={t[3]} seed={t[4]}{tag}")
        return

    ok = fail = 0; t0 = time.monotonic()
    for i, (g, c, d, s, seed, u, batched) in enumerate(tests):
        tag = (f" update={u} mode={'batched' if batched else 'perchunk'}" if u else "")
        print(f"[{i+1}/{len(tests)}] grid={g} chunk={c} dens={d} scheme={s} seed={seed}{tag}",
              flush=True)
        if run_test(exe, out, g, c, d, s, seed, u, batched): ok += 1
        else: fail += 1
    print(f"Done {time.monotonic()-t0:.0f}s. OK={ok} FAIL={fail}")
    fails = sorted(out.glob("*.fail"))
    if fails:
        print(f"!! {len(fails)} config(s) FAILED — inspect these .fail files:")
        for fp in fails: print(f"   {fp.name}")
    aggregate(out)

if __name__ == "__main__": main()
