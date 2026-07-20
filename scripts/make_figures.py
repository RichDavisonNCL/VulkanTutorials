"""Generate Figures 5.1 and 5.2 for Ch5 from results/_aggregate.csv."""
import csv
from pathlib import Path
import matplotlib.pyplot as plt

RESULTS = Path("results")
OUT = Path("../Figures")

def load():
    return list(csv.DictReader(open(RESULTS / "_aggregate.csv")))

def sel(rows, **kw):
    for r in rows:
        if all(int(r[k]) == v for k, v in kw.items()):
            return r

def fig_5_1(rows):
    vals = {}
    for s in (1, 2, 3):
        r = sel(rows, grid=1024, chunk=4, density=50, scheme=s, seed=42)
        vals[s] = float(r["cpu_record_avg"])
    labels = ["S1\n(CPU Instanced)", "S2\n(CPU Cull+Indirect)", "S3\n(GPU Cull+Indirect)"]
    values = [vals[1], vals[2], vals[3]]
    colors = ["#4A90D9", "#F5A623", "#50C878"]
    fig, ax = plt.subplots(figsize=(6, 4.5))
    bars = ax.bar(labels, values, color=colors, edgecolor="black", linewidth=0.6)
    for b, v in zip(bars, values):
        ax.text(b.get_x() + b.get_width() / 2, v, f"{v:.0f}us",
                ha="center", va="bottom", fontsize=10)
    ax.set_ylabel("cpu_record (us)")
    ax.set_title("cpu_record by Scheme\n(grid=1024, chunk=4, density=50%, seed=42)")
    fig.tight_layout()
    fig.savefig(OUT / "fig5_1_cpu_record_bar.png", dpi=200)
    plt.close(fig)

def fig_5_2(rows):
    densities = [20, 50, 80]
    seeds = [42, 1337, 9999]
    colors = {42: "#4A90D9", 1337: "#50C878", 9999: "#D95A4A"}
    fig, ax = plt.subplots(figsize=(7, 4.5))
    width = 0.25
    x = range(len(densities))
    for i, seed in enumerate(seeds):
        vals = []
        for d in densities:
            r = sel(rows, grid=4096, chunk=16, density=d, scheme=3, seed=seed)
            vals.append(float(r["gpu_exec_avg"]) / 1000.0)
        offset = (i - 1) * width
        bars = ax.bar([xi + offset for xi in x], vals, width,
                       label=f"seed={seed}", color=colors[seed], edgecolor="black", linewidth=0.5)
        for b, v in zip(bars, vals):
            if v > 100:
                ax.text(b.get_x() + b.get_width() / 2, v, f"{v:.0f}",
                        ha="center", va="bottom", fontsize=8)
    ax.set_xticks(list(x))
    ax.set_xticklabels([f"{d}%" for d in densities])
    ax.set_xlabel("Density")
    ax.set_ylabel("gpu_exec (ms)")
    ax.set_title("gpu_exec by Seed x Density\n(grid=4096, chunk=16, scheme=3)")
    ax.legend()
    fig.tight_layout()
    fig.savefig(OUT / "fig5_2_content_sensitivity_bar.png", dpi=200)
    plt.close(fig)

if __name__ == "__main__":
    rows = load()
    OUT.mkdir(exist_ok=True)
    fig_5_1(rows)
    fig_5_2(rows)
    print("Wrote fig5_1_cpu_record_bar.png and fig5_2_content_sensitivity_bar.png to", OUT.resolve())
