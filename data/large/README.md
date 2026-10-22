# India-scale logistics benchmark (synthetic)

## Dataset size

- Warehouse/fulfilment source nodes: **100**
- Sort/line-haul hubs: **180**
- Last-mile delivery stations: **2,150**
- Micro-fulfilment centres: **300**
- Distribution/transfer nodes used by the C++ input (**D**): **2,630**
- Synthetic customer destination zones (**C**): **19,000**
- Real graph nodes (**W + D + C**): **21,730**
- Directed transport routes (**M**): **37,436**
- Synthetic supply: **5,606,465 model units/day**
- Synthetic demand: **5,191,173 model units/day**
- Fixed random seed: **20261020**

## Data provenance and limitations

This is a **synthetic country-scale logistics simulation**, not Amazon's actual internal network. Publicly available company material can inform the broad scale of a logistics operation, but does not provide a complete current list of facilities, customer destinations, route connections, and route capacities.

The exact facility counts, destination-zone counts, route counts, capacities, supply, demand, and proxy coordinates in this dataset are generated model choices. `ZONE-xxxxx` records are synthetic destination catchments, not individual customers, addresses, or verified postal PIN codes. Coordinates are jittered around state/city anchors for approximate plotting only; they are not real facility coordinates and do not guarantee geographic-boundary accuracy.

Every capacity and demand value is a generated **load unit per day**, not measured package throughput. The `opening_cost_lakh` column in the node metadata is also synthetic; it is included as an input for possible future facility-selection experiments, not as a real construction cost. The current Part 1 solver does not enforce a facility's node-throughput capacity. To model that constraint, split each facility into an in-node and out-node joined by a capacity-limited edge.

## Input format

`india_amazon_scale_input.txt` is pure numeric input compatible with the C++ programs:

1. `W D C`
2. `W` warehouse supplies
3. `C` customer-zone demands
4. `M` directed-route count
5. `M` rows of `from_id to_id capacity`

The program adds a super-source connected to each warehouse by its supply capacity and connects each customer to the super-sink by its demand capacity. IDs are 1-based. Warehouse IDs are `1..W`, distribution-centre IDs are `W+1..W+D`, and customer IDs follow. Within the distribution-centre block, sort hubs come first, then last-mile stations, then micro-fulfilment centres.

## Included files

- `india_amazon_scale_input.txt` — ready for the C++ executables
- `india_amazon_scale_nodes.csv` — node names/types, state or macroregion, proxy coordinates, modeled supply/demand and assumed opening costs
- `india_amazon_scale_routes.csv` — directed edges with endpoints, route type and modeled capacity
- `india_amazon_scale_demand_by_state.csv` — modeled destination-zone and demand totals by state/UT
- `manifest.json` — random seed, node/edge counts, and summary statistics
- `generate_india_logistics_scale.py` — deterministic dataset generator

## Regenerate the dataset

From the repository root, run the deterministic generator with its output pointed to this folder:

```powershell
python data/large/generate_india_logistics_scale.py --outdir data/large
```

The generator rewrites the numeric input, node/route CSVs, demand summary, and `manifest.json`. It does not overwrite this curated README. The data are generated from a fixed seed, so the counts and modeled values should be reproducible.

## Validate the dataset

From the repository root, compile the Dinic-only validator:

```powershell
g++ -O2 -std=c++17 validator/dinic_validator.cpp -o optiroute_validate_large.exe
cmd /c ".\optiroute_validate_large.exe < data\large\india_amazon_scale_input.txt"
```

Expected invariant: maximum flow is **5,041,042** model units/day, minimum-cut capacity is **5,041,042**, and the max-flow/min-cut check is `PASS`. See [`../../validator/expected_large_output.txt`](../../validator/expected_large_output.txt) for a machine-independent result summary. Runtime varies by machine and compiler.

The combined main executable runs DFS-based Ford–Fulkerson before Dinic and may take a long time on this graph. Use the Dinic-only validator for the large network, and compare both algorithms on small/medium inputs. These generated data do not represent Amazon's actual facilities, routes, demand, or throughput.
