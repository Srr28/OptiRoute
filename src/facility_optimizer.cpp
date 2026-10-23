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

    // Greedy heuristic:
    // Repeatedly open the affordable DC with the largest marginal flow gain per cost.
    // This is a heuristic, not a proven approximation algorithm for this problem.
    Solution greedy() {
        int K = candidateCount();
        vector<char> selected(K, false);
        ll cost = 0;
        ll currentFlow = evaluateFlow(selected);

        while (true) {
            int bestIndex = -1;
            ll bestFlow = currentFlow;
            long double bestRatio = -1.0L;

            for (int i = 0; i < K; ++i) {
                if (selected[i]) continue;
                ll dcCost = facilities[candidates[i]].openingCost;
                if (cost + dcCost > budget) continue;

                selected[i] = true;
                ll newFlow = evaluateFlow(selected);
                selected[i] = false;

                ll gain = newFlow - currentFlow;
                if (gain <= 0) continue;

                long double ratio;
                if (dcCost == 0) ratio = numeric_limits<long double>::infinity();
                else ratio = (long double)gain / (long double)dcCost;

                if (bestIndex == -1 || ratio > bestRatio ||
                    (ratio == bestRatio && newFlow > bestFlow)) {
                    bestIndex = i;
                    bestRatio = ratio;
                    bestFlow = newFlow;
                }
            }

            if (bestIndex == -1) break;
            selected[bestIndex] = true;
            cost += facilities[candidates[bestIndex]].openingCost;
            currentFlow = bestFlow;
        }

        return makeSolution(selected, currentFlow);
    }

    // Local Search starts from the Greedy solution. On each iteration it checks:
    // 1) opening one more DC, 2) closing one selected DC, and 3) swapping two DCs.
    // It accepts the best strict improvement in flow (or lower cost at equal flow).
    Solution localSearch() {
        Solution current = greedy();
        int K = candidateCount();

        while (true) {
            Solution best = current;

            // Try adding one closed candidate.
            for (int i = 0; i < K; ++i) {
                if (current.open[i]) continue;
                vector<char> next = current.open;
                next[i] = true;
                ll nextCost = selectedCost(next);
                if (nextCost > budget) continue;
                Solution candidate = makeSolution(next, evaluateFlow(next));
                if (better(candidate, best)) best = move(candidate);
            }

            // Try opening two closed candidates together. This helps when neither
            // facility improves the flow alone, but the pair does improve it.
            // Keep this neighborhood limited to moderate candidate counts.
            if (K <= 40) {
                for (int i = 0; i < K; ++i) {
                    if (current.open[i]) continue;
                    for (int j = i + 1; j < K; ++j) {
                        if (current.open[j]) continue;
                        vector<char> next = current.open;
                        next[i] = true;
                        next[j] = true;
                        if (selectedCost(next) > budget) continue;
                        Solution candidate = makeSolution(next, evaluateFlow(next));
                        if (better(candidate, best)) best = move(candidate);
                    }
                }
            }

            // Try replacing one selected DC with two currently closed DCs.
            // This can escape a greedy choice that blocks a better pair.
            if (K <= 40) {
                for (int removeIndex = 0; removeIndex < K; ++removeIndex) {
                    if (!current.open[removeIndex]) continue;
                    for (int i = 0; i < K; ++i) {
                        if (current.open[i] || i == removeIndex) continue;
                        for (int j = i + 1; j < K; ++j) {
                            if (current.open[j] || j == removeIndex) continue;
                            vector<char> next = current.open;
                            next[removeIndex] = false;
                            next[i] = true;
                            next[j] = true;
                            if (selectedCost(next) > budget) continue;
                            Solution candidate = makeSolution(next, evaluateFlow(next));
                            if (better(candidate, best)) best = move(candidate);
                        }
                    }
                }
            }

            // Try removing one selected candidate.
            for (int i = 0; i < K; ++i) {
                if (!current.open[i]) continue;
                vector<char> next = current.open;
                next[i] = false;
                Solution candidate = makeSolution(next, evaluateFlow(next));
                if (better(candidate, best)) best = move(candidate);
            }

            // Try swapping one selected candidate for one unselected candidate.
            for (int i = 0; i < K; ++i) {
                if (!current.open[i]) continue;
                for (int j = 0; j < K; ++j) {
                    if (current.open[j]) continue;
                    vector<char> next = current.open;
                    next[i] = false;
                    next[j] = true;
                    if (selectedCost(next) > budget) continue;
                    Solution candidate = makeSolution(next, evaluateFlow(next));
                    if (better(candidate, best)) best = move(candidate);
                }
            }

            if (!better(best, current)) break;
            current = move(best);
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

    cout << "=== OPTIROUTE: FACILITY SELECTION ===\n";
    cout << "Warehouses: " << W << ", DCs: " << D << ", customers: " << C << '\n';
    cout << "Candidate DCs: " << candidateCount << ", budget: " << budget << '\n';

    const auto exactStart = chrono::steady_clock::now();
    Solution exact = optimizer.backtracking();
    const auto exactEnd = chrono::steady_clock::now();
    double exactMs = chrono::duration<double, milli>(exactEnd - exactStart).count();

    if (exact.valid) {
        optimizer.printSolution("BACKTRACKING (EXACT)", exact);
        cout << fixed << setprecision(3) << "Runtime: " << exactMs << " ms\n";
    } else {
        cout << "\n=== BACKTRACKING (EXACT) ===\n";
        cout << "Skipped because the candidate count exceeds the exact-search limit.\n";
    }

    long long beforeGreedy = optimizer.evaluations();
    const auto greedyStart = chrono::steady_clock::now();
    Solution greedy = optimizer.greedy();
    const auto greedyEnd = chrono::steady_clock::now();
    double greedyMs = chrono::duration<double, milli>(greedyEnd - greedyStart).count();
    optimizer.printSolution("GREEDY HEURISTIC", greedy);
    cout << fixed << setprecision(3) << "Runtime: " << greedyMs << " ms\n";
    cout << "Flow evaluations: " << optimizer.evaluations() - beforeGreedy << '\n';

    long long beforeLocal = optimizer.evaluations();
    const auto localStart = chrono::steady_clock::now();
    Solution local = optimizer.localSearch();
    const auto localEnd = chrono::steady_clock::now();
    double localMs = chrono::duration<double, milli>(localEnd - localStart).count();
    optimizer.printSolution("LOCAL SEARCH (STARTS FROM GREEDY)", local);
    cout << fixed << setprecision(3) << "Runtime: " << localMs << " ms\n";
    cout << "Flow evaluations: " << optimizer.evaluations() - beforeLocal << '\n';

    cout << "\nTotal max-flow evaluations: " << optimizer.evaluations() << '\n';
    cout << "Note: every evaluation rebuilds the graph and runs Dinic.\n";
    return 0;
}
