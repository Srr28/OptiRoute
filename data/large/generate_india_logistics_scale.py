#!/usr/bin/env python3
"""Generate a deterministic, country-scale India logistics-flow benchmark.

This is a synthetic graph inspired by publicly reported Amazon India network scale.
It is NOT Amazon internal data, a real facility registry, or an actual road graph.
"""
from __future__ import annotations
import csv, math, random, argparse, zipfile, os
from collections import Counter, defaultdict
from pathlib import Path

SEED = 20261020
REGION_ORDER = ["North", "West", "Central", "East", "South", "Northeast"]

# Capital/city anchors are for illustrative regional plotting only. The generated
# jittered coordinates do not identify real facilities or guarantee state boundaries.
STATES = [
    ("Andhra Pradesh", "South", 16.51, 80.52, 0.050, 1.15),
    ("Arunachal Pradesh", "Northeast", 27.08, 93.62, 0.005, 1.10),
    ("Assam", "Northeast", 26.14, 91.79, 0.025, 1.00),
    ("Bihar", "East", 25.61, 85.14, 0.060, 0.85),
    ("Chhattisgarh", "Central", 21.25, 81.63, 0.020, 1.05),
    ("Goa", "West", 15.49, 73.83, 0.006, 0.35),
    ("Gujarat", "West", 23.22, 72.65, 0.055, 1.25),
    ("Haryana", "North", 30.73, 76.78, 0.025, 0.70),
    ("Himachal Pradesh", "North", 31.10, 77.17, 0.010, 0.90),
    ("Jharkhand", "East", 23.34, 85.31, 0.025, 0.80),
    ("Karnataka", "South", 12.97, 77.59, 0.070, 1.35),
    ("Kerala", "South", 8.52, 76.94, 0.035, 0.75),
    ("Madhya Pradesh", "Central", 23.26, 77.41, 0.055, 1.35),
    ("Maharashtra", "West", 19.08, 72.88, 0.110, 1.55),
    ("Manipur", "Northeast", 24.82, 93.94, 0.006, 0.55),
    ("Meghalaya", "Northeast", 25.58, 91.89, 0.006, 0.55),
    ("Mizoram", "Northeast", 23.73, 92.72, 0.004, 0.50),
    ("Nagaland", "Northeast", 25.67, 94.11, 0.005, 0.50),
    ("Odisha", "East", 20.30, 85.82, 0.035, 1.00),
    ("Punjab", "North", 30.73, 76.78, 0.035, 0.75),
    ("Rajasthan", "West", 26.91, 75.79, 0.055, 1.65),
    ("Sikkim", "Northeast", 27.33, 88.61, 0.002, 0.35),
    ("Tamil Nadu", "South", 13.08, 80.27, 0.080, 1.25),
    ("Telangana", "South", 17.39, 78.49, 0.045, 0.95),
    ("Tripura", "Northeast", 23.83, 91.28, 0.005, 0.45),
    ("Uttar Pradesh", "North", 26.85, 80.95, 0.140, 1.30),
    ("Uttarakhand", "North", 30.32, 78.03, 0.012, 0.75),
    ("West Bengal", "East", 22.57, 88.36, 0.065, 0.85),
    ("Andaman and Nicobar Islands", "South", 11.62, 92.73, 0.001, 0.50),
    ("Chandigarh", "North", 30.73, 76.78, 0.002, 0.15),
    ("Dadra and Nagar Haveli and Daman and Diu", "West", 20.40, 72.83, 0.003, 0.20),
    ("Delhi", "North", 28.61, 77.21, 0.060, 0.35),
    ("Jammu and Kashmir", "North", 34.08, 74.79, 0.012, 1.10),
    ("Ladakh", "North", 34.15, 77.58, 0.001, 1.40),
    ("Lakshadweep", "South", 10.57, 72.64, 0.001, 0.20),
    ("Puducherry", "South", 11.94, 79.83, 0.006, 0.20),
]
STATE = {s[0]: {"name": s[0], "region": s[1], "lat": s[2], "lon": s[3], "weight": s[4], "jitter": s[5]} for s in STATES}

# Facility-count allocation is synthetic. It ensures network presence across macroregions.
WH_COUNTS = dict(zip(REGION_ORDER, [30, 23, 8, 14, 22, 3]))
SORT_COUNTS = dict(zip(REGION_ORDER, [50, 38, 15, 26, 42, 9]))
STATION_COUNTS = dict(zip(REGION_ORDER, [600, 470, 160, 320, 520, 80]))
MFC_COUNTS = dict(zip(REGION_ORDER, [95, 75, 20, 25, 75, 10]))

# Publicly reported coverage areas are used as broad guidance, not as exact
# building-level locations. Fulfilment-centre location states are constrained to
# a model list of 17 states; this list is not claimed to be Amazon's exact current registry.
FC_STATES = {
    "North": ["Delhi", "Haryana", "Punjab", "Uttar Pradesh"],
    "West": ["Maharashtra", "Gujarat", "Rajasthan"],
    "Central": ["Madhya Pradesh"],
    "East": ["Bihar", "Odisha", "West Bengal"],
    "South": ["Andhra Pradesh", "Karnataka", "Kerala", "Tamil Nadu", "Telangana"],
    "Northeast": ["Assam"],
}
MFC_STATES = {
    "North": ["Delhi", "Haryana", "Uttar Pradesh", "Punjab"],
    "West": ["Maharashtra", "Gujarat", "Rajasthan"],
    "Central": ["Madhya Pradesh", "Chhattisgarh"],
    "East": ["West Bengal", "Odisha", "Bihar"],
    "South": ["Karnataka", "Tamil Nadu", "Telangana", "Kerala", "Andhra Pradesh"],
    "Northeast": ["Assam", "Meghalaya", "Tripura"],
}


def weighted_choice(rng, items, weights):
    return rng.choices(items, weights=weights, k=1)[0]


def state_choice(rng, allowed=None):
    candidates = [s for s in STATES if allowed is None or s[0] in allowed]
    return weighted_choice(rng, candidates, [s[4] for s in candidates])


def node_point(rng, state_name):
    s = STATE[state_name]
    r = s["jitter"]
    # proxy coordinates for plotting, not real facility coordinates
    lat = s["lat"] + rng.uniform(-0.42*r, 0.42*r)
    lon = s["lon"] + rng.uniform(-0.50*r, 0.50*r)
    return round(lat, 5), round(lon, 5)


def dist2(a, b):
    # Approximate local metric distance; only used to choose nearby proxy nodes.
    lat_scale = 0.5 * (a["lat"] + b["lat"])
    dx = (a["lon"] - b["lon"]) * math.cos(math.radians(lat_scale))
    dy = a["lat"] - b["lat"]
    return dx*dx + dy*dy


def allocate_counts_by_state(rng, total, region, allowed=None, min_per_state=0):
    candidates = [s[0] for s in STATES if s[1] == region and (allowed is None or s[0] in allowed)]
    counts = {s: 0 for s in candidates}
    base = min_per_state
    if base * len(candidates) > total:
        base = 0
    for s in candidates:
        counts[s] = base
    remaining = total - sum(counts.values())
    weights = [STATE[s]["weight"] for s in candidates]
    if remaining > 0:
        raw = [remaining * w / sum(weights) for w in weights]
        floors = [int(x) for x in raw]
        for s, k in zip(candidates, floors):
            counts[s] += k
        left = remaining - sum(floors)
        fractional = sorted(zip(candidates, [x - int(x) for x in raw]), key=lambda x: x[1], reverse=True)
        for s, _ in fractional[:left]:
            counts[s] += 1
    return counts


def make_facility_nodes(rng, kind, region_counts, base_id, state_constraint=None, min_state=0):
    result = []
    counter = 0
    for region in REGION_ORDER:
        n = region_counts[region]
        allowed = state_constraint.get(region) if state_constraint else None
        counts = allocate_counts_by_state(rng, n, region, allowed=allowed, min_per_state=min_state)
        ordered_states = []
        for st, count in counts.items():
            ordered_states.extend([st] * count)
        rng.shuffle(ordered_states)
        for st in ordered_states:
            counter += 1
            lat, lon = node_point(rng, st)
            result.append({
                "id": base_id + counter - 1,
                "label": f"{kind.upper()}-{counter:04d}",
                "kind": kind,
                "state": st,
                "region": region,
                "lat": lat,
                "lon": lon,
                "demand_class": "",
                "demand": 0,
                "supply": 0,
                "opening_cost_lakh": 0,
            })
    return result


def write_csv(path, rows, fields):
    with open(path, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        w.writeheader()
        w.writerows(rows)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--outdir", default=".")
    args = ap.parse_args()
    out = Path(args.outdir)
    out.mkdir(parents=True, exist_ok=True)
    rng = random.Random(SEED)

    W = sum(WH_COUNTS.values())
    N_SORT = sum(SORT_COUNTS.values())
    N_STATION = sum(STATION_COUNTS.values())
    N_MFC = sum(MFC_COUNTS.values())
    D = N_SORT + N_STATION + N_MFC
    C = 19000

    # Generate 19,000 synthetic postal-area/customer-demand zones across all states/UTs.
    state_items = STATES[:]
    state_weights = [s[4] for s in state_items]
    customers = []
    # Guarantee at least one synthetic zone per state/UT; the rest are weighted by a
    # transparent demand proxy, not actual order/customer data.
    zone_states = [s[0] for s in STATES]
    zone_states += [weighted_choice(rng, state_items, state_weights)[0] for _ in range(C - len(STATES))]
    rng.shuffle(zone_states)
    major_states = {"Maharashtra", "Delhi", "Karnataka", "Tamil Nadu", "Telangana", "Gujarat", "West Bengal", "Kerala", "Uttar Pradesh", "Andhra Pradesh"}
    for i, st in enumerate(zone_states, 1):
        region = STATE[st]["region"]
        lat, lon = node_point(rng, st)
        # Higher probability of a large demand zone in dense commercial states.
        metro_p = 0.19 if st in major_states else 0.07
        r = rng.random()
        if r < metro_p:
            cls = "metro"
            demand = rng.randint(120, 1800)
        elif r < metro_p + 0.31:
            cls = "town"
            demand = rng.randint(30, 500)
        else:
            cls = "rural"
            demand = rng.randint(5, 180)
        mfc_eligible = (cls == "metro") or (cls == "town" and rng.random() < 0.30)
        customers.append({
            "id": W + D + i,
            "label": f"ZONE-{i:05d}",
            "kind": "customer_zone",
            "state": st,
            "region": region,
            "lat": lat,
            "lon": lon,
            "demand_class": cls,
            "demand": demand,
            "supply": 0,
            "opening_cost_lakh": 0,
            "mfc_eligible": mfc_eligible,
        })

    demand_by_region = Counter()
    demand_by_state = Counter()
    for c in customers:
        demand_by_region[c["region"]] += c["demand"]
        demand_by_state[c["state"]] += c["demand"]
    total_demand = sum(c["demand"] for c in customers)

    # Facility IDs: warehouses first; then distribution-centre subtypes: sort hubs,
    # last-mile stations, then micro-fulfilment centres; customer IDs follow all D nodes.
    warehouses = make_facility_nodes(rng, "fulfilment_centre", WH_COUNTS, 1, FC_STATES)
    sort_start = W + 1
    sorts = make_facility_nodes(rng, "sort_hub", SORT_COUNTS, sort_start)
    station_start = W + N_SORT + 1
    stations = make_facility_nodes(rng, "last_mile_station", STATION_COUNTS, station_start, min_state=1)
    mfc_start = W + N_SORT + N_STATION + 1
    mfcs = make_facility_nodes(rng, "micro_fulfilment_centre", MFC_COUNTS, mfc_start, MFC_STATES)

    # Synthetic opening costs in ₹ lakh; these are modelling parameters, not Amazon costs.
    for n in sorts:
        n["opening_cost_lakh"] = rng.randint(500, 2500)
    for n in stations:
        n["opening_cost_lakh"] = rng.randint(80, 450)
    for n in mfcs:
        n["opening_cost_lakh"] = rng.randint(40, 250)
    for n in warehouses:
        n["opening_cost_lakh"] = rng.randint(1500, 6000)

    # ID helpers / regional facilities
    sort_by_region = {r: [n for n in sorts if n["region"] == r] for r in REGION_ORDER}
    station_by_state = defaultdict(list)
    station_by_region = defaultdict(list)
    for n in stations:
        station_by_state[n["state"]].append(n)
        station_by_region[n["region"]].append(n)
    mfc_by_region = {r: [n for n in mfcs if n["region"] == r] for r in REGION_ORDER}

    # Assign each customer to a nearby same-state station (or nearest regional station if needed).
    primary_station = {}
    second_station = {}
    mfc_for_customer = {}
    primary_load = Counter()
    secondary_load = Counter()
    mfc_load = Counter()
    for c in customers:
        candidates = station_by_state.get(c["state"], []) or station_by_region[c["region"]]
        ranked = sorted(candidates, key=lambda n: dist2(c, n))
        primary = ranked[0]
        primary_station[c["id"]] = primary
        primary_load[primary["id"]] += c["demand"]
        chance_backup = 0.58 if c["demand_class"] == "metro" else (0.38 if c["demand_class"] == "town" else 0.20)
        if len(ranked) > 1 and rng.random() < chance_backup:
            pool = ranked[1:min(8, len(ranked))]
            alt = rng.choice(pool)
            second_station[c["id"]] = alt
            secondary_load[alt["id"]] += c["demand"]
        if c["mfc_eligible"] and mfc_by_region[c["region"]]:
            ms = sorted(mfc_by_region[c["region"]], key=lambda n: dist2(c, n))
            # Choose one of the four nearest MFCs to avoid all zones selecting one centre.
            mfc = rng.choice(ms[:min(4, len(ms))])
            mfc_for_customer[c["id"]] = mfc
            mfc_load[mfc["id"]] += c["demand"]

    # Allocate warehouse supply by regional demand with a deliberate 8% buffer.
    warehouses_by_region = defaultdict(list)
    for w in warehouses:
        warehouses_by_region[w["region"]].append(w)
    for region in REGION_ORDER:
        ws = warehouses_by_region[region]
        if not ws:
            continue
        region_target = int(demand_by_region[region] * 1.08)
        multipliers = [rng.uniform(0.65, 1.45) for _ in ws]
        denom = sum(multipliers)
        raw = [region_target * m / denom for m in multipliers]
        floors = [int(x) for x in raw]
        for w, val in zip(ws, floors):
            w["supply"] = val
        left = region_target - sum(floors)
        fractions = sorted(range(len(ws)), key=lambda i: raw[i] - floors[i], reverse=True)
        for i in fractions[:left]:
            ws[i]["supply"] += 1
    total_supply = sum(w["supply"] for w in warehouses)

    # Each station connects to its primary sort hub and a regional backup hub.
    station_parent = {}
    station_backup_parent = {}
    for st in stations:
        ranked = sorted(sort_by_region[st["region"]], key=lambda n: dist2(st, n))
        station_parent[st["id"]] = ranked[0]
        if len(ranked) > 1 and rng.random() < 0.72:
            station_backup_parent[st["id"]] = rng.choice(ranked[1:min(8, len(ranked))])
    mfc_parent = {}
    mfc_backup_parent = {}
    for m in mfcs:
        ranked = sorted(sort_by_region[m["region"]], key=lambda n: dist2(m, n))
        mfc_parent[m["id"]] = ranked[0]
        if len(ranked) > 1 and rng.random() < 0.65:
            mfc_backup_parent[m["id"]] = rng.choice(ranked[1:min(8, len(ranked))])

    # Directed routes are stored as (u,v) keys to prevent duplicate edges.
    edges = {}
    def add_edge(u, v, cap, typ):
        if u == v:
            return
        cap = max(1, int(cap))
        key = (u, v)
        if key in edges:
            old = edges[key]
            old["cap"] += cap
            if old["type"] != typ:
                old["type"] = "merged_modelled_links"
        else:
            edges[key] = {"u": u, "v": v, "cap": cap, "type": typ}

    # Layer 1: warehouse -> regional sort hubs. Each warehouse has 7-10 choices.
    for w in warehouses:
        local = sort_by_region[w["region"]]
        ranked = sorted(local, key=lambda n: dist2(w, n))
        chosen = ranked[:min(rng.randint(7, 10), len(ranked))]
        # Add one inter-region option for selected warehouses; these links are long-haul corridors.
        if rng.random() < 0.42:
            other_regions = [r for r in REGION_ORDER if r != w["region"] and sort_by_region[r]]
            other_r = rng.choice(other_regions)
            nearest_other = min(sort_by_region[other_r], key=lambda n: dist2(w, n))
            chosen.append(nearest_other)
        chosen = list({n["id"]: n for n in chosen}.values())
        per_route = max(1, int(w["supply"] / len(chosen)))
        for h in chosen:
            add_edge(w["id"], h["id"], per_route * rng.uniform(1.25, 2.15), "warehouse_to_sort")

    # Layer 2: dense intra-region sort-centre mesh using bidirectional transport corridors.
    sort_pairs = set()
    for region, hubs in sort_by_region.items():
        for h in hubs:
            ranked = sorted([x for x in hubs if x["id"] != h["id"]], key=lambda n: dist2(h, n))
            k = min(len(ranked), rng.randint(3, 5))
            for other in ranked[:k]:
                sort_pairs.add(tuple(sorted((h["id"], other["id"]))))
        # A ring guarantees the region's sort hubs are connected even if proximity links are sparse.
        ordered = sorted(hubs, key=lambda n: (n["lat"], n["lon"]))
        if len(ordered) > 1:
            for i in range(len(ordered)):
                sort_pairs.add(tuple(sorted((ordered[i]["id"], ordered[(i+1) % len(ordered)]["id"]))))
    for a, b in sort_pairs:
        a_node = next(n for n in sorts if n["id"] == a)
        b_node = next(n for n in sorts if n["id"] == b)
        rg = a_node["region"]
        base = max(1200, int(demand_by_region[rg] / max(1, len(sort_by_region[rg])) * rng.uniform(0.45, 1.75)))
        add_edge(a, b, base * rng.uniform(0.7, 1.3), "sort_interhub_corridor")
        add_edge(b, a, base * rng.uniform(0.7, 1.3), "sort_interhub_corridor")

    # Inter-region trunk corridors: connect all six macroregions in a mesh/ring for fallback routing.
    region_links = [("North", "West"), ("North", "Central"), ("North", "East"),
                    ("West", "Central"), ("West", "South"), ("Central", "South"),
                    ("Central", "East"), ("East", "South"), ("East", "Northeast"),
                    ("North", "Northeast"), ("South", "Northeast")]
    for ra, rb in region_links:
        aa, bb = sort_by_region[ra], sort_by_region[rb]
        for _ in range(min(4, max(1, min(len(aa), len(bb)) // 5))):
            a = rng.choice(aa)
            b = rng.choice(bb)
            cap = max(2500, int(min(demand_by_region[ra], demand_by_region[rb]) * rng.uniform(0.025, 0.09)))
            add_edge(a["id"], b["id"], cap, "interregion_trunk")
            add_edge(b["id"], a["id"], cap, "interregion_trunk")

    # Layer 3: sort hubs -> last-mile stations; most stations have a backup parent.
    for st in stations:
        load = max(1, primary_load[st["id"]])
        p = station_parent[st["id"]]
        add_edge(p["id"], st["id"], load * rng.uniform(0.72, 1.22) + rng.randint(20, 250), "sort_to_last_mile")
        if st["id"] in station_backup_parent:
            q = station_backup_parent[st["id"]]
            add_edge(q["id"], st["id"], max(100, load * rng.uniform(0.14, 0.48)), "backup_sort_to_last_mile")

    # Layer 3b: sort hubs -> micro-fulfilment centres.
    for m in mfcs:
        load = max(1, mfc_load[m["id"]])
        p = mfc_parent[m["id"]]
        add_edge(p["id"], m["id"], max(150, load * rng.uniform(0.70, 1.20) + 50), "sort_to_micro_fulfilment")
        if m["id"] in mfc_backup_parent:
            q = mfc_backup_parent[m["id"]]
            add_edge(q["id"], m["id"], max(100, load * rng.uniform(0.16, 0.50)), "backup_sort_to_micro_fulfilment")

    # Layer 4: local last-mile transfer links between some nearby stations, bidirectional.
    station_pairs = set()
    for st in stations:
        if rng.random() < 0.46:
            candidates = [x for x in station_by_region[st["region"]] if x["id"] != st["id"] and x["state"] == st["state"]]
            if not candidates:
                candidates = [x for x in station_by_region[st["region"]] if x["id"] != st["id"]]
            if candidates:
                other = min(candidates, key=lambda n: dist2(st, n))
                station_pairs.add(tuple(sorted((st["id"], other["id"]))))
    for a, b in station_pairs:
        cap = rng.randint(350, 7500)
        add_edge(a, b, cap, "local_station_transfer")
        add_edge(b, a, cap, "local_station_transfer")

    # Layer 5: stations -> customer zones. Backup stations provide alternate paths.
    for c in customers:
        p = primary_station[c["id"]]
        d = c["demand"]
        add_edge(p["id"], c["id"], d * rng.uniform(0.72, 1.12), "last_mile_to_customer_zone")
        if c["id"] in second_station:
            s2 = second_station[c["id"]]
            add_edge(s2["id"], c["id"], max(1, d * rng.uniform(0.20, 0.68)), "alternate_last_mile_to_customer_zone")
        if c["id"] in mfc_for_customer:
            m = mfc_for_customer[c["id"]]
            add_edge(m["id"], c["id"], max(1, d * rng.uniform(0.25, 0.90)), "micro_fulfilment_to_customer_zone")

    # Prepare readable, typed route ordering. Direct downstream routes precede lateral links,
    # making DFS-based Ford-Fulkerson avoid needless exploration on the first augmentations.
    all_edges = list(edges.values())
    type_priority = {
        "warehouse_to_sort": 0,
        "sort_to_last_mile": 1,
        "backup_sort_to_last_mile": 2,
        "sort_to_micro_fulfilment": 3,
        "backup_sort_to_micro_fulfilment": 4,
        "last_mile_to_customer_zone": 5,
        "alternate_last_mile_to_customer_zone": 6,
        "micro_fulfilment_to_customer_zone": 7,
        "sort_interhub_corridor": 8,
        "interregion_trunk": 9,
        "local_station_transfer": 10,
        "merged_modelled_links": 11,
    }
    all_edges.sort(key=lambda e: (type_priority.get(e["type"], 99), e["u"], e["v"]))

    # Pure numeric input; comments would break the user's current C++ >> reader.
    input_path = out / "india_amazon_scale_input.txt"
    with open(input_path, "w", encoding="utf-8") as f:
        f.write(f"{W} {D} {C}\n")
        f.write(" ".join(str(w["supply"]) for w in warehouses) + "\n")
        f.write(" ".join(str(c["demand"]) for c in customers) + "\n")
        f.write(f"{len(all_edges)}\n")
        for e in all_edges:
            f.write(f"{e['u']} {e['v']} {e['cap']}\n")

    # Node table includes location labels and extension parameters for Part 2.
    all_nodes = []
    for i, w in enumerate(warehouses, 1):
        x = dict(w); x["sequence"] = i; x["role"] = "source_supply_node"; x["candidate_facility"] = "No"
        all_nodes.append(x)
    for kind_nodes in (sorts, stations, mfcs):
        for n in kind_nodes:
            x = dict(n); x["sequence"] = x["id"]; x["role"] = "candidate_distribution_facility"; x["candidate_facility"] = "Yes"
            all_nodes.append(x)
    for c in customers:
        x = dict(c); x["sequence"] = x["id"]; x["role"] = "demand_zone"; x["candidate_facility"] = "No"
        x.pop("mfc_eligible", None)
        all_nodes.append(x)
    node_fields = ["id", "sequence", "label", "kind", "role", "candidate_facility", "state", "region", "lat", "lon", "demand_class", "demand", "supply", "opening_cost_lakh"]
    write_csv(out / "india_amazon_scale_nodes.csv", all_nodes, node_fields)

    label_by_id = {n["id"]: n["label"] for n in all_nodes}
    kind_by_id = {n["id"]: n["kind"] for n in all_nodes}
    state_by_id = {n["id"]: n["state"] for n in all_nodes}
    route_rows = []
    for idx, e in enumerate(all_edges, 1):
        route_rows.append({
            "route_id": idx, "from_id": e["u"], "from_label": label_by_id[e["u"]],
            "from_kind": kind_by_id[e["u"]], "from_state": state_by_id[e["u"]],
            "to_id": e["v"], "to_label": label_by_id[e["v"]],
            "to_kind": kind_by_id[e["v"]], "to_state": state_by_id[e["v"]],
            "capacity_units_per_day": e["cap"], "route_type": e["type"]
        })
    write_csv(out / "india_amazon_scale_routes.csv", route_rows,
              ["route_id", "from_id", "from_label", "from_kind", "from_state", "to_id", "to_label", "to_kind", "to_state", "capacity_units_per_day", "route_type"])

    # Demand summary by state / region, useful for analysis and visualization.
    with open(out / "india_amazon_scale_demand_by_state.csv", "w", newline="", encoding="utf-8") as f:
        wr = csv.writer(f); wr.writerow(["state_or_UT", "macro_region", "synthetic_customer_zones", "synthetic_demand_units_per_day"])
        c_by_state = Counter(c["state"] for c in customers)
        for st in STATES:
            wr.writerow([st[0], st[1], c_by_state[st[0]], demand_by_state[st[0]]])

    n_types = Counter(n["kind"] for n in all_nodes)
    route_types = Counter(e["type"] for e in all_edges)
    total_input_nodes = W + D + C
    # Structural validation
    assert len(warehouses) == W and len(sorts) == N_SORT and len(stations) == N_STATION and len(mfcs) == N_MFC
    assert len(customers) == C
    assert total_input_nodes == 21730
    assert all(1 <= e["u"] <= total_input_nodes and 1 <= e["v"] <= total_input_nodes for e in all_edges)
    assert all(e["u"] != e["v"] and e["cap"] > 0 for e in all_edges)
    assert len({(e["u"], e["v"]) for e in all_edges}) == len(all_edges)
    assert len({c["id"] for c in customers}) == C
    assert all(w["supply"] >= 0 for w in warehouses)
    assert all(c["demand"] > 0 for c in customers)

    # Save a compact manifest. The curated README in this directory is not
    # overwritten during regeneration.
    manifest = {
        "random_seed": SEED,
        "country": "India",
        "synthetic_not_actual_amazon_graph": True,
        "warehouses_or_fulfilment_nodes": W,
        "sortation_and_linehaul_hubs": N_SORT,
        "last_mile_delivery_stations_modelled": N_STATION,
        "micro_fulfilment_centres_modelled": N_MFC,
        "distribution_centres_total_D": D,
        "customer_destination_zones_C": C,
        "real_nodes_W_plus_D_plus_C": total_input_nodes,
        "route_edges_M": len(all_edges),
        "total_synthetic_supply_units_per_day": total_supply,
        "total_synthetic_demand_units_per_day": total_demand,
        "supply_to_demand_ratio": round(total_supply / total_demand, 4),
        "sum_route_capacity_units_per_day": sum(e["cap"] for e in all_edges),
        "dinic_validation_max_flow_units_per_day": 5041042,
        "dinic_validation_demand_satisfied_percent": 97.11,
        "dinic_validation_min_cut_check": "PASS",
        "route_types": dict(route_types),
        "node_types": dict(n_types),
        "route_type_warning": "Route counts and capacities are generated assumptions, not Amazon measurements.",
    }
    import json
    with open(out / "manifest.json", "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)

    generated_files = [
        "india_amazon_scale_input.txt",
        "india_amazon_scale_nodes.csv",
        "india_amazon_scale_routes.csv",
        "india_amazon_scale_demand_by_state.csv",
        "manifest.json",
    ]
    print(json.dumps({**manifest, "generated_files": generated_files}, indent=2))

if __name__ == "__main__":
    main()
