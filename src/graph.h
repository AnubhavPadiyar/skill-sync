#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "types.h"

// Directed graph (adjacency list). Node = user index.
// Edge u -> v means: u can teach v something v wants to learn.
class SkillGraph {
    std::vector<std::vector<Edge>> adj;
    // hash index: skill -> indices of users who teach it
    std::unordered_map<std::string, std::vector<int>> teachers;
    std::vector<std::string> sortedSkills;  // for binary-search prefix lookup

public:
    void build(const std::vector<User>& users);
    int size() const { return (int)adj.size(); }
    const std::vector<Edge>& neighbors(int u) const { return adj[u]; }
    bool findEdge(int u, int v, std::string* skill = nullptr) const;
    // searching: users teaching an exact skill (hash lookup)
    const std::vector<int>& teachersOf(const std::string& skill) const;
    // searching: all skills starting with prefix (binary search on sorted list)
    std::vector<std::string> skillsWithPrefix(const std::string& prefix) const;
};
