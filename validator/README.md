# Large-network validator

This folder contains a standalone C++ validator for the country-scale OptiRoute dataset. It runs Dinic independently of the main executable, then checks that the maximum-flow value equals the minimum-cut capacity.

## Compile

From the repository root: 

```powershell
g++ -O2 -std=c++17 validator/dinic_validator.cpp -o optiroute_validate_large.exe
```

## Run on the large dataset

```powershell
cmd /c ".\optiroute_validate_large.exe < data\large\india_amazon_scale_input.txt"
```

## Expected invariants

The validator should report these dataset counts and correctness result:

- `W=100 D=2630 C=19000 M=37436`
- `Maximum flow (Dinic): 5041042 units/day`
- `Minimum-cut capacity: 5041042 units/day`
- `Max-flow/min-cut check: PASS`

Runtime varies with hardware, compiler version, optimization flags, and system load. The validator rejects malformed input, negative capacities or demands, invalid node IDs, and unexpected trailing tokens. The flow units are synthetic modeled units per day, not measured Amazon shipment volumes.
