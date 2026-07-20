"""Back-to-back runner: (1) formal 810-config matrix, (2) cube:sphere
weight-ratio sweep, (3) independent-process repeat validation. Each stage
writes to its own directory; each stage starts automatically once the
previous one finishes (no separate launch needed). All three stages are
independently resumable (each underlying script skips configs/repeats
whose output CSV already exists), so re-running this after a
partial/interrupted run only fills in what's missing.
"""
import subprocess, sys, time

STAGES = [
    ("Formal 810-config matrix -> results/", ["python", "scripts/run_benchmarks.py"]),
    ("Weight-ratio sweep (63 configs) -> results_weightsweep/", ["python", "scripts/run_weight_sweep.py"]),
    ("Independent-process repeat validation (75 runs) -> results_repeatvalidation/",
     ["python", "scripts/run_repeat_validation.py"]),
]

def main():
    t0 = time.monotonic()
    for label, cmd in STAGES:
        print(f"\n{'='*72}\n{label}\n{'='*72}", flush=True)
        r = subprocess.run(cmd)
        if r.returncode != 0:
            print(f"!! Stage failed (exit={r.returncode}): {label}")
            print("Stopping — fix the issue and re-run scripts/run_all.py; "
                  "already-completed configs in either stage will be skipped.")
            sys.exit(r.returncode)
    print(f"\nAll stages complete. Total wall time: {time.monotonic()-t0:.0f}s")

if __name__ == "__main__":
    main()
