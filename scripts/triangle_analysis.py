"""Cross-check whether gpu_exec is explained by total triangle count (mesh
composition) rather than by seed/density identity per se.
Tile IDs (WFCGenerator.cpp s_tiles): 0=EMPTY, 1=LOW(cube), 2=MID(cube),
3=HIGH(cube), 4=SPHERE_S, 5=SPHERE_L.
Mesh triangle counts (verified from Assets/Meshes/*.msh headers): cube=12, sphere=768.
"""
import struct, csv
from pathlib import Path
from collections import Counter

CACHE = Path("cache")
CUBE_TRI, SPHERE_TRI = 12, 768
CUBE_IDS, SPHERE_IDS = {1, 2, 3}, {4, 5}

def load_histogram(grid, seed, density):
    path = CACHE / f"wfc_{grid}_{seed}_{density}.bin"
    with open(path, "rb") as f:
        (gridSize,) = struct.unpack("<I", f.read(4))
        n = gridSize * gridSize
        data = f.read(n * 4)
        tiles = struct.unpack(f"<{n}I", data)
    return Counter(tiles)

def total_triangles(hist):
    cube_n = sum(hist.get(t, 0) for t in CUBE_IDS)
    sphere_n = sum(hist.get(t, 0) for t in SPHERE_IDS)
    return cube_n * CUBE_TRI + sphere_n * SPHERE_TRI, cube_n, sphere_n

def load_gpu_exec():
    rows = list(csv.DictReader(open("results/_aggregate.csv")))
    out = {}
    for r in rows:
        if int(r['chunk']) == 16 and int(r['scheme']) == 3 and int(r['grid']) == 4096:
            key = (int(r['seed']), int(r['density']))
            out[key] = float(r['gpu_exec_avg'])
    return out

if __name__ == "__main__":
    gpu = load_gpu_exec()
    print(f"{'seed':>6} {'dens':>5} {'cube_n':>10} {'sphere_n':>10} {'triangles':>14} {'gpu_exec_us':>12} {'ns/Mtri':>10}")
    points = []
    for seed in (42, 1337, 9999):
        for density in (20, 50, 80):
            hist = load_histogram(4096, seed, density)
            tris, cube_n, sphere_n = total_triangles(hist)
            ge = gpu.get((seed, density))
            ns_per_mtri = (ge * 1000.0 / (tris / 1e6)) if ge and tris else float('nan')
            points.append((seed, density, tris, ge))
            print(f"{seed:>6} {density:>5} {cube_n:>10} {sphere_n:>10} {tris:>14} {ge:>12.1f} {ns_per_mtri:>10.2f}")

    print()
    print("-- sorted by triangle count (monotonicity check) --")
    for seed, density, tris, ge in sorted(points, key=lambda p: p[2]):
        print(f"  tris={tris:>14,}  gpu_exec={ge:>10.1f}us  (seed={seed}, dens={density})")
