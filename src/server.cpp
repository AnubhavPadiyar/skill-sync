// Skill Sync HTTP backend: Frontend -> this server -> C++ DSA engine -> files
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>
#include "graph.h"
#include "httplib.h"
#include "linked_list.h"
#include "matcher.h"
#include "storage.h"

static const std::string USERS_FILE = "data/users.txt";
static const std::string REQUESTS_FILE = "data/requests.txt";
static const std::string NOTIF_FILE = "data/notifications.txt";

static std::mutex mtx;
static std::vector<User> users;
static SkillGraph graph;

// ---------- tiny JSON helpers ----------
static std::string esc(const std::string& s) {
    std::string r;
    for (char c : s) {
        if (c == '"' || c == '\\') { r += '\\'; r += c; }
        else if (c == '\n') r += "\\n";
        else if ((unsigned char)c < 0x20) r += ' ';
        else r += c;
    }
    return r;
}
static std::string strList(const std::vector<std::string>& v) {
    std::string r = "[";
    for (size_t i = 0; i < v.size(); ++i) r += (i ? "," : "") + std::string("\"") + esc(v[i]) + "\"";
    return r + "]";
}
static std::string userJson(const User& u) {
    return "{\"id\":" + std::to_string(u.id) + ",\"name\":\"" + esc(u.name) + "\",\"teach\":" +
           strList(u.teach) + ",\"learn\":" + strList(u.learn) + "}";
}
static std::string matchJson(const SwapMatch& m) {
    std::string r = "{\"users\":[";
    for (size_t i = 0; i < m.users.size(); ++i) {
        const User& u = users[m.users[i]];
        r += (i ? "," : "") + std::string("{\"id\":") + std::to_string(u.id) + ",\"name\":\"" + esc(u.name) + "\"}";
    }
    return r + "],\"skills\":" + strList(m.skills) + "}";
}
static void json(httplib::Response& res, const std::string& body, int status = 200) {
    res.status = status;
    res.set_content(body, "application/json");
}
static void err(httplib::Response& res, int status, const std::string& msg) {
    json(res, "{\"error\":\"" + esc(msg) + "\"}", status);
}

// ---------- helpers ----------
static int indexOfId(int id) {
    for (size_t i = 0; i < users.size(); ++i) if (users[i].id == id) return (int)i;
    return -1;
}
static int idxParam(const httplib::Request& req, const char* key = "user") {
    try { return indexOfId(std::stoi(req.get_param_value(key))); } catch (...) { return -1; }
}
static std::vector<std::string> splitCSV(const std::string& s) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s + ",") {
        if (c == ',') {
            cur = storage::normalize(cur);
            if (!cur.empty()) {
                bool dup = false;
                for (auto& x : out) if (x == cur) dup = true;
                if (!dup) out.push_back(cur);
            }
            cur.clear();
        } else cur += c;
    }
    return out;
}
static void rebuild() { graph.build(users); }

int main() {
    users = storage::loadUsers(USERS_FILE);
    rebuild();
    httplib::Server svr;

    svr.Get("/api/stats", [](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(mtx);
        int edges = 0;
        for (int i = 0; i < graph.size(); ++i) edges += (int)graph.neighbors(i).size();
        json(res, "{\"users\":" + std::to_string(users.size()) + ",\"edges\":" + std::to_string(edges) +
                      ",\"hasCycle\":" + (matcher::hasCycle(graph) ? "true" : "false") + "}");
    });

    svr.Get("/api/users", [](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(mtx);
        std::string r = "[";
        for (size_t i = 0; i < users.size(); ++i) r += (i ? "," : "") + userJson(users[i]);
        json(res, r + "]");
    });

    svr.Post("/api/register", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(mtx);
        std::string name = req.get_param_value("name");
        if (storage::normalize(name).empty()) return err(res, 400, "Enter your name.");
        User u;
        u.id = users.empty() ? 1 : users.back().id + 1;
        u.name = name;
        for (char& c : u.name) if (c == '|') c = ' ';
        u.teach = splitCSV(req.get_param_value("teach"));
        u.learn = splitCSV(req.get_param_value("learn"));
        users.push_back(u);
        storage::saveUsers(USERS_FILE, users);
        rebuild();
        json(res, userJson(u), 201);
    });

    svr.Post("/api/login", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(mtx);
        std::string name = storage::normalize(req.get_param_value("name"));
        for (const auto& u : users)
            if (storage::normalize(u.name) == name) return json(res, userJson(u));
        err(res, 404, "No account with that name. Create one first.");
    });

    svr.Post("/api/profile", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(mtx);
        int i = idxParam(req, "id");
        if (i < 0) return err(res, 404, "User not found.");
        users[i].teach = splitCSV(req.get_param_value("teach"));
        users[i].learn = splitCSV(req.get_param_value("learn"));
        storage::saveUsers(USERS_FILE, users);
        rebuild();
        json(res, userJson(users[i]));
    });

    // skill search: exact/prefix via hash index + binary search
    svr.Get("/api/search", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(mtx);
        std::string q = storage::normalize(req.get_param_value("q"));
        auto skills = graph.skillsWithPrefix(q);
        std::string r = "[";
        for (size_t i = 0; i < skills.size(); ++i) {
            r += (i ? "," : "") + std::string("{\"skill\":\"") + esc(skills[i]) + "\",\"teachers\":[";
            const auto& t = graph.teachersOf(skills[i]);
            for (size_t j = 0; j < t.size(); ++j)
                r += (j ? "," : "") + std::string("{\"id\":") + std::to_string(users[t[j]].id) +
                     ",\"name\":\"" + esc(users[t[j]].name) + "\"}";
            r += "]}";
        }
        json(res, r + "]");
    });

    // type = direct | multi | shortest
    svr.Get("/api/matches", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(mtx);
        int i = idxParam(req);
        if (i < 0) return err(res, 404, "User not found.");
        std::string type = req.get_param_value("type");
        std::vector<SwapMatch> ms;
        if (type == "direct") ms = matcher::findDirect(graph, i);
        else if (type == "shortest") {
            SwapMatch m = matcher::shortestCycle(graph, i);
            if (!m.users.empty()) ms.push_back(m);
        } else ms = matcher::findMulti(graph, i);
        std::string r = "[";
        for (size_t k = 0; k < ms.size(); ++k) r += (k ? "," : "") + matchJson(ms[k]);
        json(res, r + "]");
    });

    // body: ids=1,2,3 (cycle order). Server re-validates every edge before saving.
    svr.Post("/api/requests", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(mtx);
        std::vector<int> idx;
        for (const auto& s : splitCSV(req.get_param_value("ids"))) {
            try { int k = indexOfId(std::stoi(s)); if (k >= 0) idx.push_back(k); } catch (...) {}
        }
        if (idx.size() < 2) return err(res, 400, "A swap needs at least two people.");
        SwapMatch m;
        m.users = idx;
        for (size_t i = 0; i < idx.size(); ++i) {
            std::string sk;
            if (!graph.findEdge(idx[i], idx[(i + 1) % idx.size()], &sk))
                return err(res, 400, "That swap is no longer valid.");
            m.skills.push_back(sk);
        }
        int rid = storage::appendRequest(REQUESTS_FILE, m, users);
        std::string text = "Swap request #" + std::to_string(rid) + ": ";
        for (size_t i = 0; i < m.users.size(); ++i)
            text += users[m.users[i]].name + " teaches " + m.skills[i] + " to " +
                    users[m.users[(i + 1) % m.users.size()]].name + (i + 1 < m.users.size() ? "; " : "");
        for (int k : m.users) storage::appendNotification(NOTIF_FILE, users[k].id, text);
        json(res, "{\"id\":" + std::to_string(rid) + ",\"match\":" + matchJson(m) + "}", 201);
    });

    // list requests that involve a user
    svr.Get("/api/requests", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(mtx);
        int i = idxParam(req);
        if (i < 0) return err(res, 404, "User not found.");
        std::ifstream in(REQUESTS_FILE);
        std::string line, r = "[";
        bool first = true;
        while (std::getline(in, line)) {
            std::stringstream ss(line);
            std::string rid, status, ids, skills;
            std::getline(ss, rid, '|'); std::getline(ss, status, '|');
            std::getline(ss, ids, '|'); std::getline(ss, skills, '|');
            SwapMatch m;
            bool mine = false;
            std::stringstream is(ids);
            std::string tok;
            while (std::getline(is, tok, '>')) {
                try { int k = indexOfId(std::stoi(tok)); if (k >= 0) { m.users.push_back(k); if (k == i) mine = true; } } catch (...) {}
            }
            if (!mine || m.users.size() < 2) continue;
            m.skills = splitCSV(skills);
            if (m.skills.size() != m.users.size()) continue;
            r += std::string(first ? "" : ",") + "{\"id\":" + esc(rid) + ",\"status\":\"" + esc(status) +
                 "\",\"match\":" + matchJson(m) + "}";
            first = false;
        }
        json(res, r + "]");
    });

    svr.Get("/api/notifications", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lk(mtx);
        int i = idxParam(req);
        if (i < 0) return err(res, 404, "User not found.");
        LinkedList<std::string> list;
        storage::loadNotifications(NOTIF_FILE, users[i].id, list);
        std::vector<std::string> v;
        list.forEach([&](const std::string& s) { v.push_back(s); });
        json(res, strList(v));
    });

    svr.set_mount_point("/", "./web");
    std::cout << "Skill Sync running on http://localhost:8080\n";
    if (!svr.listen("0.0.0.0", 8080)) {
        std::cerr << "Could not start server on port 8080 (already in use?)\n";
        return 1;
    }
    return 0;
}
