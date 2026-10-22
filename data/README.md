# Datasets

- `examples/small_network.txt`: tiny deterministic graph for quick correctness checks.
- `examples/medium_network.txt`: reproducible synthetic graph with 10 warehouses, 20 distribution centers, 100 customer zones, and 374 directed routes.
- `large/india_amazon_scale_input.txt`: country-scale synthetic graph with 21,730 real nodes and 37,436 routes. Companion CSV files explain the generated nodes and routes; `manifest.json` records key counts and results.

All `.txt` files use the input format documented in the root README. The medium and large datasets are synthetic. The large dataset is not Amazon's internal network and its model capacities are not real measured shipment volumes. See `large/README.md` for further details.
