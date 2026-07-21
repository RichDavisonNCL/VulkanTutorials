"""Extract the four locked dissertation observations from aggregate CSV files."""

import argparse
import csv
import json
import math
from datetime import datetime, timezone
from pathlib import Path
from statistics import fmean


INTEGER_COLUMNS = {"grid", "chunk", "density", "scheme", "seed", "update", "update_size"}
METRIC_COLUMNS = {
    "cpu_record_avg",
    "cpu_record_p99",
    "gpu_exec_avg",
    "gpu_exec_p99",
    "frame_wall_avg",
    "frame_wall_p99",
    "cpu_wait_avg",
    "update_cost_avg",
    "update_cost_p99",
    "update_cost_stddev",
}
SOURCE_FILES = ["results/_aggregate.csv", "results/_aggregate_update.csv"]


def load_csv(path):
    """Read an aggregate CSV and convert identifier and metric fields to numbers."""
    with Path(path).open(newline="", encoding="utf-8") as handle:
        rows = list(csv.DictReader(handle))

    for row in rows:
        for column in INTEGER_COLUMNS.intersection(row):
            row[column] = int(row[column])
        for column in METRIC_COLUMNS.intersection(row):
            row[column] = float(row[column])
    return rows


def _index_unique(rows, key_fields, label):
    index = {}
    for row in rows:
        key = tuple(row[field] for field in key_fields)
        if key in index:
            raise ValueError(f"duplicate {label} key: {key}")
        index[key] = row
    return index


def _one_row(rows, predicate, label):
    matches = [row for row in rows if predicate(row)]
    if len(matches) != 1:
        raise ValueError(f"expected one {label} row, found {len(matches)}")
    return matches[0]


def _require_positive_finite_denominator(value, claim, label):
    if not math.isfinite(value) or value <= 0:
        raise ValueError(
            f"{claim} denominator must be finite and positive for {label}: {value!r}"
        )


def compute_locked_claims(render_rows, update_rows):
    """Compute the C1--C4 observations with strict duplicate and row checks."""
    render_key = ("grid", "chunk", "density", "scheme", "seed")
    update_key = ("grid", "chunk", "density", "scheme", "seed", "update_size", "mode")
    _index_unique(render_rows, render_key, "render")
    _index_unique(update_rows, update_key, "update")

    s2_index = _index_unique(
        (row for row in render_rows if row["scheme"] == 2),
        ("grid", "chunk", "density", "seed"),
        "S2 rendering-path",
    )
    s1_rows = [
        row
        for row in render_rows
        if row["scheme"] == 1 and row["grid"] >= 256
    ]
    ratios = []
    for row in s1_rows:
        key = (row["grid"], row["chunk"], row["density"], row["seed"])
        if key not in s2_index:
            raise ValueError(f"missing S2 rendering-path row for C1 key: {key}")
        denominator = s2_index[key]["cpu_record_avg"]
        _require_positive_finite_denominator(denominator, "C1", f"S2 key {key}")
        ratios.append(row["cpu_record_avg"] / denominator)
    if not ratios:
        raise ValueError("missing matched S1/S2 rows for C1")

    c2_rows = {}
    for scheme in (1, 2, 3):
        c2_rows[scheme] = _one_row(
            render_rows,
            lambda row, scheme=scheme: (
                row["scheme"] == scheme
                and row["grid"] == 4096
                and row["chunk"] == 4
                and row["density"] == 50
                and row["seed"] == 42
            ),
            f"C2 S{scheme} rendering-path",
        )

    c3_rows = {}
    for seed in (42, 1337, 9999):
        c3_rows[seed] = _one_row(
            render_rows,
            lambda row, seed=seed: (
                row["scheme"] == 3
                and row["grid"] == 4096
                and row["chunk"] == 16
                and row["density"] == 80
                and row["seed"] == seed
            ),
            f"C3 S3 rendering-path seed {seed}",
        )

    c4_rows = [row for row in update_rows if row["update_size"] == 32]
    modes = {}
    for mode in ("batched", "perchunk"):
        values = [row["update_cost_avg"] for row in c4_rows if row["mode"] == mode]
        if not values:
            raise ValueError(f"missing C4 {mode} update rows")
        modes[mode] = fmean(values)

    c3_seed42 = c3_rows[42]["gpu_exec_avg"]
    c3_seed1337 = c3_rows[1337]["gpu_exec_avg"]
    _require_positive_finite_denominator(c3_seed42, "C3", "seed42 GPU elapsed time")
    _require_positive_finite_denominator(c3_seed1337, "C3", "seed1337 GPU elapsed time")
    _require_positive_finite_denominator(modes["batched"], "C4", "batched mean update cost")

    return {
        "C1": {
            "pair_count": len(ratios),
            "ratio_min": min(ratios),
            "ratio_max": max(ratios),
        },
        "C2": {
            "s1_cpu_us": c2_rows[1]["cpu_record_avg"],
            "s1_gpu_us": c2_rows[1]["gpu_exec_avg"],
            "s2_cpu_us": c2_rows[2]["cpu_record_avg"],
            "s2_gpu_us": c2_rows[2]["gpu_exec_avg"],
            "s3_cpu_us": c2_rows[3]["cpu_record_avg"],
            "s3_gpu_us": c2_rows[3]["gpu_exec_avg"],
        },
        "C3": {
            "seed42_gpu_us": c3_rows[42]["gpu_exec_avg"],
            "seed1337_gpu_us": c3_rows[1337]["gpu_exec_avg"],
            "seed9999_gpu_us": c3_rows[9999]["gpu_exec_avg"],
            "ratio_vs_seed42": c3_rows[9999]["gpu_exec_avg"] / c3_seed42,
            "ratio_vs_seed1337": c3_rows[9999]["gpu_exec_avg"] / c3_seed1337,
        },
        "C4": {
            "batched_mean_us": modes["batched"],
            "perchunk_mean_us": modes["perchunk"],
            "ratio": modes["perchunk"] / modes["batched"],
        },
    }


def _markdown(claims, source_files, generated_at_utc):
    c1, c2, c3, c4 = (claims[name] for name in ("C1", "C2", "C3", "C4"))
    source_list = ", ".join(f"`{path}`" for path in source_files)
    return f"""# Locked dissertation evidence

Generated UTC: {generated_at_utc}

Source files: {source_list}

## C1 — S1/S2 CPU preparation-and-command-recording comparison

Raw extracted values: {c1['pair_count']} matched rendering-path pairs; ratio range {c1['ratio_min']!r} to {c1['ratio_max']!r}.

Rounded prose value: Across {c1['pair_count']} matched configurations at grid ≥256, S1/S2 `cpu_record_avg` ratios span {c1['ratio_min']:.4f}–{c1['ratio_max']:.4f}×. This is a whole-rendering-path comparison.

## C2 — Fine-granularity stress-test configuration

Raw extracted values: S1 CPU/GPU {c2['s1_cpu_us']!r}/{c2['s1_gpu_us']!r} µs; S2 CPU/GPU {c2['s2_cpu_us']!r}/{c2['s2_gpu_us']!r} µs; S3 CPU/GPU {c2['s3_cpu_us']!r}/{c2['s3_gpu_us']!r} µs.

Rounded prose value: At grid4096/chunk4/tile-weight preset50/seed42, S3 records {c2['s3_cpu_us']:.3f}µs CPU preparation-and-command-recording time and {c2['s3_gpu_us']:,.0f}µs GPU elapsed time measured with timestamp queries.

## C3 — Scene mesh composition and GPU workload

Raw extracted values: seed42 {c3['seed42_gpu_us']!r} µs; seed1337 {c3['seed1337_gpu_us']!r} µs; seed9999 {c3['seed9999_gpu_us']!r} µs; ratios {c3['ratio_vs_seed42']!r} and {c3['ratio_vs_seed1337']!r}.

Rounded prose value: At grid4096/chunk16/tile-weight preset80 on S3, seed9999 records {c3['seed9999_gpu_us']:,.0f}µs GPU elapsed time, {c3['ratio_vs_seed42']:.2f}× seed42 and {c3['ratio_vs_seed1337']:.2f}× seed1337. This is a configuration observation.

## C4 — Buffer-update submission granularity

Raw extracted values: batched mean {c4['batched_mean_us']!r} µs; per-chunk mean {c4['perchunk_mean_us']!r} µs; ratio {c4['ratio']!r}.

Rounded prose value: For the 32-chunk standalone buffer-update microbenchmark across all seeds and rendering paths, batched submit-and-wait records {c4['batched_mean_us']:.2f}µs and the per-chunk reference path records {c4['perchunk_mean_us']:,.2f}µs, a {c4['ratio']:.2f}× ratio.
"""


def write_outputs(claims, out_dir, source_files=None):
    """Write the JSON and Markdown evidence artifacts, and only those artifacts."""
    output = Path(out_dir)
    output.mkdir(parents=True, exist_ok=True)
    source_files = list(SOURCE_FILES if source_files is None else map(str, source_files))
    generated_at_utc = datetime.now(timezone.utc).isoformat().replace("+00:00", "Z")
    document = {
        "source_files": source_files,
        "generated_at_utc": generated_at_utc,
        "claims": claims,
        "rounding_policy": {
            "raw_values": "Python float values extracted from the aggregate CSV files",
            "prose": "C1 uses 4 decimals; C2 CPU uses 3 decimals and GPU uses whole microseconds; C3 ratios use 2 decimals; C4 means and ratio use 2 decimals.",
        },
    }
    (output / "locked-claims.json").write_text(
        json.dumps(document, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    (output / "locked-claims.md").write_text(
        _markdown(claims, source_files, generated_at_utc), encoding="utf-8"
    )


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--render", required=True, type=Path)
    parser.add_argument("--update", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    args = parser.parse_args()

    claims = compute_locked_claims(load_csv(args.render), load_csv(args.update))
    write_outputs(claims, args.out, source_files=[args.render, args.update])
    print(f"C1 pair_count={claims['C1']['pair_count']}")
    print(f"C2 s3_gpu_us={claims['C2']['s3_gpu_us']:.0f}")
    print(f"C3 seed9999_gpu_us={claims['C3']['seed9999_gpu_us']:.0f}")
    print(f"C4 ratio={claims['C4']['ratio']:.2f}")


if __name__ == "__main__":
    main()
