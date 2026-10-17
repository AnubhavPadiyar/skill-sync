#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include "graph.h"
#include "linked_list.h"
#include "matcher.h"
#include "storage.h"

static const std::string USERS_FILE = "data/users.txt";
static const std::string REQUESTS_FILE = "data/requests.txt";
static const std::string NOTIF_FILE = "data/notifications.txt";

static std::vector<User> users;
static SkillGraph graph;
static std::vector<SwapMatch> lastMatches;  // matches shown last, for "request swap"

static void rebuild() { graph.build(users); }

static int indexOfId(int id) {
    for (size_t i = 0; i < users.size(); ++i) if (users[i].id == id) return (int)i;
    return -1;
}

static std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string s;
    std::getline(std::cin, s);
    return s;
}

static int readUser(const std::string& prompt) {
    try {
        int idx = indexOfId(std::stoi(readLine(prompt)));
        if (idx < 0) std::cout << "No such user.\n";
        return idx;
    } catch (...) { std::cout << "Invalid id.\n"; return -1; }
}

static std::string describe(const SwapMatch& m) {
    std::string r;
    int n = (int)m.users.size();
    for (int i = 0; i < n; ++i)
        r += users[m.users[i]].name + " -[" + m.skills[i] + "]-> ";
    return r + users[m.users[0]].name;
}

static void showMatches(const std::vector<SwapMatch>& ms) {
    lastMatches = ms;
    if (ms.empty()) { std::cout << "No matches found.\n"; return; }
    for (size_t i = 0; i < ms.size(); ++i)
        std::cout << "  [" << i + 1 << "] (" << ms[i].users.size() << " people) " << describe(ms[i]) << "\n";
}

static std::vector<std::string> splitCSV(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s + ",") {
        if (c == ',') { cur = storage::normalize(cur); if (!cur.empty()) out.push_back(cur); cur.clear(); }
        else cur += c;
    }
    return out;
}

static void listUsers() {
    for (const auto& u : users) {
        std::cout << "  #" << u.id << " " << u.name << " | teaches: ";
        for (const auto& s : u.teach) std::cout << s << " ";
        std::cout << "| wants: ";
        for (const auto& s : u.learn) std::cout << s << " ";
        std::cout << "\n";
    }
}

static void addUser() {
    User u;
    u.id = users.empty() ? 1 : users.back().id + 1;
    u.name = readLine("Name: ");
    u.teach = splitCSV(readLine("Skills you can teach (comma separated): "));
    u.learn = splitCSV(readLine("Skills you want to learn (comma separated): "));
    users.push_back(u);
    storage::saveUsers(USERS_FILE, users);
    rebuild();
    std::cout << "Added user #" << u.id << "\n";
}

static void searchSkill() {
    std::string q = storage::normalize(readLine("Skill (or prefix): "));
    auto skills = graph.skillsWithPrefix(q);
    if (skills.empty()) { std::cout << "No skills match.\n"; return; }
    for (const auto& s : skills) {
        std::cout << "  " << s << ": ";
        for (int i : graph.teachersOf(s)) std::cout << users[i].name << " ";
        std::cout << "\n";
    }
}

static void requestSwap() {
    if (lastMatches.empty()) { std::cout << "Run a match search first.\n"; return; }
    try {
        size_t k = std::stoul(readLine("Match number to request: "));
        if (k < 1 || k > lastMatches.size()) { std::cout << "Out of range.\n"; return; }
        const SwapMatch& m = lastMatches[k - 1];
        int rid = storage::appendRequest(REQUESTS_FILE, m, users);
        for (int idx : m.users)
            storage::appendNotification(NOTIF_FILE, users[idx].id,
                                        "Swap request #" + std::to_string(rid) + ": " + describe(m));
        std::cout << "Request #" << rid << " saved, " << m.users.size() << " people notified.\n";
    } catch (...) { std::cout << "Invalid number.\n"; }
}

static void showNotifications() {
    int idx = readUser("User id: ");
    if (idx < 0) return;
    LinkedList<std::string> list;
    storage::loadNotifications(NOTIF_FILE, users[idx].id, list);
    if (list.empty()) { std::cout << "No notifications.\n"; return; }
    int i = 1;
    list.forEach([&](const std::string& s) { std::cout << "  " << i++ << ". " << s << "\n"; });
}

int main() {
    users = storage::loadUsers(USERS_FILE);
    rebuild();
    std::cout << "Skill Sync (phase 2) - " << users.size() << " users loaded\n";
    while (true) {
        std::cout << "\n1 List users   2 Add user   3 Search skill\n"
                     "4 Direct swaps 5 Multi swaps 6 Shortest chain (BFS)\n"
                     "7 Request swap 8 Notifications 9 Graph has cycle?  0 Exit\n";
        std::string c = readLine("> ");
        if (!std::cin) break;
        if (c == "0") break;
        else if (c == "1") listUsers();
        else if (c == "2") addUser();
        else if (c == "3") searchSkill();
        else if (c == "4") { int i = readUser("User id: "); if (i >= 0) showMatches(matcher::findDirect(graph, i)); }
        else if (c == "5") { int i = readUser("User id: "); if (i >= 0) showMatches(matcher::findMulti(graph, i)); }
        else if (c == "6") {
            int i = readUser("User id: ");
            if (i >= 0) {
                SwapMatch m = matcher::shortestCycle(graph, i);
                std::vector<SwapMatch> v;
                if (!m.users.empty()) v.push_back(m);
                showMatches(v);
            }
        }
        else if (c == "7") requestSwap();
        else if (c == "8") showNotifications();
        else if (c == "9") std::cout << (matcher::hasCycle(graph) ? "Yes, exchange cycles exist.\n" : "No cycles.\n");
        else std::cout << "Unknown option.\n";
    }
    return 0;
}
