# Phase 2 datasets

Phase 2 solves budget-constrained distribution-center (DC) selection. These files are deliberately separate from `data/large/`, which remains the Phase 1 fixed-network benchmark.

## Files

- `phase2_medium_input.txt`: deterministic medium-sized benchmark accepted directly by `src/phase2.cpp`.
- `phase2_candidate_facilities.csv`: human-readable facility metadata matching the generated input.
- `generate_phase2_medium.py`: reproducible generator (default seed `20261022`).

The medium benchmark contains 10 warehouses, 30 DCs, 40 customer zones, 275 directed routes, 6 existing DCs, 24 optional candidate DCs, and a budget of 210 synthetic cost units. Costs, capacities, supplies, and demands are synthetic model values, not actual company data. The 24-candidate dataset intentionally exceeds the exact Backtracking limit, so it demonstrates the heuristics. Use `data/examples/phase2_facility_selection.txt` for an instance where Backtracking can verify the optimum.

## Run

From the repository root:

```bash
g++ -O2 -std=c++17 src/phase2.cpp -o optiroute_phase2
./optiroute_phase2 < data/phase2/phase2_medium_input.txt
```

Windows PowerShell:

```powershell
g++ -O2 -std=c++17 src/phase2.cpp -o optiroute_phase2.exe
cmd /c ".\optiroute_phase2.exe < data\phase2\phase2_medium_input.txt"
```

Regenerate the medium input and matching CSV:

```bash
python3 data/phase2/generate_phase2_medium.py --output data/phase2/phase2_medium_input.txt --seed 20261022
```

Changing the seed creates a different reproducible instance. The input format is documented in the root README.

## India-scale Phase 2 benchmark

- `phase2_india_scale_input.txt`: Phase 2-formatted version of the large Phase 1 graph (100 warehouses, 2,630 DCs, 19,000 customer zones, 37,436 routes).
- `phase2_india_scale_candidate_facilities.csv`: metadata for all 2,630 DCs; 2,618 are existing and 12 are optional candidates spread across the DC ID range.
- `phase2_india_scale_manifest.json`: dimensions, candidate IDs, budget, and synthetic-data provenance.
- `generate_phase2_india_scale.py`: regenerates these files from the Phase 1 input with a fixed seed.

This is a large graph with a deliberately small candidate pool so the heuristic algorithms remain practical on a personal computer. **Run with `--heuristics-only`** to skip exhaustive Backtracking. Even Greedy and Local Search perform repeated maximum-flow evaluations, so runtime depends on hardware. The facility costs and handling capacities are synthetic assumptions, not actual Amazon data.

Windows PowerShell from the repository root:

```powershell
g++ -O2 -std=c++17 src/phase2.cpp -o optiroute_phase2.exe
cmd /c ".\\optiroute_phase2.exe --heuristics-only < data\\phase2\\phase2_india_scale_input.txt"
```

Regenerate the large Phase 2 dataset:

```powershell
python data/phase2/generate_phase2_india_scale.py --phase1-input data/large/india_amazon_scale_input.txt --output-dir data/phase2 --seed 20261022
```
