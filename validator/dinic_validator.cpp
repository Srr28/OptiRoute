#include <bits/stdc++.h>
using namespace std;
using ll = long long;
struct Route { int u, v; ll cap; };
struct Dinic {
    struct Edge { int to, rev; ll cap; };
    vector<vector<Edge>> g;
    vector<int> level, ptr;
    explicit Dinic(int n): g(n), level(n), ptr(n) {}
    void addEdge(int u, int v, ll cap) {
        Edge a{v, (int)g[v].size(), cap};
        Edge b{u, (int)g[u].size(), 0};
        g[u].push_back(a); g[v].push_back(b);
    }
    bool bfs(int s, int t) {
        fill(level.begin(), level.end(), -1);
        queue<int> q; level[s] = 0; q.push(s);
        while (!q.empty()) {
            int v = q.front(); q.pop();
            for (const auto &e : g[v]) if (e.cap > 0 && level[e.to] == -1) {
                level[e.to] = level[v] + 1; q.push(e.to);
            }
        }
        return level[t] != -1;
    }
    ll dfs(int v, int t, ll pushed) {
        if (pushed == 0) return 0;
        if (v == t) return pushed;
        for (int &cid = ptr[v]; cid < (int)g[v].size(); ++cid) {
            Edge &e = g[v][cid];
            if (e.cap <= 0 || level[e.to] != level[v] + 1) continue;
            ll tr = dfs(e.to, t, min(pushed, e.cap));
            if (!tr) continue;
            e.cap -= tr; g[e.to][e.rev].cap += tr; return tr;
        }
        return 0;
    }
    ll maxFlow(int s, int t) {
        ll flow = 0;
        while (bfs(s, t)) {
            fill(ptr.begin(), ptr.end(), 0);
            while (ll pushed = dfs(s, t, LLONG_MAX)) flow += pushed;
        }
        return flow;
    }
    vector<char> reachable(int s) const {
        vector<char> seen(g.size(), false); queue<int> q;
        seen[s] = true; q.push(s);
        while (!q.empty()) {
            int v = q.front(); q.pop();
            for (const auto &e : g[v]) if (e.cap > 0 && !seen[e.to]) {
                seen[e.to] = true; q.push(e.to);
            }
        }
        return seen;
    }
};
int main() {
    ios::sync_with_stdio(false); cin.tie(nullptr);
    int W, D, C;
    if (!(cin >> W >> D >> C)) { cerr << "Bad header\n"; return 1; }
    if (W < 0 || D < 0 || C < 0) { cerr << "Negative count\n"; return 1; }
    int N = W + D + C, S = N + 1, T = N + 2;
    vector<ll> supply(W+1), demand(C+1);
    for (int i=1; i<=W; ++i) if (!(cin >> supply[i]) || supply[i] < 0) { cerr << "Bad supply\n"; return 1; }
    for (int i=1; i<=C; ++i) if (!(cin >> demand[i]) || demand[i] < 0) { cerr << "Bad demand\n"; return 1; }
    int M; if (!(cin >> M) || M < 0) { cerr << "Bad M\n"; return 1; }
    vector<Route> routes(M);
    for (auto &r : routes) {
        if (!(cin >> r.u >> r.v >> r.cap)) { cerr << "Bad route row\n"; return 1; }
        if (r.u < 1 || r.u > N || r.v < 1 || r.v > N || r.cap < 0) { cerr << "Bad edge endpoint/capacity\n"; return 1; }
    }
    string trailing; if (cin >> trailing) { cerr << "Unexpected token after final route: " << trailing << "\n"; return 1; }
    Dinic flow(N+3);
    for (int i=1; i<=W; ++i) flow.addEdge(S, i, supply[i]);
    for (const auto &r : routes) flow.addEdge(r.u, r.v, r.cap);
    for (int i=1; i<=C; ++i) flow.addEdge(W+D+i, T, demand[i]);
    auto start = chrono::steady_clock::now();
    ll value = flow.maxFlow(S,T);
    auto end = chrono::steady_clock::now();
    auto reach = flow.reachable(S);
    ll cut = 0;
    for (int i=1; i<=W; ++i) if (reach[S] && !reach[i]) cut += supply[i];
    for (const auto &r : routes) if (reach[r.u] && !reach[r.v]) cut += r.cap;
    for (int i=1; i<=C; ++i) if (reach[W+D+i] && !reach[T]) cut += demand[i];
    ll totalSupply = accumulate(supply.begin()+1, supply.end(), 0LL);
    ll totalDemand = accumulate(demand.begin()+1, demand.end(), 0LL);
    cout << "W=" << W << " D=" << D << " C=" << C << " M=" << M << "\n";
    cout << "Total vertices excluding S/T: " << N << "\n";
    cout << "Total supply: " << totalSupply << " units/day\n";
    cout << "Total demand: " << totalDemand << " units/day\n";
    cout << "Maximum flow (Dinic): " << value << " units/day\n";
    cout << "Demand satisfied: " << fixed << setprecision(2) << (totalDemand ? 100.0 * (double)value / (double)totalDemand : 0.0) << "%\n";
    cout << "Minimum-cut capacity: " << cut << " units/day\n";
    cout << "Max-flow/min-cut check: " << (cut == value ? "PASS" : "FAIL") << "\n";
    cout << "Dinic runtime: " << setprecision(3) << chrono::duration<double,milli>(end-start).count() << " ms\n";
    return cut == value ? 0 : 2;
}
