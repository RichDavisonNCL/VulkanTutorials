"""
analyze.py -- Full data analysis for "A Vulkan-based Evaluation of
GPU-Driven Scene Management for PCG Modular Scenes"
Outputs all numbers needed for Chapter 5 (Results & Analysis), organized
by section. Also generates LaTeX-ready tables.

Usage: python scripts/analyze.py             (print all results)
       python scripts/analyze.py --latex     (generate LaTeX tables)
       python scripts/analyze.py --section 5.1  (only that section)
"""
import csv, os, io, statistics as st, argparse, sys
from collections import defaultdict
from pathlib import Path

RESULT_DIR = Path("results")

# -- Helpers ----------------------------------------------------------------

def first_frame(path):
    """Read draw_calls / visible_chunks from first recorded frame row."""
    with open(path, encoding='utf-8') as fh:
        lines = fh.readlines()
    data_lines = [l for l in lines if not l.startswith('#')]
    rd = csv.reader(data_lines[1:])
    row = next(rd)
    return int(row[5]), int(row[6])

def load_render_agg():
    rows = list(csv.DictReader(open(RESULT_DIR / "_aggregate.csv")))
    for r in rows:
        try:
            dc, vc = first_frame(RESULT_DIR / r['file'])
            r['draw_calls'] = dc
            r['visible_chunks'] = vc
        except Exception:
            r['draw_calls'] = None
            r['visible_chunks'] = None
    return rows

def load_update_agg():
    return list(csv.DictReader(open(RESULT_DIR / "_aggregate_update.csv")))

def f(r, k): return float(r[k])
def i(r, k): return int(r[k])

def sel(rows, **kw):
    out = []
    for r in rows:
        ok = True
        for k, v in kw.items():
            try:
                if isinstance(v, (list, tuple)):
                    if int(r[k]) not in v: ok = False; break
                else:
                    if int(r[k]) != v: ok = False; break
            except Exception: ok = False; break
        if ok: out.append(r)
    return out

def avg(rr, k):
    vals = [f(r,k) for r in rr if r.get(k) not in (None,'')]
    return st.mean(vals) if vals else float('nan')
def avg_int(rr, k):
    vals = [r[k] for r in rr if r[k] is not None]
    return st.mean(vals) if vals else float('nan')

# -- Main Analysis ----------------------------------------------------------

def section_5_1(rows):
    """5.1 Steady-State Rendering Cost by Scheme"""
    print("=" * 72)
    print("5.1  STEADY-STATE RENDERING COST BY SCHEME")
    print("=" * 72)

    # 5.1.1 GPU Execution Time
    print("\n-- 5.1.1 GPU Execution Time vs Grid --")
    print(f"{'grid':>6} {'s1_gpu':>10} {'s2_gpu':>10} {'s3_gpu':>10} {'s3/s1%':>8}")
    for g in (16, 64, 256, 512, 1024, 2048, 4096):
        vals = {}
        for s in (1, 2, 3):
            sub = sel(rows, scheme=s, grid=g, chunk=16, density=50, seed=42)
            if sub: vals[s] = f(sub[0], 'gpu_exec_avg')
        if len(vals) == 3:
            pct = (vals[3]-vals[1])/vals[1]*100 if vals[1] else 0
            print(f"{g:>6} {vals[1]:>10.1f} {vals[2]:>10.1f} {vals[3]:>10.1f} {pct:>+7.1f}%")

    # 5.1.2 CPU Recording Cost
    print("\n-- 5.1.2 CPU Recording Cost vs Chunk^2 (grid=1024, dens50, seed42) --")
    print(f"{'chunk':>6} {'chunks_sq':>10} {'s1_cpu':>10} {'s2_cpu':>10} {'s3_cpu':>10} {'s1/s3':>8}")
    for c in (4, 8, 16):
        chunks_sq = (1024 // c) ** 2
        cr = {}
        for s in (1, 2, 3):
            sub = sel(rows, scheme=s, grid=1024, chunk=c, density=50, seed=42)
            if sub: cr[s] = f(sub[0], 'cpu_record_avg')
        if len(cr) == 3:
            print(f"{c:>6} {chunks_sq:>10} {cr[1]:>10.1f} {cr[2]:>10.1f} {cr[3]:>10.1f} {cr[1]/cr[3]:>8.0f}x")

    # 5.1.3 Counterintuitive (S2 slower than S1 at high visibility)
    print("\n-- 5.1.3 CPU Culling Degradation (grid1024/ch4/dens50) --")
    for s in (1, 2, 3):
        sub = sel(rows, scheme=s, grid=1024, chunk=4, density=50, seed=42)
        if sub:
            cr = f(sub[0], 'cpu_record_avg')
            dc = sub[0].get('draw_calls', '?')
            print(f"  Scheme {s}: cpu_record={cr:.0f}us  draw_calls={dc}")

    # Per-chunk CPU cost
    print("\n-- Per-chunk CPU cost @ grid4096 (dens50, seed42) --")
    for c in (4, 16):
        for s in (1, 3):
            sub = sel(rows, scheme=s, grid=4096, chunk=c, density=50, seed=42)
            if sub:
                nchunks = (4096 // c) ** 2
                pc = f(sub[0], 'cpu_record_avg') / nchunks
                print(f"  chunk{c} scheme{s}: {pc:.2f}us/chunk  ({nchunks} chunks, {f(sub[0],'cpu_record_avg'):.0f}us total)")


def section_5_2(rows):
    """5.2 CPU/GPU Bottleneck Crossover"""
    print("\n" + "=" * 72)
    print("5.2  CPU/GPU BOTTLENECK CROSSOVER")
    print("=" * 72)

    print("\n-- Bottleneck at ch16/dens50/seed42 --")
    print(f"{'grid':>6} {'s1_cpu':>10} {'s1_gpu':>10} {'s1_limit':>8} | {'s3_cpu':>10} {'s3_gpu':>10} {'s3_limit':>8}")
    for g in (64, 128, 256, 512, 1024, 2048, 4096):
        line = f"{g:>6}"
        for s in (1, 3):
            sub = sel(rows, scheme=s, grid=g, chunk=16, density=50, seed=42)
            if sub:
                cr = f(sub[0], 'cpu_record_avg')
                ge = f(sub[0], 'gpu_exec_avg')
                bot = "CPU" if cr > ge else "GPU"
                line += f" {cr:>10.0f} {ge:>10.0f} {bot:>8}"
            else:
                line += f" {'n/a':>10} {'n/a':>10} {'n/a':>8}"
        print(line)

    # chunk4 CPU-bound
    print("\n-- chunk4 CPU-bound check (grid4096/ch4/dens50) --")
    for s in (1, 2, 3):
        sub = sel(rows, scheme=s, grid=4096, chunk=4, density=50, seed=42)
        if sub:
            cr = f(sub[0], 'cpu_record_avg')
            ge = f(sub[0], 'gpu_exec_avg')
            bot = "CPU-BOUND" if cr > ge else "GPU-BOUND"
            print(f"  Scheme {s}: cpu={cr:.0f}  gpu={ge:.0f}  -> {bot}")


def section_5_3(rows):
    """5.3 Scene Content Sensitivity"""
    print("\n" + "=" * 72)
    print("5.3  SCENE CONTENT SENSITIVITY")
    print("=" * 72)

    # 5.3.1 seed9999 x dens80 outlier
    print("\n-- gpu_exec by seed x density (ch16, grid4096, scheme3) --")
    for d in (20, 50, 80):
        print(f"  density={d}:")
        for seed in (42, 1337, 9999):
            sub = sel(rows, scheme=3, grid=4096, chunk=16, density=d, seed=seed)
            if sub:
                ge = f(sub[0], 'gpu_exec_avg')
                dc = sub[0].get('draw_calls', '?')
                vc = sub[0].get('visible_chunks', '?')
                print(f"    seed={seed}: gpu_exec={ge:.0f}us  draw_calls={dc}  vis_chunks={vc}")

    # seed9999 x dens80 across grid sizes
    print("\n-- seed9999/dens80 ratio to other seeds (ch16, scheme3) --")
    print(f"{'grid':>6} {'seed42':>10} {'seed1337':>10} {'seed9999':>10} {'ratio':>8}")
    for g in (256, 512, 1024, 2048, 4096):
        ge = {}
        for seed in (42, 1337, 9999):
            sub = sel(rows, scheme=3, grid=g, chunk=16, density=80, seed=seed)
            if sub: ge[seed] = f(sub[0], 'gpu_exec_avg')
        if len(ge) == 3:
            base = (ge[42] + ge[1337]) / 2
            print(f"{g:>6} {ge[42]:>10.0f} {ge[1337]:>10.0f} {ge[9999]:>10.0f} {ge[9999]/base:>7.1f}x")

    # 5.3.2 per-chunk GPU cost
    print("\n-- per-chunk GPU cost (ch16/dens20/scheme1) --")
    for seed in (42, 1337, 9999):
        for g in (64, 256, 1024, 4096):
            sub = sel(rows, scheme=1, grid=g, chunk=16, density=20, seed=seed)
            if sub:
                ge = f(sub[0], 'gpu_exec_avg')
                nchunks = (g // 16) ** 2
                ns = ge * 1000.0 / nchunks
                print(f"  seed{seed} grid{g:>5}: {ns:.1f} ns/chunk")


def section_5_4(update_rows, render_rows):
    """5.4 Local Update Cost"""
    print("\n" + "=" * 72)
    print("5.4  LOCAL UPDATE COST")
    print("=" * 72)

    # 5.4.1 Batched vs Per-Chunk (mean across all 3 schemes, seed=42)
    print("\n-- Batched vs Per-chunk Update Cost (seed42, mean across schemes) --")
    print(f"{'size':>5} {'batched_avg':>12} {'perchunk_avg':>12} {'speedup':>8}")
    bd, pd = {}, {}
    for u in (1, 2, 4, 8, 16, 32):
        bvals = [f(r, 'update_cost_avg') for r in update_rows
                  if r['mode']=='batched' and int(r['seed'])==42 and int(r['update_size'])==u]
        pvals = [f(r, 'update_cost_avg') for r in update_rows
                  if r['mode']=='perchunk' and int(r['seed'])==42 and int(r['update_size'])==u]
        if bvals: bd[u] = st.mean(bvals)
        if pvals: pd[u] = st.mean(pvals)
    for u in sorted(bd.keys()):
        if u in pd:
            sp = pd[u] / bd[u] if bd[u] > 0 else 0
            print(f"{u:>5} {bd[u]:>12.1f} {pd[u]:>12.1f} {sp:>7.1f}x")

    # 5.4.2 Scheme-independence
    print("\n-- Scheme-independence (batched cost by scheme) --")
    for u in (1, 16, 32):
        by_scheme = defaultdict(list)
        for r in update_rows:
            if int(r['update_size']) == u and r['mode'] == 'batched':
                by_scheme[int(r['scheme'])].append(f(r, 'update_cost_avg'))
        if by_scheme:
            vals = ", ".join(f"s{s}={st.mean(v):.1f}" for s, v in sorted(by_scheme.items()))
            maxdiff = max(st.mean(v) for v in by_scheme.values()) - min(st.mean(v) for v in by_scheme.values())
            overall = st.mean([x for v in by_scheme.values() for x in v])
            pct = maxdiff / overall * 100 if overall > 0 else 0
            print(f"  size={u:>2}: {vals}  (max diff = {pct:.1f}%)")

    # 5.4.3 Frame budget impact
    print("\n-- Frame Budget Impact (128^2/ch8/dens50, batched 16-chunk) --")
    batched_16 = [r for r in update_rows if int(r['update_size'])==16 and r['mode']=='batched']
    if batched_16:
        avg_cost = st.mean(f(r, 'update_cost_avg') for r in batched_16)
        for s in (1, 3):
            sub = sel(render_rows, scheme=s, grid=128, chunk=8, density=50, seed=42)
            if sub:
                fw = f(sub[0], 'frame_wall_avg')
                print(f"  Scheme {s}: update_cost={avg_cost:.0f}us  frame_wall={fw:.1f}us  -> {avg_cost/fw:.1f} frames equivalent")


def generate_latex_tables(rows, update_rows):
    """Output LaTeX table source for key comparisons."""
    print("\n" + "%" * 60)
    print("% LATEX TABLES")
    print("%" * 60)

    # Table: gpu_exec vs grid
    print("\n% Table 5.1: GPU execution time vs grid (ch16, dens50, seed42)")
    print(r"\begin{table}[ht]")
    print(r"\caption{GPU execution time (\textmu s) vs grid size (chunk=16, density=50\%, seed=42)}")
    print(r"\begin{tabular}{rrrrr}")
    print(r"\hline")
    print(r"Grid & Scheme 1 & Scheme 2 & Scheme 3 & S3/S1 (\%) \\ \hline")
    for g in (16, 64, 256, 512, 1024, 2048, 4096):
        vals = {}
        for s in (1,2,3):
            sub = sel(rows, scheme=s, grid=g, chunk=16, density=50, seed=42)
            if sub: vals[s] = f(sub[0], 'gpu_exec_avg')
        if len(vals)==3:
            pct = (vals[3]-vals[1])/vals[1]*100
            print(fr"{g} & {vals[1]:.0f} & {vals[2]:.0f} & {vals[3]:.0f} & {pct:+.1f} \\")
    print(r"\hline")
    print(r"\end{tabular}")
    print(r"\end{table}")

    # Table: CPU recording cost
    print("\n% Table 5.2: CPU recording cost vs chunk size (grid=1024, dens50, seed42)")
    print(r"\begin{table}[ht]")
    print(r"\caption{CPU recording cost (\textmu s) vs chunk size, grid=1024, density=50\%, seed=42}")
    print(r"\begin{tabular}{rrrrrr}")
    print(r"\hline")
    print(r"Chunk & Chunks$^2$ & Scheme 1 & Scheme 2 & Scheme 3 & S1/S3 \\ \hline")
    for c in (4, 8, 16):
        chunks_sq = (1024 // c) ** 2
        cr = {}
        for s in (1, 2, 3):
            sub = sel(rows, scheme=s, grid=1024, chunk=c, density=50, seed=42)
            if sub: cr[s] = f(sub[0], 'cpu_record_avg')
        if len(cr)==3:
            print(fr"{c} & {chunks_sq} & {cr[1]:.0f} & {cr[2]:.0f} & {cr[3]:.1f} & {cr[1]/cr[3]:.0f}$\times$ \\")
    print(r"\hline")
    print(r"\end{tabular}")
    print(r"\end{table}")

    # Table: Update cost
    print("\n% Table 5.3: Batched vs per-chunk update cost")
    print(r"\begin{table}[ht]")
    print(r"\caption{Local update cost (\textmu s) -- batched vs per-chunk submission, seed=42, mean across schemes}")
    print(r"\begin{tabular}{rrrr}")
    print(r"\hline")
    print(r"Update Size & Batched (avg) & Per-Chunk (avg) & Speedup \\ \hline")
    bd, pd = {}, {}
    for u in (1, 2, 4, 8, 16, 32):
        bvals = [f(r, 'update_cost_avg') for r in update_rows
                  if r['mode']=='batched' and int(r['seed'])==42 and int(r['update_size'])==u]
        pvals = [f(r, 'update_cost_avg') for r in update_rows
                  if r['mode']=='perchunk' and int(r['seed'])==42 and int(r['update_size'])==u]
        if bvals: bd[u] = st.mean(bvals)
        if pvals: pd[u] = st.mean(pvals)
    for u in sorted(bd):
        if u in pd:
            sp = pd[u] / bd[u]
            print(fr"{u} & {bd[u]:.0f} & {pd[u]:.0f} & {sp:.1f}$\times$ \\")
    print(r"\hline")
    print(r"\end{tabular}")
    print(r"\end{table}")

    # Table: seed x density interaction
    print("\n% Table 5.4: gpu_exec by seed x density (ch16, grid4096, scheme3)")
    print(r"\begin{table}[ht]")
    print(r"\caption{gpu\_exec (\textmu s) by seed $\times$ density (chunk=16, grid=4096, scheme 3)}")
    print(r"\begin{tabular}{rrrr}")
    print(r"\hline")
    print(r"Density & Seed 42 & Seed 1337 & Seed 9999 \\ \hline")
    for d in (20, 50, 80):
        vals = {}
        for seed in (42, 1337, 9999):
            sub = sel(rows, scheme=3, grid=4096, chunk=16, density=d, seed=seed)
            if sub: vals[seed] = f(sub[0], 'gpu_exec_avg')
        if len(vals) == 3:
            print(fr"{d}\% & {vals[42]:.0f} & {vals[1337]:.0f} & {vals[9999]:.0f} \\")
    print(r"\hline")
    print(r"\end{tabular}")
    print(r"\end{table}")


# -- Main --------------------------------------------------------------------
if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("--latex", action="store_true", help="Generate LaTeX tables")
    ap.add_argument("--section", type=str, help="Only run specific section (e.g. 5.1)")
    args = ap.parse_args()

    rows = load_render_agg()
    update_rows = load_update_agg()

    print(f"Loaded {len(rows)} render + {len(update_rows)} update configs\n")

    run = args.section or "all"

    if run == "all" or run == "5.1":
        section_5_1(rows)
    if run == "all" or run == "5.2":
        section_5_2(rows)
    if run == "all" or run == "5.3":
        section_5_3(rows)
    if run == "all" or run == "5.4":
        section_5_4(update_rows, rows)

    if args.latex:
        generate_latex_tables(rows, update_rows)
