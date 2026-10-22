#!/usr/bin/env python3
"""Convert the Phase 1 India-scale synthetic network into a Phase 2 benchmark.

The generated graph, costs, and capacities are synthetic and are not actual
Amazon/internal logistics data. The Phase 1 input is reused for identical
warehouse supplies, customer demands, and directed routes.
"""
import argparse
import csv
import json
import random
from pathlib import Path


def parse_phase1(path):
    values = list(map(int, path.read_text(encoding="utf-8").split()))
    pos = 0
    W, D, C = values[pos:pos + 3]
    pos += 3
    supply = values[pos:pos + W]
    pos += W
    demand = values[pos:pos + C]
    pos += C
    M = values[pos]
    pos += 1
    routes = []
    for _ in range(M):
        routes.append(tuple(values[pos:pos + 3]))
        pos += 3
    if pos != len(values):
        raise ValueError("Unexpected trailing values in Phase 1 input")
    return W, D, C, supply, demand, routes


def generate(phase1_path, out_dir, seed=20261022):
    W, D, C, supply, demand, routes = parse_phase1(phase1_path)
    N = W + D + C
    incoming = [0] * (N + 1)
    outgoing = [0] * (N + 1)
    for u, v, cap in routes:
        outgoing[u] += cap
        incoming[v] += cap

    # Twelve candidates spaced throughout the DC ID range keep the large-graph
    # heuristic run usable on a personal computer. Backtracking is skipped.
    candidates = {(j * (D - 1)) // 11 for j in range(12)}
    rng = random.Random(seed)
    facilities = []
    total_cost = 0
    for idx in range(D):
        node = W + idx + 1
        if idx in candidates:
            cost = rng.randint(700, 1800)
            cap = max(1, min(incoming[node], outgoing[node]))
            status = 1
            total_cost += cost
        else:
            cost = 0
            cap = max(1, max(incoming[node], outgoing[node]))
            status = 0
        facilities.append((cost, cap, status))

    budget = int(total_cost * 0.65)
    out_dir.mkdir(parents=True, exist_ok=True)
    target = out_dir / "phase2_india_scale_input.txt"
    with target.open("w", encoding="utf-8", newline="\n") as f:
        f.write(f"{W} {D} {C}\n{budget}\n")
        f.write(" ".join(map(str, supply)) + "\n")
        f.write(" ".join(map(str, demand)) + "\n")
        for cost, cap, status in facilities:
            f.write(f"{cost} {cap} {status}\n")
        f.write(f"{len(routes)}\n")
        for u, v, cap in routes:
            f.write(f"{u} {v} {cap}\n")

    with (out_dir / "phase2_india_scale_candidate_facilities.csv").open(
        "w", encoding="utf-8", newline=""
    ) as f:
        writer = csv.writer(f)
        writer.writerow(["dc_index_1_based", "node_id", "opening_cost",
                         "handling_capacity", "status"])
        for idx, (cost, cap, status) in enumerate(facilities, 1):
            writer.writerow([idx, W + idx, cost, cap,
                             "candidate" if status else "existing"])

    manifest = {
        "dataset": "OptiRoute Phase 2 India-scale facility-selection benchmark",
        "synthetic_not_actual_amazon_data": True,
        "seed": seed,
        "derived_from": str(phase1_path),
        "warehouses": W, "distribution_centres": D,
        "customer_zones": C, "real_nodes": N, "directed_routes": len(routes),
        "existing_facilities": D - len(candidates),
        "candidate_facilities": len(candidates),
        "candidate_dc_indices_1_based": [i + 1 for i in sorted(candidates)],
        "budget_synthetic_cost_units": budget,
        "opening_cost_total_candidates": total_cost,
        "recommended_mode": "--heuristics-only",
        "capacity_note": (
            "Existing DC capacities are permissive relative to adjacent route "
            "capacities. Candidate DC capacities use min(total incoming, total "
            "outgoing route capacity). Costs and capacities are synthetic assumptions."
        ),
    }
    (out_dir / "phase2_india_scale_manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n", encoding="utf-8"
    )
    print(f"Wrote {target}: {W} warehouses, {D} DCs, {C} customer zones, "
          f"{len(routes)} routes, {len(candidates)} candidates, budget {budget}.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--phase1-input", default="data/large/india_amazon_scale_input.txt")
    parser.add_argument("--output-dir", default="data/phase2")
    parser.add_argument("--seed", type=int, default=20261022)
    args = parser.parse_args()
    generate(Path(args.phase1_input), Path(args.output_dir), args.seed)
