#include "matcher.h"
#include <algorithm>
#include "linked_list.h"

namespace matcher {

static void fillSkills(const SkillGraph& g, SwapMatch& m) {
    int n = (int)m.users.size();
    m.skills.clear();
    for (int i = 0; i < n; ++i) {
        std::string s;
        g.findEdge(m.users[i], m.users[(i + 1) % n], &s);
        m.skills.push_back(s);
    }
}

std::vector<SwapMatch> findDirect(const SkillGraph& g, int src) {
    std::vector<SwapMatch> out;
    for (const auto& e : g.neighbors(src)) {
        if (g.findEdge(e.to, src)) {
            SwapMatch m;
            m.users = {src, e.to};
            fillSkills(g, m);
            out.push_back(m);
        }
    }
    return out;
}

static void dfs(const SkillGraph& g, int src, int cur, int maxLen,
                std::vector<int>& path, std::vector<char>& vis,
                std::vector<SwapMatch>& out) {
    for (const auto& e : g.neighbors(cur)) {
        if (e.to == src) {
            if (path.size() >= 3) {  // length 2 is a direct swap
                SwapMatch m;
                m.users = path;
                fillSkills(g, m);
                out.push_back(m);
            }
            continue;
        }
        if (vis[e.to] || (int)path.size() >= maxLen) continue;
        vis[e.to] = 1;
        path.push_back(e.to);
        dfs(g, src, e.to, maxLen, path, vis, out);
        path.pop_back();
        vis[e.to] = 0;
    }
}

std::vector<SwapMatch> findMulti(const SkillGraph& g, int src, int maxLen) {
    std::vector<SwapMatch> out;
    std::vector<char> vis(g.size(), 0);
    std::vector<int> path = {src};
    vis[src] = 1;
    dfs(g, src, src, maxLen, path, vis, out);
    std::sort(out.begin(), out.end(), [](const SwapMatch& a, const SwapMatch& b) {
        return a.users.size() < b.users.size();
    });
    return out;
}

SwapMatch shortestCycle(const SkillGraph& g, int src) {
    int n = g.size();
    std::vector<int> parent(n, -1), dist(n, -1);
    Queue<int> q;
    q.push(src);
    dist[src] = 0;
    int u;
    while (q.pop(u)) {
        for (const auto& e : g.neighbors(u)) {
            if (e.to == src && u != src) {
                SwapMatch m;
                for (int x = u; x != -1; x = parent[x]) m.users.push_back(x);
                std::reverse(m.users.begin(), m.users.end());
                fillSkills(g, m);
                return m;
            }
            if (dist[e.to] == -1) {
                dist[e.to] = dist[u] + 1;
                parent[e.to] = u;
                q.push(e.to);
            }
        }
    }
    return {};
}

static bool dfsCycle(const SkillGraph& g, int u, std::vector<int>& color) {
    color[u] = 1;  // in recursion stack
    for (const auto& e : g.neighbors(u)) {
        if (color[e.to] == 1) return true;
        if (color[e.to] == 0 && dfsCycle(g, e.to, color)) return true;
    }
    color[u] = 2;
    return false;
}

bool hasCycle(const SkillGraph& g) {
    std::vector<int> color(g.size(), 0);
    for (int i = 0; i < g.size(); ++i)
        if (color[i] == 0 && dfsCycle(g, i, color)) return true;
    return false;
}

}  // namespace matcher
