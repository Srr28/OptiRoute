# OptiRoute

### Large-Scale Network Optimization and Resource Allocation Engine

OptiRoute is a C++17 project for modeling capacitated logistics networks and studying maximum throughput, bottlenecks, and budget-constrained facility selection.

- **Phase 1:** Ford–Fulkerson, Dinic's maximum-flow algorithm, and minimum-cut validation on a fixed network.
- **Phase 2:** budget-constrained selection of optional distribution centers using **Backtracking**, **Greedy**, and **Local Search**. Each candidate solution is evaluated by rebuilding the network and running Dinic.
- **Large-graph validation:** a Dinic-only validator for the synthetic India-scale dataset.

> **Data provenance:** The India-scale dataset is a synthetic simulation inspired by publicly described logistics-network scale. It is **not** Amazon's internal data, an actual facility inventory, a real road map, or measured package throughput. All generated route capacities, supplies, and demands are model parameters. See [`data/large/README.md`](data/large/README.md).

## Repository layout

```text
optiroute/
├── README.md
├── LICENSE
├── CMakeLists.txt
├── src/
│   ├── optiroute.cpp        # Phase 1: Ford–Fulkerson + Dinic + minimum cut
│   └── phase2.cpp           # Phase 2: Backtracking + Greedy + Local Search
├── data/
│   ├── examples/
│   │   ├── small_network.txt
│   │   ├── medium_network.txt
│   │   └── phase2_facility_selection.txt
│   ├── phase2/              # Phase 2 medium benchmark, metadata, generator
│   └── large/               # Synthetic India-scale Phase 1 input and CSV datasets
└── validator/
    ├── dinic_validator.cpp  # Dinic-only large-graph validator
    ├── expected_large_output.txt
    └── README.md
```

No smoke-test framework or smoke-test script is included.

## Requirements

- C++17 compiler (GCC, Clang, or recent MSVC)
- CMake 3.16+ (optional)

## Build on Windows (PowerShell)

From the repository root:

```powershell
g++ -O2 -std=c++17 src/optiroute.cpp -o optiroute.exe
g++ -O2 -std=c++17 src/phase2.cpp -o optiroute_phase2.exe
g++ -O2 -std=c++17 validator/dinic_validator.cpp -o optiroute_validate_large.exe
```

## Phase 1: fixed-network maximum flow

Run the combined Ford–Fulkerson/Dinic implementation on a small example:

```powershell
cmd /c ".\optiroute.exe < data\examples\small_network.txt"
```

The program reports both maximum-flow results, their runtimes, and the minimum cut from Dinic's residual graph. The max-flow/min-cut check should pass.

For the country-scale synthetic graph, run the Dinic-only validator:

```powershell
cmd /c ".\optiroute_validate_large.exe < data\large\india_amazon_scale_input.txt"
```

The combined Phase 1 executable runs DFS-based Ford–Fulkerson first, which may be very slow on the large graph. Use the Dinic-only validator for that dataset.

## Phase 2: facility selection under a budget

Run the example:

```powershell
cmd /c ".\optiroute_phase2.exe < data\examples\phase2_facility_selection.txt"
```

The example is intentionally small and demonstrates complementarity: two candidate DCs together provide more flow than the individually attractive choice selected by Greedy. Backtracking finds the exact best subset, while Local Search improves the Greedy result using add, remove, swap, pair-add, and (for at most 40 candidates) one-for-two pair-swap moves. A reproducible medium benchmark is available under `data/phase2/`.

### Phase 2 input format

All values are whitespace-separated; line breaks are for readability. Node IDs follow the same Phase 1 convention.

```text
W D C
budget
W supply values
C demand values
D rows: opening_cost handling_capacity status
M
M rows: from_node to_node capacity
```

- `W`: number of warehouses.
- `D`: total number of distribution centers (existing and optional).
- `C`: number of customer zones.
- `budget`: maximum total opening cost for optional DCs.
- Warehouse supplies: one nonnegative integer per warehouse.
- Customer demands: one nonnegative integer per customer zone.
- Each DC row contains:
  - `opening_cost`: nonnegative integer. Must be `0` for existing facilities.
  - `handling_capacity`: maximum throughput of that DC.
  - `status`: `0` for an existing DC (always active), `1` for an optional candidate (initially closed).
- `M`: number of directed transportation routes.
- Each route row contains `from_node to_node capacity` with node IDs in `1..W+D+C`.

Node IDs are assigned as follows:

- Warehouses: `1..W`
- Distribution centers: `W+1..W+D`
- Customer zones: `W+D+1..W+D+C`

Phase 2 internally splits each node into an in-node and an out-node. The edge between the two sides of a DC enforces its handling capacity. For a closed candidate DC, that edge has capacity zero, preventing flow through that facility. Route capacities remain independent of opening costs; the opening cost only affects the budget constraint.

### Phase 2 algorithms

- **Backtracking:** enumerates feasible candidate subsets and returns the exact optimum when the candidate count is at most 18. It is exponential and is intended as a baseline for small instances.
- **Greedy:** repeatedly opens the affordable candidate with the largest positive marginal flow gain per unit opening cost. It is fast relative to exhaustive search but is not guaranteed to find the optimum and can miss complementary facilities.
- **Local Search:** starts from the Greedy solution and evaluates single add/remove moves, one-for-one swaps, and—when there are at most 40 candidates—pair-add and one-for-two pair-swap moves. This can escape some greedy choices but is still a heuristic, not an optimality guarantee.
- **Dinic:** evaluates the maximum deliverable flow for each facility subset.

The program prints selected candidate DC indices (1-based within the DC list), opening cost, maximum flow, runtime, and the flow gap versus Backtracking when the exact result is available. It also reports the number of max-flow evaluations because each evaluation rebuilds the residual network. Existing DCs are always active and are not included in the opening budget.

**Scalability note:** this Phase 2 implementation is designed for small/medium candidate-selection instances. Backtracking is limited to 18 candidates; Greedy and Local Search can still be expensive on large graphs because they repeatedly run Dinic. Do not run all methods over thousands of candidates without further optimization or a reduced candidate pool.


### Medium Phase 2 benchmark

Run the supplied medium instance (10 warehouses, 30 DCs, 40 customer zones, 6 existing DCs, 24 candidate DCs, and 275 directed routes):

```powershell
cmd /c ".\\optiroute_phase2.exe < data\\phase2\\phase2_medium_input.txt"
```

This benchmark intentionally has 24 candidates, above the exact-search limit, so Backtracking reports that it is skipped while Greedy and Local Search run. The instance and its companion CSV are synthetic. See [`data/phase2/README.md`](data/phase2/README.md) and [`data/phase2/manifest.json`](data/phase2/manifest.json) for provenance and regeneration instructions.

### India-scale Phase 2 benchmark

Phase 2 also includes a large input derived from the Phase 1 India-scale synthetic network: 100 warehouses, 2,630 DCs, 19,000 customer zones, and 37,436 routes. It has 2,618 existing DCs and 12 optional candidate DCs. The candidate pool is deliberately limited to keep repeated max-flow evaluations more practical.

```powershell
cmd /c ".\optiroute_phase2.exe --heuristics-only < data\phase2\phase2_india_scale_input.txt"
```

Use `--heuristics-only` on this large graph to skip exhaustive Backtracking. Greedy and Local Search still run repeated Dinic computations and may take substantially longer than the small/medium examples. The facility costs and handling capacities are synthetic assumptions. See [`data/phase2/README.md`](data/phase2/README.md).

## Synthetic India-scale dataset summary

| Metric | Value |
|---|---:|
| Warehouses / source nodes | 100 |
| Distribution-center-like nodes | 2,630 |
| Customer destination zones | 19,000 |
| Real graph nodes | 21,730 |
| Directed transport routes | 37,436 |
| Synthetic total supply | 5,606,465 units/day |
| Synthetic total demand | 5,191,173 units/day |
| Recorded Dinic maximum flow | 5,041,042 units/day |
| Recorded demand satisfaction | 97.11% |
| Max-flow/min-cut check | PASS |

Runtime depends on machine and compiler. These are synthetic modeled units/day, not package counts from a real operator.

## Core concepts

- **Maximum flow:** computes the maximum deliverable quantity through a capacitated directed graph.
- **Minimum cut:** identifies a capacity bottleneck; its capacity equals maximum flow for a completed max-flow computation.
- **Facility selection:** chooses optional DCs under a budget to maximize network throughput.
- **Exact vs heuristic optimization:** Backtracking provides an exact baseline for small instances; Greedy and Local Search trade guarantees for reduced search effort.

## License

MIT. Review the license and update its copyright line if needed before publishing.
