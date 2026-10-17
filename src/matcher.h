#pragma once
#include <vector>
#include "graph.h"
#include "types.h"

namespace matcher {
// A <-> B
std::vector<SwapMatch> findDirect(const SkillGraph& g, int src);
// A -> B -> C -> ... -> A (DFS + backtracking, length 3..maxLen), sorted by length
std::vector<SwapMatch> findMulti(const SkillGraph& g, int src, int maxLen = 5);
// shortest exchange cycle through src (BFS); empty users => none
SwapMatch shortestCycle(const SkillGraph& g, int src);
// does the whole graph contain any cycle? (DFS colouring)
bool hasCycle(const SkillGraph& g);
}
