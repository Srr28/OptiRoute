#!/usr/bin/env python3
"""Generate a deterministic, synthetic Phase 2 facility-selection benchmark."""
import argparse, csv, random
from pathlib import Path

def generate(out_path: Path, seed: int = 20261022):
    rng = random.Random(seed)
    W, D, C, budget = 10, 30, 40, 210
    supply = [rng.randint(700, 1200) for _ in range(W)]
    demand = [rng.randint(150, 400) for _ in range(C)]
    facilities = [(0, rng.randint(900, 1600), 0) if i < 6 else
                  (rng.randint(25, 90), rng.randint(350, 950), 1) for i in range(D)]
    routes = []
    for di in range(D):
        for w in rng.sample(range(1, W + 1), k=2):
            routes.append((w, W + di + 1, rng.randint(250, 850)))
    for _ in range(30):
        routes.append((rng.randint(1, W), W + rng.randrange(D) + 1, rng.randint(150, 650)))
    for ci in range(C):
        customer = W + D + ci + 1
        for di in rng.sample(range(D), k=4):
            routes.append((W + di + 1, customer, rng.randint(100, 600)))
    for _ in range(25):
        a, b = rng.sample(range(D), k=2)
        routes.append((W + a + 1, W + b + 1, rng.randint(100, 350)))
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("w", encoding="utf-8", newline="\n") as f:
        f.write(f"{W} {D} {C}\n{budget}\n")
        f.write(" ".join(map(str, supply)) + "\n")
        f.write(" ".join(map(str, demand)) + "\n")
        for cost, cap, status in facilities:
            f.write(f"{cost} {cap} {status}\n")
        f.write(f"{len(routes)}\n")
        for u, v, cap in routes:
            f.write(f"{u} {v} {cap}\n")
    with out_path.with_name("phase2_candidate_facilities.csv").open("w", encoding="utf-8", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["dc_index_1_based", "node_id", "opening_cost", "handling_capacity", "status"])
        for i, (cost, cap, status) in enumerate(facilities, start=1):
            writer.writerow([i, W+i, cost, cap, "existing" if status == 0 else "candidate"])
    print(f"Wrote {out_path}; W={W}, D={D}, C={C}, routes={len(routes)}, budget={budget}, candidates={sum(x[2] for x in facilities)}")

if __name__ == "__main__":
    p = argparse.ArgumentParser()
    p.add_argument("--output", default="phase2_medium_input.txt")
    p.add_argument("--seed", type=int, default=20261022)
    a = p.parse_args()
    generate(Path(a.output), a.seed)
