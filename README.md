# OptiRoute

### Large-Scale Network Optimization and Resource Allocation Engine

OptiRoute is a C++17 project for studying capacity-constrained logistics networks with graph algorithms. It computes maximum deliverable flow, extracts a minimum cut to reveal network bottlenecks, and validates the max-flow/min-cut equality. The repository includes two algorithm approaches—DFS-based Ford–Fulkerson and Dinic—plus small, medium, and country-scale synthetic datasets.

> **Data note:** The India-scale graph is synthetic and inspired by the scale of nationwide logistics systems. It is not Amazon's internal network, not a measured road network, and not actual warehouse/customer data. Capacities and demand are simulated model units per day. Read [`data/large/README.md`](data/large/README.md) for provenance and assumptions.

## Features

- **Ford–Fulkerson:** DFS augmenting paths; useful for learning and small/medium graphs.
- **Dinic:** BFS level graph plus DFS blocking flow; preferred for the included large graph.
- **Minimum-cut analysis:** reports the capacity of the cut induced by Dinic's final residual graph.
- **Correctness check:** verifies that the computed max-flow value equals the cut capacity.
- **Multiple datasets:** a small deterministic example, a medium synthetic graph, and a large India-scale graph with node and route metadata.
- **Large-network validator:** a separate Dinic-only validator checks the country-scale input and verifies max-flow/min-cut equality.

## Repository layout

```text
optiroute/
├── README.md
├── LICENSE
├── .gitignore
├── .gitattributes
├── CMakeLists.txt
├── src/
│   └── optiroute.cpp                 # Ford–Fulkerson + Dinic + min-cut
├── data/
│   ├── README.md
│   ├── examples/
│   │   ├── small_network.txt
│   │   └── medium_network.txt
│   └── large/
│       ├── india_amazon_scale_input.txt
│       ├── india_amazon_scale_nodes.csv
│       ├── india_amazon_scale_routes.csv
│       ├── india_amazon_scale_demand_by_state.csv
│       ├── manifest.json
│       ├── generate_india_logistics_scale.py
│       └── README.md
└── validator/
    ├── dinic_validator.cpp
    └── README.md
```

## Requirements

- A C++17 compiler such as GCC or Clang (the current source uses `<bits/stdc++.h>`, so MSVC is not supported without adapting the includes).
- Python 3 is only needed if you want to regenerate the synthetic country-scale dataset; it is not required to compile or run the C++ programs.
- CMake 3.16+ is optional; direct `g++` commands work too.

## Build and run on Windows PowerShell

Compile the main program and the large-graph validator:

```powershell
g++ -O2 -std=c++17 src/optiroute.cpp -o optiroute.exe
g++ -O2 -std=c++17 validator/dinic_validator.cpp -o optiroute_validate_large.exe
```

Run the combined solver against the small example to inspect both algorithm outputs:

```powershell
cmd /c ".\optiroute.exe < data\examples\small_network.txt"
```

Expected results for this example: Ford–Fulkerson flow `15`, Dinic flow `15`, minimum-cut capacity `15`, and `Max-flow/min-cut check: PASS`.

Run the combined solver on the medium graph:

```powershell
cmd /c ".\optiroute.exe < data\examples\medium_network.txt"
```

Run the country-scale dataset with the Dinic-only validator:

```powershell
cmd /c ".\optiroute_validate_large.exe < data\large\india_amazon_scale_input.txt"
```

**Why a separate large validator?** The main executable runs Ford–Fulkerson and Dinic sequentially. DFS-based Ford–Fulkerson may take a long time on the country-scale graph, so the separate validator uses Dinic only and checks the max-flow/min-cut equality without waiting for Ford–Fulkerson.

## Build with CMake (optional)

```powershell
cmake -S . -B build
cmake --build build --config Release
```


## Input format

The C++ programs use this whitespace-separated format:

1. Header: `W D C` — number of warehouses, distribution centers, and customer zones.
2. `W` nonnegative integer supply values.
3. `C` nonnegative integer demand values.
4. `M` — number of directed transport routes.
5. `M` rows containing `from_id to_id capacity`.

Real nodes use 1-based IDs: warehouses are `1..W`, distribution centers are `W+1..W+D`, and customer zones are `W+D+1..W+D+C`. The solver adds a super-source linked to warehouses by supply capacities and links customer zones to a super-sink by demand capacities.

## Country-scale benchmark

| Metric | Value |
|---|---:|
| Warehouse/source nodes | 100 |
| Distribution/transfer nodes | 2,630 |
| Customer destination zones | 19,000 |
| Real nodes | 21,730 |
| Directed transport routes | 37,436 |
| Synthetic total supply | 5,606,465 units/day |
| Synthetic total demand | 5,191,173 units/day |
| Recorded Dinic maximum flow | 5,041,042 units/day |
| Recorded demand satisfied | 97.11% |
| Max-flow/min-cut check | PASS |

Run time depends on hardware, compiler, build flags, and system load. Do not treat one runtime as a universal performance guarantee. Flow capacities are **synthetic model units per day**, not package counts or measurements reported by Amazon.

## How the algorithms work

- **Maximum flow** finds the greatest total flow that can be routed from source to sink without exceeding edge capacities.
- **Ford–Fulkerson** repeatedly searches for an augmenting path with DFS and updates residual capacities.
- **Dinic** builds BFS levels and sends blocking flow with DFS, then repeats until no source-to-sink path remains.
- **Minimum cut** is obtained by finding vertices still reachable from the source in the final residual graph. The sum of original capacities crossing from reachable to unreachable vertices must equal maximum flow.

## Current scope and roadmap

**Implemented:** maximum flow, minimum-cut extraction, two flow approaches, large graph validation, and deterministic input examples.

**Planned next:** budget-constrained facility selection; exact backtracking for small cases; greedy, local search, and simulated annealing for larger cases; a repeatable benchmark harness with median runtimes and solution-quality comparisons. These facility-selection features are not implemented in the current version.

## License

Released under the MIT License. See [`LICENSE`](LICENSE).
