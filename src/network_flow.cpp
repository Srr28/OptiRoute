#include <bits/stdc++.h>
using namespace std;
using ll = long long;

struct Route {
    int u, v;
    ll capacity;
};

class FordFulkerson {
    struct Edge { int to, rev; ll cap; };
    vector<vector<Edge>> g;

    ll dfs(int v, int t, ll pushed, vector<char>& vis) {
        if (pushed == 0) return 0;
        if (v == t) return pushed;
        vis[v] = true;

        for (auto& e : g[v]) {
            if (vis[e.to] || e.cap == 0) continue;
            ll sent = dfs(e.to, t, min(pushed, e.cap), vis);
            if (sent == 0) continue;
            e.cap -= sent;
            g[e.to][e.rev].cap += sent;
            return sent;
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
            ll sent = dfs(s, t, LLONG_MAX, vis);
            if (sent == 0) break;
            flow += sent;
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
            int v = q.front();
            q.pop();
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

        for (int& i = ptr[v]; i < (int)g[v].size(); ++i) {
            Edge& e = g[v][i];
            if (e.cap == 0 || level[e.to] != level[v] + 1) continue;
            ll sent = dfs(e.to, t, min(pushed, e.cap));
            if (sent == 0) continue;
            e.cap -= sent;
            g[e.to][e.rev].cap += sent;
            return sent;
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
            while (ll sent = dfs(s, t, LLONG_MAX)) flow += sent;
        }
        return flow;
    }

    vector<char> residualReachable(int s) const {
        vector<char> vis(g.size(), false);
        queue<int> q;
        vis[s] = true;
        q.push(s);
        while (!q.empty()) {
            int v = q.front();
            q.pop();
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

int inNode(int node) { return 2 * node; }
int outNode(int node) { return 2 * node + 1; }

string nodeName(int id, int W, int D) {
    if (id <= W) return "W" + to_string(id);
    if (id <= W + D) return "D" + to_string(id - W);
    return "C" + to_string(id - W - D);
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int W, D, C;
    if (!(cin >> W >> D >> C)) return 0;
    if (W < 0 || D < 0 || C < 0 || W + D + C == 0) {
        cerr << "Invalid node counts.\n";
        return 1;
    }

    int N = W + D + C;
    vector<ll> supply(W + 1), demand(C + 1), dcCapacity(D + 1);

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
    for (int i = 1; i <= D; ++i) {
        if (!(cin >> dcCapacity[i]) || dcCapacity[i] < 0) {
            cerr << "Invalid DC handling capacity.\n";
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
        if (!(cin >> routes[i].u >> routes[i].v >> routes[i].capacity)) {
            cerr << "Invalid route row. Expected: from_node to_node capacity.\n";
            return 1;
        }
        if (routes[i].u < 1 || routes[i].u > N || routes[i].v < 1 ||
            routes[i].v > N || routes[i].capacity < 0) {
            cerr << "Invalid route endpoint or negative route capacity.\n";
            return 1;
        }
    }

    // Every real node is split into in/out nodes. Only DCs have a finite
    // in->out capacity; this is the DC's total handling capacity.
    const ll INF = LLONG_MAX / 4;
    int source = 2 * N + 2;
    int sink = 2 * N + 3;
    int graphNodes = 2 * N + 4;
    FordFulkerson ff(graphNodes);
    Dinic dinic(graphNodes);

    auto addBoth = [&](int u, int v, ll cap) {
        ff.addEdge(u, v, cap);
        dinic.addEdge(u, v, cap);
    };

    for (int node = 1; node <= N; ++node) {
        ll cap = INF;
        if (node > W && node <= W + D) cap = dcCapacity[node - W];
        addBoth(inNode(node), outNode(node), cap);
    }
    for (int i = 1; i <= W; ++i) addBoth(source, inNode(i), supply[i]);
    for (const auto& r : routes) {
        addBoth(outNode(r.u), inNode(r.v), r.capacity);
    }
    for (int i = 1; i <= C; ++i) {
        int customer = W + D + i;
        addBoth(outNode(customer), sink, demand[i]);
    }

    auto startFF = chrono::steady_clock::now();
    ll ffFlow = ff.maxFlow(source, sink);
    auto endFF = chrono::steady_clock::now();
    auto startDinic = chrono::steady_clock::now();
    ll dinicFlow = dinic.maxFlow(source, sink);
    auto endDinic = chrono::steady_clock::now();

    double ffMs = chrono::duration<double, milli>(endFF - startFF).count();
    double dinicMs = chrono::duration<double, milli>(endDinic - startDinic).count();

    cout << "=== OPTIROUTE: MAX-FLOW ANALYSIS ===\n";
    cout << "Warehouses: " << W << ", DCs: " << D << ", customers: " << C << '\n';
    cout << "Directed routes: " << M << '\n';
    cout << "Ford-Fulkerson maximum flow: " << ffFlow << '\n';
    cout << "Dinic maximum flow:          " << dinicFlow << '\n';
    cout << fixed << setprecision(3);
    cout << "Ford-Fulkerson runtime: " << ffMs << " ms\n";
    cout << "Dinic runtime:          " << dinicMs << " ms\n";
    cout << "Algorithm agreement: " << (ffFlow == dinicFlow ? "PASS" : "FAIL") << '\n';

    // A cut can cross supply edges, route edges, DC handling edges, or demand edges.
    vector<char> reach = dinic.residualReachable(source);
    ll cutCapacity = 0;
    cout << "\n=== MINIMUM CUT ===\n";
    for (int i = 1; i <= W; ++i) {
        if (reach[source] && !reach[inNode(i)]) {
            cout << "Source -> W" << i << " (supply " << supply[i] << ")\n";
            cutCapacity += supply[i];
        }
    }
    for (const auto& r : routes) {
        if (reach[outNode(r.u)] && !reach[inNode(r.v)]) {
            cout << nodeName(r.u, W, D) << " -> " << nodeName(r.v, W, D)
                 << " (route " << r.capacity << ")\n";
            cutCapacity += r.capacity;
        }
    }
    for (int i = 1; i <= D; ++i) {
        int node = W + i;
        if (reach[inNode(node)] && !reach[outNode(node)]) {
            cout << "D" << i << " handling capacity " << dcCapacity[i] << '\n';
            cutCapacity += dcCapacity[i];
        }
    }
    for (int i = 1; i <= C; ++i) {
        int customer = W + D + i;
        if (reach[outNode(customer)] && !reach[sink]) {
            cout << "C" << i << " -> Sink (demand " << demand[i] << ")\n";
            cutCapacity += demand[i];
        }
    }
    cout << "Minimum-cut capacity: " << cutCapacity << '\n';
    cout << "Max-flow/min-cut check: " << (cutCapacity == dinicFlow ? "PASS" : "FAIL") << '\n';
    cout << "\nNode IDs: warehouses 1..W, DCs W+1..W+D, customers W+D+1..W+D+C.\n";
    return ffFlow == dinicFlow && cutCapacity == dinicFlow ? 0 : 2;
}
