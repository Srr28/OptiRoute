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
