# OptiRoute

**A C++17 logistics-network optimization project**

OptiRoute models a directed logistics network and applies maximum-flow algorithms to measure how much supply can reach customer demand. It also models a budget-constrained facility-selection problem: choose which optional distribution centers (DCs) to open so that the network delivers as much flow as possible without exceeding the available budget.

The repository is intentionally organized around two readable C++ programs and six plain-text input files. There are no test frameworks, smoke-test scripts, or additional runtime dependencies.

## Repository structure

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

### Programs

| File | Purpose |
|---|---|
| `src/network_flow.cpp` | Runs Ford–Fulkerson and Dinic on the same network, reports maximum flow and runtimes, and verifies the max-flow/min-cut relationship. |
| `src/facility_optimizer.cpp` | Selects optional DCs under a budget using Backtracking, Greedy, and Local Search. Dinic evaluates the flow of each selection. |

### Datasets

Each program has its own input format, so use files only from the corresponding data folder.

| Dataset | Intended use |
|---|---|
| `small.txt` | Understand the input format and verify behavior quickly. |
| `medium.txt` | Run a more representative benchmark while keeping execution practical. |
| `large.txt` | Run a larger synthetic logistics-network experiment. |

The large network-flow dataset contains a synthetic network with 100 warehouses, 2,630 DC-like nodes, 19,000 customer zones, and 37,436 directed routes. The large facility-optimization dataset uses the same overall node scale and a budget with a smaller set of optional candidate DCs. Opening costs and facility capacities are modeled values. These are **synthetic demonstration datasets**, not actual Amazon operational data.

## Requirements

- A C++17-compatible compiler; `g++` is recommended.
- Windows PowerShell or Command Prompt, Linux, or macOS terminal.
- Enough memory for the selected dataset.

No third-party C++ libraries are required.

## Quick start on Windows

Open PowerShell in the repository root—the directory containing this README.

### 1. Build and run the network-flow program

Compile:

```powershell
g++ -O2 -std=c++17 src/network_flow.cpp -o network_flow.exe
```

Run the small input:

```powershell
cmd /c ".\network_flow.exe < data\network_flow\small.txt"
```

Run the medium input:

```powershell
cmd /c ".\network_flow.exe < data\network_flow\medium.txt"
```

Run the large input:

```powershell
cmd /c ".\network_flow.exe < data\network_flow\large.txt"
```

**Performance note:** Ford–Fulkerson uses DFS to find augmenting paths and can be slow on a large graph. The large run executes both algorithms for comparison, so it may take noticeably longer than running Dinic alone. Use the small and medium files for quick iteration.

### 2. Build and run the facility-optimization program

Compile:

```powershell
g++ -O2 -std=c++17 src/facility_optimizer.cpp -o facility_optimizer.exe
```

Run the small input (includes the exact Backtracking baseline):

```powershell
cmd /c ".\facility_optimizer.exe < data\facility_optimization\small.txt"
```

Run the medium input:

```powershell
cmd /c ".\facility_optimizer.exe < data\facility_optimization\medium.txt"
```

Run the large input with heuristics only:

```powershell
cmd /c ".\facility_optimizer.exe --heuristics-only < data\facility_optimization\large.txt"
```

For the medium input, Backtracking may be skipped automatically if the number of candidate DCs exceeds the implementation's exact-search limit. For the large input, use `--heuristics-only`; exhaustive Backtracking grows exponentially with the number of optional candidates and is not appropriate for large instances. The flag skips Backtracking but still runs Greedy and Local Search.

### Linux or macOS

Compile with the same `g++` commands, then use shell input redirection directly. For example:

```bash
./network_flow < data/network_flow/small.txt
./facility_optimizer < data/facility_optimization/small.txt
./facility_optimizer --heuristics-only < data/facility_optimization/large.txt
```

## How the network-flow input works

`network_flow.cpp` reads a whitespace-separated text file in this order:

1. `W D C`: number of warehouses, distribution centers, and customer zones.
2. `W` nonnegative warehouse supply values.
3. `C` nonnegative customer demand values.
4. `M`: number of directed routes.
5. `M` rows of `from_node to_node capacity`.

Real node IDs are assigned consecutively: warehouses `1..W`, DCs `W+1..W+D`, and customers `W+D+1..W+D+C`. Route capacities, supplies, and demands must be nonnegative. The program adds a super-source connected to warehouses by their supply capacities and connects each customer to a super-sink by its demand capacity.

The output includes both maximum-flow results, algorithm runtimes, and a minimum-cut validation. For a correct implementation, the maximum-flow value equals the capacity of the minimum cut found from the final residual graph.

## How the facility-optimization input works

`facility_optimizer.cpp` reads:

1. `W D C`: number of warehouses, DCs, and customer zones.
2. `budget`: maximum total opening cost for optional DCs.
3. `W` warehouse supply values.
4. `C` customer demand values.
5. `D` rows of `opening_cost handling_capacity status`.
   - `status = 0`: existing facility; it is always active and must have opening cost `0`.
   - `status = 1`: optional candidate facility; it can be opened if the budget allows.
6. `M`: number of directed routes.
7. `M` rows of `from_node to_node capacity`.

Node IDs follow the same warehouse/DC/customer convention as the network-flow input. The program splits each real node into an in-node and an out-node; the edge between them enforces the DC's handling capacity. A closed candidate DC receives zero capacity, preventing flow from passing through it. Route capacities and customer demands also constrain the achievable flow.

The objective is to maximize delivered flow while respecting the opening budget:

\[
\max_X \operatorname{MaxFlow}(G_X), \qquad \sum_{i\in X} c_i \le B.
\]

Here, `X` is the selected set of optional DCs, `c_i` is the opening cost, and `B` is the budget.

## Algorithms and interpretation

### Network flow

- **Ford–Fulkerson:** repeatedly finds an augmenting path using depth-first search.
- **Dinic:** uses BFS level graphs and DFS blocking flows; generally better suited to larger networks.
- **Minimum cut:** identifies a capacity bottleneck separating the source side from the sink side after maximum flow is reached.

### Facility selection

- **Backtracking:** enumerates feasible subsets of optional facilities and gives an exact optimum when it completes its search. Its worst-case search grows as \(O(2^K)\) subsets for `K` candidates, with a max-flow computation at each leaf.
- **Greedy:** repeatedly opens the feasible facility with the best positive marginal flow gain per opening cost. It is fast but does not guarantee the global optimum.
- **Local Search:** starts from the Greedy selection and tests local changes, including additions, removals, swaps, and selected pair additions for modest candidate sets. It can improve the result but does not guarantee the global optimum.

When Backtracking completes, its result is an exact reference for that particular input and candidate set. A heuristic matching that flow has matched the exact result on that instance; this does not create a general optimality guarantee for the heuristic. Runtime depends on the computer, compiler, input, and number of flow evaluations.

## Recommended experiment workflow

1. Run both programs with their small inputs to understand the output.
2. Run the medium inputs and record maximum flow, opening cost, runtime, and selected DCs.
3. Run the large network-flow input; expect Ford–Fulkerson to take longer.
4. Run the large facility-optimization input with `--heuristics-only`.
5. Compare Greedy and Local Search on the same input. Use a small instance where Backtracking completes to measure their gap from an exact solution.
6. Record the machine and compiler settings when reporting timings.

## Input and modeling assumptions

- All routes are directed; a reverse route must be listed separately if it exists.
- Capacities, supply, demand, and costs are nonnegative integers.
- Facility opening cost is a budget cost, not a transportation cost.
- The model maximizes flow; it does not minimize travel distance, route cost, or delivery time.
- Facility capacities are enforced by node splitting in the facility-optimization program.
- All large-scale figures are synthetic and should be described as modeled units, not real-world shipment counts.

## Troubleshooting

- **`cannot open` / input is not found:** run the command from the repository root and copy the relative path exactly.
- **`g++` is not recognized:** install a C++ toolchain and ensure `g++` is on `PATH`.
- **Large facility run is slow:** use `--heuristics-only` to skip exhaustive Backtracking.
- **Large network-flow run is slow:** Ford–Fulkerson may be the bottleneck; use small or medium input for rapid development.
- **Unexpected input errors:** do not mix files between `data/network_flow/` and `data/facility_optimization/`; their formats are different.

## Project scope

OptiRoute is an educational and experimental implementation of maximum-flow/minimum-cut algorithms and budget-constrained facility selection. Its purpose is to compare exact search and heuristics on controlled synthetic data. It is not a production logistics planner and its synthetic network should not be represented as a real company's internal network.
