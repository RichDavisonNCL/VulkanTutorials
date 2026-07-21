"""Generate evidence-aligned dissertation figures from locked local artifacts.

``--check-only`` reads and validates every input but deliberately never creates
or changes figure files.  The generator only writes the four registered PNGs
in the sibling ``Figures`` directory; it never writes benchmark inputs or CSVs.
"""

from __future__ import annotations

import argparse
import csv
import json
import math
import struct
import sys
from array import array
from collections import Counter
from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.lines import Line2D
import numpy as np


ROOT = Path(__file__).resolve().parents[1]
RESULTS = ROOT / "results"
CACHE = ROOT / "cache"
LOCKED_CLAIMS = ROOT / "docs" / "dissertation" / "evidence" / "locked-claims.json"
FIGURE_REGISTER = ROOT / "docs" / "dissertation" / "figure-register.md"
FIGURES = ROOT.parent / "Figures"
AGGREGATE = RESULTS / "_aggregate.csv"
UPDATE_AGGREGATE = RESULTS / "_aggregate_update.csv"

OUTPUTS = {
    "C1": FIGURES / "fig5_1_cpu_record_bar.png",
    "C2": FIGURES / "fig5_2_content_sensitivity_bar.png",
    "C3": FIGURES / "fig5_3_triangle_count_scatter.png",
    "C4": FIGURES / "fig5_4_buffer_update_submission.png",
}

FORMAL_METADATA = {
    "commit": "407efde",
    "dirty": "true",
    "exe_hash": "e415726b9b02c3e5",
}
BANNED_LABELS = ("Density (%)", "GPU execution time", "CPU frame time", "Scheme")

# Okabe-Ito colors plus redundant marker, hatch, and line-style encodings.
BLUE, ORANGE, GREEN, VERMILION, PURPLE, BLACK = (
    "#0072B2",
    "#E69F00",
    "#009E73",
    "#D55E00",
    "#CC79A7",
    "#000000",
)
PATH_STYLE = {
    1: (BLUE, "o", "//"),
    2: (ORANGE, "s", "\\\\"),
    3: (GREEN, "^", "xx"),
}


def configure_style() -> None:
    plt.rcParams.update(
        {
            "font.family": "sans-serif",
            "font.sans-serif": ["Arial", "DejaVu Sans", "Liberation Sans"],
            "font.size": 9,
            "axes.labelsize": 10,
            "axes.titlesize": 11,
            "xtick.labelsize": 8,
            "ytick.labelsize": 8,
            "legend.fontsize": 8,
            "axes.linewidth": 0.8,
            "savefig.dpi": 400,
        }
    )


def read_csv(path: Path) -> list[dict[str, str]]:
    with path.open(newline="", encoding="utf-8") as handle:
        return list(csv.DictReader(handle))


def ints(row: dict[str, str], *fields: str) -> tuple[int, ...]:
    return tuple(int(row[field]) for field in fields)


def select(rows: list[dict[str, str]], **criteria: int | str) -> list[dict[str, str]]:
    return [
        row
        for row in rows
        if all(str(row[field]) == str(value) for field, value in criteria.items())
    ]


def close(a: float, b: float, label: str) -> None:
    if not math.isclose(a, b, rel_tol=1e-10, abs_tol=1e-9):
        raise ValueError(f"locked value mismatch for {label}: {a!r} != {b!r}")


def parse_metadata(path: Path) -> dict[str, str]:
    metadata: dict[str, str] = {}
    with path.open(encoding="utf-8") as handle:
        for line in handle:
            if not line.startswith("# "):
                break
            key, value = line[2:].strip().split("=", 1)
            metadata[key] = value
    return metadata


def validate_formal_raw_files(rows: list[dict[str, str]]) -> None:
    for row in rows:
        source = RESULTS / row["file"]
        if not source.is_file():
            raise FileNotFoundError(f"registered formal source is missing: {source}")
        metadata = parse_metadata(source)
        for key, expected in FORMAL_METADATA.items():
            actual = metadata.get(key)
            if actual != expected:
                raise ValueError(f"{source.name}: {key}={actual!r}; expected {expected!r}")


def cache_histogram(seed: int, tile_weight: int) -> Counter[int]:
    """Read the existing WFC cache format without producing source artifacts."""
    path = CACHE / f"wfc_4096_{seed}_{tile_weight}.bin"
    with path.open("rb") as handle:
        grid = struct.unpack("<I", handle.read(4))[0]
        if grid != 4096:
            raise ValueError(f"{path.name}: expected grid 4096, found {grid}")
        tiles = array("I")
        tiles.fromfile(handle, grid * grid)
    if sys.byteorder != "little":
        tiles.byteswap()
    if len(tiles) != grid * grid:
        raise ValueError(f"{path.name}: incomplete tile payload")
    return Counter(tiles)


def triangle_count(histogram: Counter[int]) -> int:
    cube_count = sum(histogram.get(tile, 0) for tile in (1, 2, 3))
    sphere_count = sum(histogram.get(tile, 0) for tile in (4, 5))
    return cube_count * 12 + sphere_count * 768


def c1_pairs(rows: list[dict[str, str]]) -> list[tuple[dict[str, str], dict[str, str]]]:
    grouped: dict[tuple[int, int, int, int], dict[int, dict[str, str]]] = {}
    for row in rows:
        grid, chunk, density, scheme, seed = ints(row, "grid", "chunk", "density", "scheme", "seed")
        if grid >= 256 and scheme in (1, 2):
            grouped.setdefault((grid, chunk, density, seed), {})[scheme] = row
    pairs = []
    for key, schemes in grouped.items():
        if set(schemes) != {1, 2}:
            raise ValueError(f"C1 lacks an S1/S2 match for {key}")
        pairs.append((schemes[1], schemes[2]))
    return pairs


def c2_rows(rows: list[dict[str, str]]) -> list[dict[str, str]]:
    selected = select(rows, grid=4096, chunk=4, density=50, seed=42)
    selected.sort(key=lambda row: int(row["scheme"]))
    if [int(row["scheme"]) for row in selected] != [1, 2, 3]:
        raise ValueError("C2 requires exactly S1, S2, and S3")
    return selected


def c3_rows(rows: list[dict[str, str]]) -> list[dict[str, str]]:
    selected = select(rows, grid=4096, chunk=16, density=80, scheme=3)
    selected.sort(key=lambda row: int(row["seed"]))
    if [int(row["seed"]) for row in selected] != [42, 1337, 9999]:
        raise ValueError("C3 requires exactly seeds 42, 1337, and 9999")
    return selected


def c4_rows(rows: list[dict[str, str]]) -> list[dict[str, str]]:
    expected_sizes = {1, 2, 4, 8, 16, 32}
    if len(rows) != 108:
        raise ValueError(f"C4 expected 108 aggregate rows, found {len(rows)}")
    for update_size in expected_sizes:
        for mode in ("batched", "perchunk"):
            count = len(select(rows, update_size=update_size, mode=mode))
            if count != 9:
                raise ValueError(f"C4 expected nine {mode} rows at update size {update_size}, found {count}")
    return rows


def validate_register() -> None:
    if not FIGURE_REGISTER.is_file():
        raise FileNotFoundError(f"figure register is missing: {FIGURE_REGISTER}")
    text = FIGURE_REGISTER.read_text(encoding="utf-8")
    for output in OUTPUTS.values():
        if output.name not in text:
            raise ValueError(f"figure register does not name {output.name}")
    for source in ("results/_aggregate.csv", "results/_aggregate_update.csv", "cache/wfc_4096_"):
        if source not in text:
            raise ValueError(f"figure register does not register source {source}")


def validate(aggregate: list[dict[str, str]], updates: list[dict[str, str]], claims: dict) -> dict[str, object]:
    for source in (AGGREGATE, UPDATE_AGGREGATE, LOCKED_CLAIMS):
        if not source.is_file():
            raise FileNotFoundError(f"locked source is missing: {source}")
    validate_register()

    pairs = c1_pairs(aggregate)
    if len(pairs) != claims["C1"]["pair_count"]:
        raise ValueError(f"C1 pair count mismatch: {len(pairs)}")
    ratios = [float(s1["cpu_record_avg"]) / float(s2["cpu_record_avg"]) for s1, s2 in pairs]
    close(min(ratios), claims["C1"]["ratio_min"], "C1 ratio_min")
    close(max(ratios), claims["C1"]["ratio_max"], "C1 ratio_max")
    validate_formal_raw_files([row for pair in pairs for row in pair])

    stress = c2_rows(aggregate)
    for row in stress:
        scheme = int(row["scheme"])
        close(float(row["cpu_record_avg"]), claims["C2"][f"s{scheme}_cpu_us"], f"C2 S{scheme} CPU")
        close(float(row["gpu_exec_avg"]), claims["C2"][f"s{scheme}_gpu_us"], f"C2 S{scheme} GPU")
    validate_formal_raw_files(stress)

    scene = c3_rows(aggregate)
    for row in scene:
        seed = int(row["seed"])
        close(float(row["gpu_exec_avg"]), claims["C3"][f"seed{seed}_gpu_us"], f"C3 seed {seed} GPU")
    scene_by_seed = {int(row["seed"]): row for row in scene}
    close(
        float(scene_by_seed[9999]["gpu_exec_avg"]) / float(scene_by_seed[42]["gpu_exec_avg"]),
        claims["C3"]["ratio_vs_seed42"],
        "C3 ratio vs seed42",
    )
    close(
        float(scene_by_seed[9999]["gpu_exec_avg"]) / float(scene_by_seed[1337]["gpu_exec_avg"]),
        claims["C3"]["ratio_vs_seed1337"],
        "C3 ratio vs seed1337",
    )
    validate_formal_raw_files(scene)
    histograms = {seed: cache_histogram(seed, 80) for seed in (42, 1337, 9999)}
    triangles = {seed: triangle_count(histogram) for seed, histogram in histograms.items()}
    if any(value <= 0 for value in triangles.values()):
        raise ValueError("C3 cache histograms produced no scene triangles")

    update_rows = c4_rows(updates)
    at_32_batched = [float(row["update_cost_avg"]) for row in select(update_rows, update_size=32, mode="batched")]
    at_32_perchunk = [float(row["update_cost_avg"]) for row in select(update_rows, update_size=32, mode="perchunk")]
    batched_mean = float(np.mean(at_32_batched))
    perchunk_mean = float(np.mean(at_32_perchunk))
    close(batched_mean, claims["C4"]["batched_mean_us"], "C4 batched mean")
    close(perchunk_mean, claims["C4"]["perchunk_mean_us"], "C4 per-chunk mean")
    close(perchunk_mean / batched_mean, claims["C4"]["ratio"], "C4 ratio")
    validate_formal_raw_files(update_rows)

    return {
        "ratios": ratios,
        "stress": stress,
        "scene": scene,
        "triangles": triangles,
        "updates": update_rows,
    }


def finish_axis(axis: plt.Axes) -> None:
    axis.spines["top"].set_visible(False)
    axis.spines["right"].set_visible(False)
    axis.grid(axis="y", color="#D9D9D9", linewidth=0.6, zorder=0)


def fig_c1(ratios: list[float]) -> plt.Figure:
    fig, axis = plt.subplots(figsize=(7.1, 4.2))
    bins = np.linspace(1.5, 10.25, 18)
    axis.hist(ratios, bins=bins, color=BLUE, edgecolor=BLACK, linewidth=0.6, hatch="//", zorder=2)
    axis.axvline(min(ratios), color=VERMILION, linestyle="--", linewidth=1.2, label=f"Range: {min(ratios):.2f}–{max(ratios):.2f}×")
    axis.axvline(max(ratios), color=VERMILION, linestyle="--", linewidth=1.2)
    axis.set_xlabel("S1/S2 CPU preparation-and-command-recording time ratio")
    axis.set_ylabel("Matched configuration pairs (count)")
    axis.set_title("Distribution of matched S1/S2 CPU preparation-and-command-recording time ratios\nGrid size ≥256; n=135")
    axis.set_xlim(1.5, 10.25)
    axis.set_ylim(bottom=0)
    axis.legend(frameon=False, loc="upper right")
    finish_axis(axis)
    fig.tight_layout()
    return fig


def fig_c2(rows: list[dict[str, str]]) -> plt.Figure:
    fig, axes = plt.subplots(1, 2, figsize=(9.0, 4.2), sharex=True)
    labels = [f"S{row['scheme']}" for row in rows]
    x = np.arange(len(rows))
    cpu_values = [float(row["cpu_record_avg"]) for row in rows]
    gpu_values = [float(row["gpu_exec_avg"]) for row in rows]
    for axis, values, ylabel, panel in (
        (axes[0], cpu_values, "CPU preparation-and-command-recording time (µs)", "A"),
        (axes[1], gpu_values, "GPU elapsed time from timestamp queries (µs)", "B"),
    ):
        for index, (row, value) in enumerate(zip(rows, values)):
            color, marker, hatch = PATH_STYLE[int(row["scheme"])]
            axis.bar(index, value, color=color, edgecolor=BLACK, linewidth=0.7, hatch=hatch, zorder=2)
            axis.plot(index, value, marker=marker, color=BLACK, markerfacecolor="white", markersize=5, zorder=3)
            label = f"{value:,.3f}" if value < 1000 else f"{value:,.0f}"
            axis.annotate(label, (index, value), xytext=(0, 5), textcoords="offset points", ha="center", va="bottom", fontsize=8)
        axis.set_yscale("log")
        axis.set_xticks(x, labels)
        axis.set_xlabel("Rendering path")
        axis.set_ylabel(ylabel)
        axis.set_title(panel)
        finish_axis(axis)
    legend = [
        Line2D([0], [0], color=color, marker=marker, linestyle="", markerfacecolor="white", markeredgecolor=BLACK, label=f"S{scheme}")
        for scheme, (color, marker, _) in PATH_STYLE.items()
    ]
    axes[1].legend(handles=legend, title="Rendering path", frameon=False, loc="upper left")
    fig.suptitle("Fine-granularity stress-test configuration: grid 4096, chunk size 4, tile-weight preset 50, seed 42", y=1.02, fontsize=11)
    fig.tight_layout()
    return fig


def fig_c3(rows: list[dict[str, str]], triangles: dict[int, int]) -> plt.Figure:
    fig, axis = plt.subplots(figsize=(7.4, 4.8))
    for row in rows:
        seed = int(row["seed"])
        x = triangles[seed] / 1_000_000
        y = float(row["gpu_exec_avg"]) / 1000
        # Orange open triangles encode the supplementary cache-histogram x value;
        # blue circles encode the formal timing y value.  No fitted statistic pools tiers.
        axis.scatter(x, y, s=280, marker="^", facecolors="none", edgecolors=ORANGE, linewidths=1.7, zorder=2)
        axis.scatter(x, y, s=75, marker="o", color=BLUE, edgecolors=BLACK, linewidths=0.7, zorder=3)
        if seed == 9999:
            axis.annotate("seed 9999", (x, y), xytext=(8, 8), textcoords="offset points", fontsize=8)
    axis.set_xlabel("Total triangle count from supplementary cache histogram (millions)")
    axis.set_ylabel("Formal GPU elapsed time from timestamp queries (ms)")
    axis.set_title("GPU elapsed time against scene mesh composition\nGrid 4096, chunk size 16, tile-weight preset 80, S3")
    axis.set_yscale("log")
    axis.legend(
        handles=[
            Line2D([0], [0], color=BLUE, marker="o", markeredgecolor=BLACK, linestyle="", label="Formal timing matrix"),
            Line2D([0], [0], color=ORANGE, marker="^", markerfacecolor="none", linestyle="", label="Supplementary cache histogram"),
        ],
        frameon=False,
        loc="upper left",
    )
    axis.text(0.99, 0.04, "Paired by seed; no pooled fit or cross-tier summary.", transform=axis.transAxes, ha="right", va="bottom", fontsize=8)
    finish_axis(axis)
    fig.tight_layout()
    return fig


def fig_c4(rows: list[dict[str, str]], claim: dict[str, float]) -> plt.Figure:
    fig, axis = plt.subplots(figsize=(8.0, 4.8))
    update_sizes = [1, 2, 4, 8, 16, 32]
    mode_style = {"batched": (BLUE, "o", "-"), "perchunk": (VERMILION, "s", "--")}
    offsets = {"batched": -0.14, "perchunk": 0.14}
    means: dict[str, list[float]] = {mode: [] for mode in mode_style}
    for mode, (color, marker, line_style) in mode_style.items():
        for position, update_size in enumerate(update_sizes):
            selected = select(rows, update_size=update_size, mode=mode)
            values = [float(row["update_cost_avg"]) for row in selected]
            jitter = np.linspace(-0.035, 0.035, len(values))
            axis.scatter(position + offsets[mode] + jitter, values, color=color, marker=marker, edgecolor=BLACK, linewidth=0.5, s=35, alpha=0.8, zorder=3)
            means[mode].append(float(np.mean(values)))
        axis.plot(np.arange(len(update_sizes)) + offsets[mode], means[mode], color=color, marker=marker, markeredgecolor=BLACK, linewidth=1.5, linestyle=line_style, label=f"{mode.replace('perchunk', 'per-chunk')} mean (n=9)", zorder=4)
    axis.set_yscale("log")
    axis.set_xticks(np.arange(len(update_sizes)), [str(size) for size in update_sizes])
    axis.set_xlabel("Update size (chunks)")
    axis.set_ylabel("Standalone buffer-update time (µs)")
    axis.set_title("Standalone buffer-update microbenchmark by update size\nIndividual configuration aggregates and mean paths")
    axis.annotate(
        f"32 chunks: {claim['ratio']:.2f}×\n{claim['batched_mean_us']:.2f} vs {claim['perchunk_mean_us']:,.2f} µs",
        xy=(5 + offsets["perchunk"], means["perchunk"][-1]),
        xytext=(3.1, 1500),
        arrowprops={"arrowstyle": "->", "color": BLACK, "linewidth": 0.8},
        fontsize=8,
        ha="left",
    )
    axis.legend(frameon=False, loc="upper left")
    finish_axis(axis)
    fig.tight_layout()
    return fig


def figure_texts(figures: list[plt.Figure]) -> list[str]:
    texts: list[str] = []
    for figure in figures:
        for axis in figure.axes:
            texts.extend((axis.get_title(), axis.get_xlabel(), axis.get_ylabel()))
            legend = axis.get_legend()
            if legend:
                texts.extend(item.get_text() for item in legend.get_texts())
    return texts


def banned_label_scan(figures: list[plt.Figure]) -> None:
    labels = figure_texts(figures)
    hits = [banned for banned in BANNED_LABELS if any(banned in label for label in labels)]
    if hits:
        raise ValueError(f"banned figure labels found: {', '.join(hits)}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check-only", action="store_true", help="validate inputs without creating or changing figures")
    args = parser.parse_args()

    configure_style()
    aggregate = read_csv(AGGREGATE)
    updates = read_csv(UPDATE_AGGREGATE)
    claims = json.loads(LOCKED_CLAIMS.read_text(encoding="utf-8"))["claims"]
    data = validate(aggregate, updates, claims)
    figures = [
        fig_c1(data["ratios"]),
        fig_c2(data["stress"]),
        fig_c3(data["scene"], data["triangles"]),
        fig_c4(data["updates"], claims["C4"]),
    ]
    banned_label_scan(figures)
    if args.check_only:
        for figure in figures:
            plt.close(figure)
        print("check-only: validated sources, filters, row counts, locked values, provenance, figure-register references, and figure labels")
        return 0
    for figure, output in zip(figures, OUTPUTS.values()):
        figure.savefig(output, dpi=400, bbox_inches="tight", facecolor="white")
        plt.close(figure)
        print(f"wrote {output}")
    print("banned-label scan: passed")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print(f"error: {error}", file=sys.stderr)
        raise SystemExit(1)
