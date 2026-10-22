#include <bits/stdc++.h>
using namespace std;
using ll = long long;

struct Route {
    int u, v;
    ll cap;
};

class FordFulkerson {
    struct Edge { int to, rev; ll cap; };
    vector<vector<Edge>> g;

    ll dfs(int v, int t, ll pushed, vector<char>& vis) {
        if (pushed == 0) return 0;
        if (v == t) return pushed;
        vis[v] = true;

        for (auto& e : g[v]) {
            if (vis[e.to] || e.cap <= 0) continue;
            ll tr = dfs(e.to, t, min(pushed, e.cap), vis);
            if (tr == 0) continue;
            e.cap -= tr;
            g[e.to][e.rev].cap += tr;
            return tr;
        }
        return 0;
    }

public:
    explicit FordFulkerson(int n) : g(n) {}

    void addEdge(int u, int v, ll cap) {
        Edge a{v, (int)g[v].size(), cap};
        Edge b{u, (int)g[u].size(), 0};
        g[u].push_back(a);
        g[v].push_back(b);
    }

    ll maxFlow(int s, int t) {
        ll flow = 0;
        while (true) {
            vector<char> vis(g.size(), false);
            ll pushed = dfs(s, t, LLONG_MAX, vis);
            if (pushed == 0) break;
            flow += pushed;
        }
        return flow;
    }
};

class Dinic {
    struct Edge { int to, rev; ll cap; };
    vector<vector<Edge>> g;
    vector<int> level, ptr;

    bool bfs(int s, int t) {
        fill(level.begin(), level.end(), -1);
        queue<int> q;
        level[s] = 0;
        q.push(s);
        while (!q.empty()) {
            int v = q.front(); q.pop();
            for (const auto& e : g[v]) {
                if (e.cap > 0 && level[e.to] == -1) {
                    level[e.to] = level[v] + 1;
                    q.push(e.to);
                }
            }
        }
        return level[t] != -1;
    }

    ll dfs(int v, int t, ll pushed) {
        if (pushed == 0) return 0;
        if (v == t) return pushed;
        for (int& cid = ptr[v]; cid < (int)g[v].size(); ++cid) {
            Edge& e = g[v][cid];
            if (e.cap <= 0 || level[e.to] != level[v] + 1) continue;
            ll tr = dfs(e.to, t, min(pushed, e.cap));
            if (tr == 0) continue;
            e.cap -= tr;
            g[e.to][e.rev].cap += tr;
            return tr;
        }
        return 0;
    }

public:
    explicit Dinic(int n) : g(n), level(n), ptr(n) {}

    void addEdge(int u, int v, ll cap) {
        Edge a{v, (int)g[v].size(), cap};
        Edge b{u, (int)g[u].size(), 0};
        g[u].push_back(a);
        g[v].push_back(b);
    }

    ll maxFlow(int s, int t) {
        ll flow = 0;
        while (bfs(s, t)) {
            fill(ptr.begin(), ptr.end(), 0);
            while (ll pushed = dfs(s, t, LLONG_MAX)) {
                flow += pushed;
            }
        }
        return flow;
    }

    vector<char> residualReachable(int s) const {
        vector<char> vis(g.size(), false);
        queue<int> q;
        vis[s] = true;
        q.push(s);
        while (!q.empty()) {
            int v = q.front(); q.pop();
            for (const auto& e : g[v]) {
                if (e.cap > 0 && !vis[e.to]) {
                    vis[e.to] = true;
                    q.push(e.to);
                }
            }
        }
        return vis;
    }
};

string nodeName(int id, int W, int D, int S, int T) {
    if (id == S) return "S (super-source)";
    if (id == T) return "T (super-sink)";
    if (id <= W) return "W" + to_string(id);
    if (id <= W + D) return "D" + to_string(id - W);
    return "C" + to_string(id - W - D);
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int W, D, C;
    if (!(cin >> W >> D >> C)) return 0;
    if (W < 0 || D < 0 || C < 0 || (W + D + C) == 0) {
        cerr << "Invalid node counts.\n";
        return 1;
    }

    const int N = W + D + C;
    const int S = N + 1;
    const int T = N + 2;
    const int totalNodes = N + 3;

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

    int M;
    if (!(cin >> M) || M < 0) {
        cerr << "Invalid number of routes.\n";
        return 1;
    }
    vector<Route> routes(M);
    for (int i = 0; i < M; ++i) {
        if (!(cin >> routes[i].u >> routes[i].v >> routes[i].cap)) {
            cerr << "Invalid route input.\n";
            return 1;
        }
        if (routes[i].u < 1 || routes[i].u > N || routes[i].v < 1 || routes[i].v > N || routes[i].cap < 0) {
            cerr << "Route endpoints must be valid real-node IDs and capacity must be nonnegative.\n";
            return 1;
        }
    }

    FordFulkerson ff(totalNodes);
    Dinic dinic(totalNodes);
    auto addBoth = [&](int u, int v, ll cap) {
        ff.addEdge(u, v, cap);
        dinic.addEdge(u, v, cap);
    };

    // Super-source -> warehouses: capacities equal warehouse supplies.
    for (int i = 1; i <= W; ++i) addBoth(S, i, supply[i]);

    // Real directed transportation routes.
    for (const auto& r : routes) addBoth(r.u, r.v, r.cap);

    // Customers -> super-sink: capacities equal customer demands.
    for (int i = 1; i <= C; ++i) {
        int customerId = W + D + i;
        addBoth(customerId, T, demand[i]);
    }

    auto ffStart = chrono::steady_clock::now();
    ll ffFlow = ff.maxFlow(S, T);
    auto ffEnd = chrono::steady_clock::now();

    auto dinicStart = chrono::steady_clock::now();
    ll dinicFlow = dinic.maxFlow(S, T);
    auto dinicEnd = chrono::steady_clock::now();

    double ffMs = chrono::duration<double, milli>(ffEnd - ffStart).count();
    double dinicMs = chrono::duration<double, milli>(dinicEnd - dinicStart).count();

    cout << "=== PART 1: MAXIMUM FLOW ===\n";
    cout << "Ford-Fulkerson maximum flow: " << ffFlow << '\n';
    cout << "Dinic maximum flow:         " << dinicFlow << '\n';
    cout << fixed << setprecision(3);
    cout << "Ford-Fulkerson runtime: " << ffMs << " ms\n";
    cout << "Dinic runtime:          " << dinicMs << " ms\n";
    if (ffFlow == dinicFlow) cout << "Check: both algorithms agree.\n";
    else cout << "WARNING: results differ; check the implementation/input.\n";

    // Minimum cut from Dinic's residual graph.
    vector<char> reach = dinic.residualReachable(S);
    ll cutCapacity = 0;
    cout << "\n=== MINIMUM CUT (from Dinic residual graph) ===\n";
    bool printedAny = false;

    for (int i = 1; i <= W; ++i) {
        if (reach[S] && !reach[i]) {
            cout << nodeName(S, W, D, S, T) << " -> "
                 << nodeName(i, W, D, S, T)
                 << " (supply capacity " << supply[i] << ")\n";
            cutCapacity += supply[i];
            printedAny = true;
        }
    }
    for (const auto& r : routes) {
        if (reach[r.u] && !reach[r.v]) {
            cout << nodeName(r.u, W, D, S, T) << " -> "
                 << nodeName(r.v, W, D, S, T)
                 << " (route capacity " << r.cap << ")\n";
            cutCapacity += r.cap;
            printedAny = true;
        }
    }
    for (int i = 1; i <= C; ++i) {
        int customerId = W + D + i;
        if (reach[customerId] && !reach[T]) {
            cout << nodeName(customerId, W, D, S, T) << " -> "
                 << nodeName(T, W, D, S, T)
                 << " (demand capacity " << demand[i] << ")\n";
            cutCapacity += demand[i];
            printedAny = true;
        }
    }
    if (!printedAny) cout << "No cut edges found (check the input graph).\n";
    cout << "Minimum-cut capacity: " << cutCapacity << '\n';
    cout << "Max-flow/min-cut check: "
         << (cutCapacity == dinicFlow ? "PASS" : "FAIL") << '\n';

    cout << "\nNode IDs: W1..WW are warehouses, D1..DD are distribution centers, "
         << "C1..CC are customers.\n";
    return 0;
}
