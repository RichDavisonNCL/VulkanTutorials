"""Supplementary experiment: sweep cube:sphere weight ratio at a fixed,
non-locking density (emptyWeight=5, matching the formal matrix's density=50
tier) to get a continuous triangle-count scan for gpu_exec, rather than the
formal matrix's 3 discrete (mostly coincident) density/seed points.
Not part of the 810-config formal matrix — writes to results_weightsweep/.
"""
import subprocess, sys
from pathlib import Path

EXE = Path("cmake-build-debug-visual-studio/GPUDrivenRendering/Release/GPUDrivenRendering.exe")
OUT = Path("results_weightsweep")
GRID, CHUNK, SEEDS = 1024, 16, [42, 1337, 9999]
# 11 points, cube:sphere from all-cube to all-sphere, total weight fixed at 10
# to match the formal matrix's density=50 otherWeight=5+5 magnitude.
WEIGHT_POINTS = [(10 - i, i) for i in range(0, 11)]  # (cubeWeight, sphereWeight)
WARMUP, RECORD = 120, 1200

def run_one(cube_w, sphere_w, seed):
    name = f"grid{GRID}_chunk{CHUNK}_cube{cube_w}_sphere{sphere_w}_seed{seed}"
    out_path = OUT / f"{name}.csv"
    if out_path.exists():
        return True
    cmd = [str(EXE.resolve()), "-Benchmark", "-GridSize", str(GRID), "-ChunkSize", str(CHUNK),
           "-Density", "50", "-Scheme", "1", "-Seed", str(seed),
           "-CubeWeight", str(cube_w), "-SphereWeight", str(sphere_w),
           "-WarmupFrames", str(WARMUP), "-RecordFrames", str(RECORD),
           "-Output", str(out_path)]
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
        if r.returncode != 0:
            print(f"  FAIL exit={r.returncode}\n{r.stderr[-2000:]}")
            return False
        return True
    except subprocess.TimeoutExpired:
        print(f"  TIMEOUT on {name}")
        return False

def main():
    OUT.mkdir(exist_ok=True)
    tests = [(cw, sw, seed) for cw, sw in WEIGHT_POINTS for seed in SEEDS]
    print(f"Weight sweep: {len(tests)} configs (grid={GRID}, chunk={CHUNK}, "
          f"{len(WEIGHT_POINTS)} weight points x {len(SEEDS)} seeds)")
    ok = fail = 0
    for i, (cw, sw, seed) in enumerate(tests):
        print(f"[{i+1}/{len(tests)}] cube={cw} sphere={sw} seed={seed}", flush=True)
        if run_one(cw, sw, seed):
            ok += 1
        else:
            fail += 1
    print(f"Done. OK={ok} FAIL={fail}")

if __name__ == "__main__":
    main()
