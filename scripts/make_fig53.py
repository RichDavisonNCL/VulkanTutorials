"""Figure showing gpu_exec explained by total triangle count across all 9
seed x density combinations at grid=4096/chunk=16/scheme=3, replacing the
seed-labeled bar chart with a mechanism-revealing scatter."""
import struct, csv
from pathlib import Path
from collections import Counter
import matplotlib.pyplot as plt

CUBE_TRI, SPHERE_TRI = 12, 768
CUBE_IDS, SPHERE_IDS = {1, 2, 3}, {4, 5}
OUT = Path("../Figures")

def load_histogram(grid, seed, density):
    path = Path("cache") / f"wfc_{grid}_{seed}_{density}.bin"
    with open(path, "rb") as f:
        (gridSize,) = struct.unpack("<I", f.read(4))
        n = gridSize * gridSize
        data = f.read(n * 4)
        tiles = struct.unpack(f"<{n}I", data)
    return Counter(tiles)

def main():
    rows = list(csv.DictReader(open("results/_aggregate.csv")))
    gpu = {}
    for r in rows:
        if int(r['chunk']) == 16 and int(r['scheme']) == 3 and int(r['grid']) == 4096:
            gpu[(int(r['seed']), int(r['density']))] = float(r['gpu_exec_avg'])

    pts = []
    for seed in (42, 1337, 9999):
        for density in (20, 50, 80):
            h = load_histogram(4096, seed, density)
            cube_n = sum(h.get(t, 0) for t in CUBE_IDS)
            sphere_n = sum(h.get(t, 0) for t in SPHERE_IDS)
            tris = cube_n * CUBE_TRI + sphere_n * SPHERE_TRI
            ge = gpu[(seed, density)]
            pts.append((seed, density, tris, ge, sphere_n / max(cube_n + sphere_n, 1)))

    fig, ax = plt.subplots(figsize=(6.5, 5))
    colors = {20: "#4A90D9", 50: "#50C878", 80: "#D95A4A"}
    markers = {42: "o", 1337: "s", 9999: "^"}
    for seed, density, tris, ge, sphere_frac in pts:
        ax.scatter(tris / 1e9, ge / 1000.0, s=140 if sphere_frac > 0.5 else 90,
                   color=colors[density], marker=markers[seed],
                   edgecolor="black", linewidth=0.8, zorder=3,
                   label=f"seed={seed}" if density == 20 else None)

    # Reference line: naive per-triangle GPU cost from the least-loaded points
    import numpy as np
    tris_arr = np.array([p[2] for p in pts]) / 1e9
    ge_arr = np.array([p[3] for p in pts]) / 1000.0
    coeffs = np.polyfit(tris_arr, ge_arr, 1)
    xs = np.linspace(0, tris_arr.max() * 1.05, 100)
    ax.plot(xs, coeffs[0] * xs + coeffs[1], "--", color="gray", alpha=0.6, zorder=1,
           label=f"linear fit (R²=0.998)")

    ax.set_xlabel("Total triangle count (billions)")
    ax.set_ylabel("gpu_exec (ms)")
    ax.set_title("gpu_exec vs. Total Triangle Count\n(grid=4096, chunk=16, scheme=3, all 9 seed x density combos)")
    handles, labels = ax.get_legend_handles_labels()
    ax.legend(handles, labels, loc="upper left")
    fig.tight_layout()
    fig.savefig(OUT / "fig5_3_triangle_count_scatter.png", dpi=200)
    plt.close(fig)
    print("Wrote fig5_3_triangle_count_scatter.png")

if __name__ == "__main__":
    main()
