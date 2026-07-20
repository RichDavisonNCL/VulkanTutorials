"""Independent-process repeat validation: 5 fully independent process
launches (full re-init: WFC load, Vulkan device/pipeline creation, warmup)
per config, across 15 configs spanning 5 distinct performance regimes x
all 3 schemes. Answers a question the formal matrix's 1200-frame-per-run
statistics cannot: how much does the reported mean vary if the WHOLE
measurement process is redone, not just resampled within one run.

Regimes (see thesis discussion for why each was chosen):
  A: grid4096/chunk4/dens50   - CPU-bound extreme (large S1-vs-S3 gap)
  B: grid1024/chunk4/dens50   - S1/S2 indirect-draw reversal headline config
  C: grid16/chunk4/dens50     - tiny-scene regime (S1 briefly faster than S2)
  D: grid4096/chunk16/dens80/seed9999 - sphere-lock content-sensitivity outlier
  E: grid1024/chunk16/dens50  - steady mid-scale GPU-bound baseline

Each of the 5 regimes is run at all 3 schemes = 15 configs x 5 repeats = 75
independent process launches. Writes full per-frame CSVs (not just
aggregated stats) to results_repeatvalidation/, filename-suffixed with the
repeat index so nothing overwrites a prior repeat.
"""
import subprocess, sys
from pathlib import Path

EXE = Path("cmake-build-debug-visual-studio/GPUDrivenRendering/Release/GPUDrivenRendering.exe")
OUT = Path("results_repeatvalidation")
WARMUP, RECORD = 120, 1200
N_REPEATS = 5

REGIMES = [
    ("A", dict(grid=4096, chunk=4, density=50, seed=42)),
    ("B", dict(grid=1024, chunk=4, density=50, seed=42)),
    ("C", dict(grid=16,   chunk=4, density=50, seed=42)),
    ("D", dict(grid=4096, chunk=16, density=80, seed=9999)),
    ("E", dict(grid=1024, chunk=16, density=50, seed=42)),
]
SCHEMES = [1, 2, 3]

def est_timeout(grid, chunk):
    # Generous margin over the slowest observed single-run wall time
    # (regime D at ~13-14 min); floor covers process startup + small configs.
    chunks_sq = (grid // chunk) ** 2
    return int(min(120 + chunks_sq * 0.01 + grid * grid * 2e-4, 3600))

def run_one(regime_label, grid, chunk, density, seed, scheme, repeat):
    name = (f"regime{regime_label}_grid{grid}_chunk{chunk}_dens{density}"
             f"_scheme{scheme}_seed{seed}_run{repeat}")
    out_path = OUT / f"{name}.csv"
    if out_path.exists():
        return True
    cmd = [str(EXE.resolve()), "-Benchmark", "-GridSize", str(grid), "-ChunkSize", str(chunk),
           "-Density", str(density), "-Scheme", str(scheme), "-Seed", str(seed),
           "-WarmupFrames", str(WARMUP), "-RecordFrames", str(RECORD),
           "-Output", str(out_path)]
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=est_timeout(grid, chunk))
        if r.returncode != 0:
            print(f"  FAIL exit={r.returncode}\n{r.stderr[-2000:]}")
            if out_path.exists(): out_path.unlink()
            return False
        return True
    except subprocess.TimeoutExpired:
        print(f"  TIMEOUT on {name}")
        if out_path.exists(): out_path.unlink()
        return False

def main():
    OUT.mkdir(exist_ok=True)
    tests = []
    for label, base in REGIMES:
        for scheme in SCHEMES:
            for repeat in range(1, N_REPEATS + 1):
                tests.append((label, base, scheme, repeat))
    print(f"Repeat validation: {len(tests)} runs "
          f"({len(REGIMES)} regimes x {len(SCHEMES)} schemes x {N_REPEATS} repeats)")
    ok = fail = 0
    for i, (label, base, scheme, repeat) in enumerate(tests):
        print(f"[{i+1}/{len(tests)}] regime={label} grid={base['grid']} chunk={base['chunk']} "
              f"dens={base['density']} scheme={scheme} seed={base['seed']} run={repeat}", flush=True)
        if run_one(label, base['grid'], base['chunk'], base['density'], base['seed'], scheme, repeat):
            ok += 1
        else:
            fail += 1
    print(f"Done. OK={ok} FAIL={fail}")

if __name__ == "__main__":
    main()
