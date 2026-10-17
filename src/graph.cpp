#include "graph.h"
#include <algorithm>
#include <unordered_set>

void SkillGraph::build(const std::vector<User>& users) {
    int n = (int)users.size();
    adj.assign(n, {});
    teachers.clear();
    sortedSkills.clear();

    for (int i = 0; i < n; ++i)
        for (const auto& s : users[i].teach) teachers[s].push_back(i);

    for (const auto& kv : teachers) sortedSkills.push_back(kv.first);
    std::sort(sortedSkills.begin(), sortedSkills.end());

    std::unordered_set<long long> seen;  // one edge per (u,v) pair
    for (int v = 0; v < n; ++v) {
        for (const auto& s : users[v].learn) {
            auto it = teachers.find(s);
            if (it == teachers.end()) continue;
            for (int u : it->second) {
                if (u == v) continue;
                long long key = (long long)u * n + v;
                if (!seen.insert(key).second) continue;
                adj[u].push_back({v, s});
            }
        }
    }
}

bool SkillGraph::findEdge(int u, int v, std::string* skill) const {
    for (const auto& e : adj[u])
        if (e.to == v) {
            if (skill) *skill = e.skill;
            return true;
        }
    return false;
}

const std::vector<int>& SkillGraph::teachersOf(const std::string& skill) const {
    static const std::vector<int> none;
    auto it = teachers.find(skill);
    return it == teachers.end() ? none : it->second;
}

std::vector<std::string> SkillGraph::skillsWithPrefix(const std::string& prefix) const {
    std::vector<std::string> out;
    auto it = std::lower_bound(sortedSkills.begin(), sortedSkills.end(), prefix);
    for (; it != sortedSkills.end() && it->compare(0, prefix.size(), prefix) == 0; ++it)
        out.push_back(*it);
    return out;
}
