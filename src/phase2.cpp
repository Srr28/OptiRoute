#include <bits/stdc++.h>
using namespace std;

using ll = long long;

struct Route {
    int u, v;
    ll capacity;
};

struct Facility {
    ll openingCost = 0;
    ll handlingCapacity = 0;
    bool existing = false;
    int nodeId = -1;
};

struct Solution {
    vector<char> open;
    ll cost = 0;
    ll flow = 0;
    bool valid = false;
};

class Dinic {
    struct Edge { int to, rev; ll cap; };
    vector<vector<Edge>> graph;
    vector<int> level, ptr;

    bool bfs(int source, int sink) {
        fill(level.begin(), level.end(), -1);
        queue<int> q;
        level[source] = 0;
        q.push(source);

        while (!q.empty()) {
            int v = q.front();
            q.pop();
            for (const auto& e : graph[v]) {
                if (e.cap > 0 && level[e.to] == -1) {
                    level[e.to] = level[v] + 1;
                    q.push(e.to);
                }
            }
        }
        return level[sink] != -1;
    }

    ll dfs(int v, int sink, ll pushed) {
        if (pushed == 0) return 0;
        if (v == sink) return pushed;

        for (int& i = ptr[v]; i < (int)graph[v].size(); ++i) {
            Edge& e = graph[v][i];
            if (e.cap <= 0 || level[e.to] != level[v] + 1) continue;

            ll sent = dfs(e.to, sink, min(pushed, e.cap));
            if (sent == 0) continue;

            e.cap -= sent;
            graph[e.to][e.rev].cap += sent;
            return sent;
        }
        return 0;
    }

public:
    explicit Dinic(int n) : graph(n), level(n), ptr(n) {}

    void addEdge(int u, int v, ll capacity) {
        Edge forward{v, (int)graph[v].size(), capacity};
        Edge reverse{u, (int)graph[u].size(), 0};
        graph[u].push_back(forward);
        graph[v].push_back(reverse);
    }

    ll maxFlow(int source, int sink) {
        ll flow = 0;
        while (bfs(source, sink)) {
            fill(ptr.begin(), ptr.end(), 0);
            while (ll pushed = dfs(source, sink, LLONG_MAX)) {
                flow += pushed;
            }
        }
        return flow;
    }
};

class FacilityOptimizer {
    int W, D, C, N, source, sink, graphNodes;
    ll budget;
    vector<ll> supply, demand;
    vector<Facility> facilities;       // Indexed by DC index 0..D-1.
    vector<Route> routes;
    vector<int> candidates;             // DC indices that are optional.
    vector<int> candidatePosition;      // DC index -> position in candidates, or -1.
    long long flowEvaluations = 0;

    int inNode(int originalNode) const { return 2 * originalNode; }
    int outNode(int originalNode) const { return 2 * originalNode + 1; }

    ll evaluateFlow(const vector<char>& selected) {
        ++flowEvaluations;
        Dinic dinic(graphNodes);

        // Split every real node into in/out. For a DC, the in->out edge
        // enforces its handling capacity. An unopened candidate has capacity 0.
        const ll INF = LLONG_MAX / 4;

        for (int node = 1; node <= N; ++node) {
            ll nodeCapacity = INF;

            if (node > W && node <= W + D) {
                int dcIndex = node - W - 1;
                const Facility& f = facilities[dcIndex];
                if (f.existing) {
                    nodeCapacity = f.handlingCapacity;
                } else {
                    int pos = candidatePosition[dcIndex];
                    nodeCapacity = (pos >= 0 && selected[pos])
                        ? f.handlingCapacity : 0;
                }
            }

            dinic.addEdge(inNode(node), outNode(node), nodeCapacity);
        }

        // Source-to-warehouse supply constraints.
        for (int i = 1; i <= W; ++i) {
            dinic.addEdge(source, inNode(i), supply[i]);
        }

        // Directed transportation routes. Incoming routes enter the in-node;
        // outgoing routes leave the out-node, so closed DCs cannot pass flow.
        for (const auto& r : routes) {
            dinic.addEdge(outNode(r.u), inNode(r.v), r.capacity);
        }

        // Customer demand constraints.
        for (int i = 1; i <= C; ++i) {
            int customerNode = W + D + i;
            dinic.addEdge(outNode(customerNode), sink, demand[i]);
        }

        return dinic.maxFlow(source, sink);
    }

    ll selectedCost(const vector<char>& selected) const {
        ll total = 0;
        for (int i = 0; i < (int)candidates.size(); ++i) {
            if (selected[i]) total += facilities[candidates[i]].openingCost;
        }
        return total;
    }

    Solution makeSolution(const vector<char>& selected, ll flow) const {
        return Solution{selected, selectedCost(selected), flow, true};
    }

    bool better(const Solution& a, const Solution& b) const {
        if (!a.valid) return false;
        if (!b.valid) return true;
        if (a.flow != b.flow) return a.flow > b.flow;
        if (a.cost != b.cost) return a.cost < b.cost;
        return a.open < b.open;
    }

public:
    FacilityOptimizer(int w, int d, int c, ll b,
                      vector<ll> s, vector<ll> dem,
                      vector<Facility> fs, vector<Route> rs)
        : W(w), D(d), C(c), N(w + d + c), source(2 * (w + d + c) + 2),
          sink(2 * (w + d + c) + 3), graphNodes(2 * (w + d + c) + 4),
          budget(b), supply(move(s)), demand(move(dem)),
          facilities(move(fs)), routes(move(rs)) {
        candidatePosition.assign(D, -1);
        for (int i = 0; i < D; ++i) {
            facilities[i].nodeId = W + i + 1;
            if (!facilities[i].existing) {
                candidatePosition[i] = (int)candidates.size();
                candidates.push_back(i);
            }
        }
    }

    int candidateCount() const { return (int)candidates.size(); }
    long long evaluations() const { return flowEvaluations; }

    Solution backtracking(int exactLimit = 18) {
        const int K = candidateCount();
        if (K > exactLimit) {
            cerr << "Backtracking skipped: " << K
                 << " candidates exceed the exact-search limit of "
                 << exactLimit << ". Use a smaller candidate set for the exact baseline.\n";
            return {};
        }

        Solution best;
        vector<char> selected(K, false);

        // Empty selection is always feasible. Existing facilities remain active.
        best = makeSolution(selected, evaluateFlow(selected));

        function<void(int, ll)> dfs = [&](int index, ll cost) {
            if (cost > budget) return; // Safe pruning because all costs are nonnegative.

            if (index == K) {
                ll flow = evaluateFlow(selected);
                Solution current = makeSolution(selected, flow);
                if (better(current, best)) best = move(current);
                return;
            }

            // Branch 1: do not open this candidate.
            selected[index] = false;
            dfs(index + 1, cost);

            // Branch 2: open this candidate if it stays within budget.
            ll nextCost = cost + facilities[candidates[index]].openingCost;
            if (nextCost <= budget) {
                selected[index] = true;
                dfs(index + 1, nextCost);
                selected[index] = false;
            }
        };

        dfs(0, 0);
        return best;
    }

    Solution greedy() {
        int K = candidateCount();
        vector<char> selected(K, false);
        ll cost = 0;
        ll currentFlow = evaluateFlow(selected);

        while (true) {
            int bestPosition = -1;
            ll bestFlow = currentFlow;
            long double bestRatio = -1.0L;

            for (int i = 0; i < K; ++i) {
                if (selected[i]) continue;
                ll candidateCost = facilities[candidates[i]].openingCost;
                if (candidateCost > budget - cost) continue;

                selected[i] = true;
                ll newFlow = evaluateFlow(selected);
                selected[i] = false;

                ll gain = newFlow - currentFlow;
                if (gain <= 0) continue;

                long double ratio = candidateCost == 0
                    ? numeric_limits<long double>::infinity()
                    : (long double)gain / (long double)candidateCost;

                if (bestPosition == -1 || ratio > bestRatio ||
                    (ratio == bestRatio && newFlow > bestFlow)) {
                    bestPosition = i;
                    bestRatio = ratio;
                    bestFlow = newFlow;
                }
            }

            if (bestPosition == -1) break;
            selected[bestPosition] = true;
            cost += facilities[candidates[bestPosition]].openingCost;
            currentFlow = bestFlow;
        }

        return makeSolution(selected, currentFlow);
    }

    Solution localSearch(const Solution& initial, int maxIterations = 50) {
        Solution current = initial;
        if (!current.valid) {
            vector<char> empty(candidateCount(), false);
            current = makeSolution(empty, evaluateFlow(empty));
        }

        const int K = candidateCount();
        for (int iteration = 0; iteration < maxIterations; ++iteration) {
            Solution bestNeighbor = current;

            // Neighborhood 1: add one unopened candidate or remove one open candidate.
            for (int i = 0; i < K; ++i) {
                vector<char> trial = current.open;
                trial[i] = !trial[i];
                ll cost = selectedCost(trial);
                if (cost > budget) continue;

                Solution neighbor = makeSolution(trial, evaluateFlow(trial));
                if (better(neighbor, bestNeighbor)) bestNeighbor = move(neighbor);
            }

            // Neighborhood 2: swap one open candidate for one closed candidate.
            // This can improve the solution when the budget is already full.
            for (int i = 0; i < K; ++i) {
                if (!current.open[i]) continue;
                for (int j = 0; j < K; ++j) {
                    if (current.open[j]) continue;
                    vector<char> trial = current.open;
                    trial[i] = false;
                    trial[j] = true;
                    ll cost = selectedCost(trial);
                    if (cost > budget) continue;

                    Solution neighbor = makeSolution(trial, evaluateFlow(trial));
                    if (better(neighbor, bestNeighbor)) bestNeighbor = move(neighbor);
                }
            }

            // Pair-add and pair-swap neighborhoods help detect complementarity:
            // two facilities can improve flow even when neither improves it alone.
            // Keep these O(K^2) neighborhoods limited to modest candidate sets.
            if (K <= 40) {
                for (int i = 0; i < K; ++i) {
                    if (current.open[i]) continue;
                    for (int j = i + 1; j < K; ++j) {
                        if (current.open[j]) continue;

                        // Add both candidates without removing any selected facility.
                        ll pairCost = facilities[candidates[i]].openingCost
                                    + facilities[candidates[j]].openingCost;
                        if (pairCost <= budget - current.cost) {
                            vector<char> trial = current.open;
                            trial[i] = trial[j] = true;
                            Solution neighbor = makeSolution(trial, evaluateFlow(trial));
                            if (better(neighbor, bestNeighbor)) bestNeighbor = move(neighbor);
                        }

                        // Swap one selected facility for two unopened facilities.
                        // This lets local search escape a greedy choice that uses budget.
                        for (int remove = 0; remove < K; ++remove) {
                            if (!current.open[remove]) continue;
                            vector<char> trial = current.open;
                            trial[remove] = false;
                            trial[i] = trial[j] = true;
                            if (selectedCost(trial) > budget) continue;
                            Solution neighbor = makeSolution(trial, evaluateFlow(trial));
                            if (better(neighbor, bestNeighbor)) bestNeighbor = move(neighbor);
                        }
                    }
                }
            }

            if (!better(bestNeighbor, current)) break;
            current = move(bestNeighbor);
        }

        return current;
    }

    void printSolution(const string& label, const Solution& solution) const {
        cout << "\n=== " << label << " ===\n";
        if (!solution.valid) {
            cout << "No solution produced.\n";
            return;
        }

        cout << "Maximum flow: " << solution.flow << '\n';
        cout << "Opening cost: " << solution.cost << " / budget " << budget << '\n';
        cout << "Selected candidate DCs:";
        bool any = false;
        for (int i = 0; i < (int)solution.open.size(); ++i) {
            if (solution.open[i]) {
                cout << ' ' << (candidates[i] + 1);
                any = true;
            }
        }
        if (!any) cout << " none";
        cout << '\n';
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int W, D, C;
    ll budget;

    // Phase 2 input format:
    // W D C
    // budget
    // W supply values
    // C demand values
    // D rows: opening_cost handling_capacity status
    //      status 0 = existing (always active), 1 = candidate (optional)
    // M
    // M rows: from_node to_node capacity
    if (!(cin >> W >> D >> C)) return 0;
    if (W < 0 || D < 0 || C < 0 || W + D + C == 0) {
        cerr << "Invalid node counts.\n";
        return 1;
    }
    if (!(cin >> budget) || budget < 0) {
        cerr << "Budget must be a nonnegative integer.\n";
        return 1;
    }

    const int N = W + D + C;
    vector<ll> supply(W + 1), demand(C + 1);

    for (int i = 1; i <= W; ++i) {
        if (!(cin >> supply[i]) || supply[i] < 0) {
            cerr << "Invalid warehouse supply.\n";
            return 1;
        }
    }
    for (int i = 1; i <= C; ++i) {
        if (!(cin >> demand[i]) || demand[i] < 0) {
            cerr << "Invalid customer demand.\n";
            return 1;
        }
    }

    vector<Facility> facilities(D);
    int candidateCount = 0;
    for (int i = 0; i < D; ++i) {
        int status;
        if (!(cin >> facilities[i].openingCost
                  >> facilities[i].handlingCapacity >> status)) {
            cerr << "Invalid facility row. Expected: opening_cost handling_capacity status.\n";
            return 1;
        }
        if (facilities[i].openingCost < 0 || facilities[i].handlingCapacity < 0 ||
            (status != 0 && status != 1)) {
            cerr << "Facility costs/capacities must be nonnegative; status must be 0 or 1.\n";
            return 1;
        }
        facilities[i].existing = (status == 0);
        if (facilities[i].existing && facilities[i].openingCost != 0) {
            cerr << "Existing facilities must have opening_cost 0.\n";
            return 1;
        }
        if (!facilities[i].existing) ++candidateCount;
    }

    int M;
    if (!(cin >> M) || M < 0) {
        cerr << "Invalid number of routes.\n";
        return 1;
    }

    vector<Route> routes(M);
    for (int i = 0; i < M; ++i) {
        if (!(cin >> routes[i].u >> routes[i].v >> routes[i].capacity)) {
            cerr << "Invalid route row.\n";
            return 1;
        }
        if (routes[i].u < 1 || routes[i].u > N ||
            routes[i].v < 1 || routes[i].v > N ||
            routes[i].capacity < 0) {
            cerr << "Route endpoints must be in 1..W+D+C and capacity nonnegative.\n";
            return 1;
        }
    }

    FacilityOptimizer optimizer(W, D, C, budget, move(supply), move(demand),
                                move(facilities), move(routes));

    cout << "=== OPTIROUTE PHASE 2: BUDGET-CONSTRAINED FACILITY SELECTION ===\n";
    cout << "Warehouses: " << W << ", DCs: " << D << ", customers: " << C << '\n';
    cout << "Candidate DCs: " << candidateCount << ", budget: " << budget << '\n';

    auto start = chrono::steady_clock::now();
    Solution exact = optimizer.backtracking();
    auto end = chrono::steady_clock::now();
    double backtrackingMs = chrono::duration<double, milli>(end - start).count();
    if (exact.valid) optimizer.printSolution("BACKTRACKING (EXACT ON SMALL INSTANCES)", exact);
    cout << fixed << setprecision(3);
    if (exact.valid) cout << "Runtime: " << backtrackingMs << " ms\n";

    start = chrono::steady_clock::now();
    Solution greedy = optimizer.greedy();
    end = chrono::steady_clock::now();
    double greedyMs = chrono::duration<double, milli>(end - start).count();
    optimizer.printSolution("GREEDY", greedy);
    cout << fixed << setprecision(3) << "Runtime: " << greedyMs << " ms\n";

    start = chrono::steady_clock::now();
    Solution local = optimizer.localSearch(greedy);
    end = chrono::steady_clock::now();
    double localMs = chrono::duration<double, milli>(end - start).count();
    optimizer.printSolution("LOCAL SEARCH (STARTED FROM GREEDY)", local);
    cout << fixed << setprecision(3) << "Runtime: " << localMs << " ms\n";

    if (exact.valid && exact.flow > 0) {
        cout << "\nGreedy flow gap vs exact: "
             << fixed << setprecision(2)
             << (100.0 * (exact.flow - greedy.flow) / exact.flow) << "%\n";
        cout << "Local-search flow gap vs exact: "
             << fixed << setprecision(2)
             << (100.0 * (exact.flow - local.flow) / exact.flow) << "%\n";
    } else if (exact.valid) {
        cout << "\nExact flow is zero; percentage optimality gaps are undefined.\n";
    } else {
        cout << "\nExact comparison unavailable because the candidate count exceeds the backtracking limit.\n";
    }

    cout << "Total max-flow evaluations: " << optimizer.evaluations() << '\n';
    cout << "Note: each evaluation rebuilds the residual graph and runs Dinic.\n";
    return 0;
}
