# OptiRoute

**Logistics Network Flow and Facility Optimization in C++17**

OptiRoute is a graph-algorithms project that models a directed logistics network. Warehouses provide supply, distribution centers (DCs) transfer and process goods, and customer zones have demand. The project measures how much demand the network can serve and studies which optional DCs should be opened when there is a limited budget.

The code uses a straightforward competitive-programming style: standard C++17, adjacency lists, `vector`, `queue`, recursive DFS, and `long long` capacities. It has no external libraries or build system.

## Contents

```text
optiroute/
├── README.md
├── src/
│   ├── network_flow.cpp
│   └── facility_optimizer.cpp
└── data/
    ├── network_flow/
    │   ├── small.txt
    │   ├── medium.txt
    │   └── large.txt
    └── facility_optimization/
        ├── small.txt
        ├── medium.txt
        └── large.txt
```

## What the project does

### Phase 1: Network flow

`src/network_flow.cpp` runs two maximum-flow algorithms on the same network:

- **Ford–Fulkerson:** repeatedly finds an augmenting path using DFS in the residual graph.
- **Dinic:** builds a level graph with BFS and sends blocking flow using DFS.

The program reports both maximum-flow values and runtimes. It also builds a minimum cut from Dinic's final residual graph and checks that the cut capacity equals the maximum flow.

### Phase 2: Facility selection

`src/facility_optimizer.cpp` chooses optional DCs while respecting an opening budget. For each proposed selection, it rebuilds the flow network and uses Dinic to calculate the throughput.

It compares three approaches:

1. **Backtracking (exact):** explores all budget-feasible combinations, up to 18 optional DCs. It returns an optimal selection within that limit.
2. **Greedy heuristic:** repeatedly opens the affordable DC with the highest positive marginal flow gain per unit of opening cost. It is a heuristic; no approximation ratio is claimed.
3. **Local Search:** starts from the Greedy solution and checks single-DC additions, two-DC additions, one-for-two replacements, removals, and one-for-one swaps. It accepts an improvement in flow, or a lower-cost solution with the same flow. Pair additions and one-for-two replacements are enabled for at most 40 candidates.

The three methods optimize the same primary objective: **maximize delivered flow subject to the opening budget**. If two solutions deliver equal flow, the lower-cost solution is preferred.

## How DC handling capacity works

A route capacity limits how much flow can travel along one directed route. A DC handling capacity limits how much total flow can pass through that DC, summed across all its incoming and outgoing routes.

Both programs enforce DC capacity by **node splitting**. Each real node `v` is represented by `in(v)` and `out(v)`, joined by an edge:

```text
in(v) -- handling capacity --> out(v)
```

For DCs, this edge has the DC's configured handling capacity. Warehouse and customer split edges receive a sufficiently large capacity; their actual limits are enforced by the source-to-warehouse supply edges and customer-to-sink demand edges. Transportation routes connect `out(u)` to `in(v)`.

In Phase 2, a closed optional DC receives handling capacity zero. An opened DC receives its configured handling capacity. Existing DCs are always active and do not consume the opening budget.

The Phase 1 datasets now include explicit DC handling capacities as well. Their synthetic capacities are generated from the smaller of each DC's aggregate incoming-route capacity and aggregate outgoing-route capacity, then stored in the input file so that the constraint is applied by the program.

## Input formats

All node IDs are 1-based:

- Warehouses: `1` through `W`
- DCs: `W+1` through `W+D`
- Customers: `W+D+1` through `W+D+C`

All capacities, supplies, demands, and opening costs must be nonnegative integers. Routes are directed.

### Phase 1 input: `data/network_flow/*.txt`

```text
W D C
W supply values
C demand values
D DC handling-capacity values
M
M lines: from_node to_node route_capacity
```

For example, the small dataset begins with `2 2 3`: two warehouses, two DCs, and three customers. The following lines provide two supplies, three demands, two DC handling capacities, and then the route count and route rows.

### Phase 2 input: `data/facility_optimization/*.txt`

```text
W D C
budget
W supply values
C demand values
D lines: opening_cost handling_capacity status
M
M lines: from_node to_node route_capacity
```

For each DC row, `status = 0` means the DC already exists and is always active. Its opening cost must be zero. `status = 1` means it is an optional candidate that the algorithms may open, subject to the budget.

## Build and run

Run these commands from the repository root. No CMake or Makefile is required.

### Linux, macOS, or Fish shell

Compile Phase 1:

```fish
g++ -O2 -std=c++17 -Wall -Wextra -pedantic src/network_flow.cpp -o network_flow
```

Run Phase 1 with the small, medium, or large dataset:

```fish
./network_flow < data/network_flow/small.txt
./network_flow < data/network_flow/medium.txt
./network_flow < data/network_flow/large.txt
```

Compile Phase 2:

```fish
g++ -O2 -std=c++17 -Wall -Wextra -pedantic src/facility_optimizer.cpp -o facility_optimizer
```

Run the facility optimizer:

```fish
./facility_optimizer < data/facility_optimization/small.txt
./facility_optimizer < data/facility_optimization/medium.txt
./facility_optimizer < data/facility_optimization/large.txt
```

### Windows PowerShell

```powershell
g++ -O2 -std=c++17 -Wall -Wextra -pedantic src/network_flow.cpp -o network_flow.exe
cmd /c ".\network_flow.exe < data\network_flow\small.txt"

g++ -O2 -std=c++17 -Wall -Wextra -pedantic src/facility_optimizer.cpp -o facility_optimizer.exe
cmd /c ".\facility_optimizer.exe < data\facility_optimization\small.txt"
```

Replace `small.txt` with `medium.txt` or `large.txt` to run a different dataset.

## Understanding the output

### Phase 1

- `Ford-Fulkerson maximum flow` and `Dinic maximum flow`: total modeled units per day that can be sent from warehouses to customer zones.
- `Algorithm agreement`: `PASS` when both algorithms return the same flow.
- `Minimum-cut capacity`: capacity of the cut found using the final residual graph. The cut can include supply edges, route edges, DC handling-capacity edges, or customer-demand edges.
- `Max-flow/min-cut check`: `PASS` when the computed cut capacity equals Dinic's maximum flow.
- Runtime values are measured on the machine executing the program and will vary by hardware and system load.

### Phase 2

Each algorithm reports maximum flow, opening cost, budget, and selected optional DC IDs. Runtime and flow-evaluation counts are also printed. One flow evaluation reconstructs the network and runs Dinic, so its cost depends on the graph size.

Backtracking is skipped when there are more than 18 optional candidates. Greedy and Local Search still run. This limit is a practical guard against exponential search, not a limitation of the optimization model itself.

## Algorithms and complexity

Let `V` be the number of real nodes, `E` the number of directed routes, and `K` the number of optional DCs.

- **Ford–Fulkerson:** DFS-based augmenting paths. Runtime depends on capacities and the augmenting-path sequence; it can be slow on large networks.
- **Dinic:** level-graph BFS and blocking-flow DFS. The standard general-graph bound is `O(V^2 E)`; practical performance depends on graph structure.
- **Backtracking:** up to `O(2^K)` facility subsets, each requiring a maximum-flow evaluation. Budget pruning safely skips branches whose accumulated opening cost exceeds the budget.
- **Greedy:** at each step evaluates each affordable unopened DC to estimate its marginal flow gain. This implementation is a heuristic and does not promise a globally optimal result or a formal approximation ratio.
- **Local Search:** repeatedly evaluates neighboring selections (add, pair-add, one-for-two replacement, remove, and swap). It stops when no tested neighbor improves the current solution. It is a heuristic and can stop at a local optimum.

Because every subset or neighborhood move triggers a fresh flow computation, facility optimization can take significantly longer than a single Phase 1 run.

## Datasets and assumptions

The repository includes small, medium, and large text inputs for each phase. The large network-flow case models 100 warehouses, 2,630 DCs, 19,000 customer zones, and 37,436 directed routes. The datasets are synthetic and intended for algorithmic experiments; they are not actual company logistics records.

The model assumes:

- A static directed network with integer capacities.
- Warehouse supply, customer demand, route capacity, DC handling capacity, and facility opening cost are known inputs.
- Each unit of flow consumes one unit of capacity on every traversed route and at each traversed DC.
- DC handling capacity is shared across all flow passing through that DC.
- Opening a candidate DC enables its handling capacity; it does not create or modify transportation routes.
- The optimization objective is maximum flow first, then minimum opening cost among equal-flow solutions.

## Limitations

- Backtracking is exact only when it completes the full search; the implementation limits it to 18 candidates by default.
- Greedy and Local Search are useful baselines but do not have a proven approximation guarantee for this objective.
- The synthetic data is a simplified model. It does not include travel time, vehicle routing, multiple commodities, uncertainty, or time-varying capacity.
- Ford–Fulkerson is included for algorithm comparison and may be slow on the large network.

## Repository design

The project intentionally keeps the source tree small: two C++ files, two data folders, six `.txt` inputs, and this README. There are no external dependencies, data-generation scripts, or test-framework scaffolding.
