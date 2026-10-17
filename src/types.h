#pragma once
#include <string>
#include <vector>

struct User {
    int id;
    std::string name;
    std::vector<std::string> teach;
    std::vector<std::string> learn;
};

// Directed edge: "from" teaches `skill` to "to"
struct Edge {
    int to;
    std::string skill;
};

// users[i] teaches skills[i] to users[(i+1) % n]
// n == 2 -> direct swap, n >= 3 -> multi-person swap
struct SwapMatch {
    std::vector<int> users;  // indices into the users vector
    std::vector<std::string> skills;
};
