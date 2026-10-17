#include "storage.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

namespace storage {

std::string normalize(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a])) ++a;
    while (b > a && std::isspace((unsigned char)s[b - 1])) --b;
    std::string r = s.substr(a, b - a);
    std::transform(r.begin(), r.end(), r.begin(), [](unsigned char c) { return std::tolower(c); });
    return r;
}

static std::vector<std::string> splitList(const std::string& s, char d) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, d)) {
        item = normalize(item);
        if (!item.empty()) out.push_back(item);
    }
    return out;
}

static std::string joinList(const std::vector<std::string>& v) {
    std::string r;
    for (size_t i = 0; i < v.size(); ++i) r += (i ? "," : "") + v[i];
    return r;
}

// users.txt: id|name|teach1,teach2|learn1,learn2
std::vector<User> loadUsers(const std::string& path) {
    std::vector<User> users;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::stringstream ss(line);
        std::string id, name, teach, learn;
        std::getline(ss, id, '|');
        std::getline(ss, name, '|');
        std::getline(ss, teach, '|');
        std::getline(ss, learn, '|');
        try {
            User u;
            u.id = std::stoi(id);
            u.name = name;
            u.teach = splitList(teach, ',');
            u.learn = splitList(learn, ',');
            users.push_back(u);
        } catch (...) { /* skip malformed line */ }
    }
    return users;
}

bool saveUsers(const std::string& path, const std::vector<User>& users) {
    std::ofstream out(path, std::ios::trunc);
    if (!out) return false;
    out << "# id|name|teach|learn\n";
    for (const auto& u : users)
        out << u.id << '|' << u.name << '|' << joinList(u.teach) << '|' << joinList(u.learn) << '\n';
    return true;
}

// requests.txt: reqId|PENDING|id>id>id|skill,skill,skill
int appendRequest(const std::string& path, const SwapMatch& m, const std::vector<User>& users) {
    int count = 0;
    {
        std::ifstream in(path);
        std::string line;
        while (std::getline(in, line)) if (!line.empty()) ++count;
    }
    int rid = count + 1;
    std::ofstream out(path, std::ios::app);
    out << rid << "|PENDING|";
    for (size_t i = 0; i < m.users.size(); ++i)
        out << (i ? ">" : "") << users[m.users[i]].id;
    out << '|' << joinList(m.skills) << '\n';
    return rid;
}

// notifications.txt: userId|message
void appendNotification(const std::string& path, int userId, const std::string& msg) {
    std::ofstream out(path, std::ios::app);
    out << userId << '|' << msg << '\n';
}

void loadNotifications(const std::string& path, int userId, LinkedList<std::string>& out) {
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line)) {
        size_t p = line.find('|');
        if (p == std::string::npos) continue;
        try {
            if (std::stoi(line.substr(0, p)) == userId) out.pushBack(line.substr(p + 1));
        } catch (...) {}
    }
}

}  // namespace storage
