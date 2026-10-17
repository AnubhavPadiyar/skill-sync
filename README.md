# Skill Sync - Phase 2 (C++ DSA engine + backend + frontend)

Architecture: web/ (frontend) -> src/server.cpp (C++ backend) -> graph/matcher (C++ DSA engine) -> data/*.txt (file storage)

Run the web app:
1. `make`
2. `./server`
3. open http://localhost:8080

Run from this folder so `data/` and `web/` resolve. `./skillsync` is the CLI version of the same engine.

| File | What it does |
|---|---|
| src/graph.* | Directed graph, adjacency list, hash index (skill -> teachers), sorted skills + binary search |
| src/matcher.* | Direct swap, chain swaps (DFS + backtracking), shortest loop (BFS), cycle detection |
| src/linked_list.h | Linked list (notifications) + queue (BFS) |
| src/storage.* | File handling: users / requests / notifications |
| src/server.cpp | REST API (cpp-httplib, bundled as src/httplib.h) + serves web/ |
| web/ | Landing, sign in / create account, dashboard, find skills, matches, requests + notifications, profile |

Edge u -> v means u teaches v a skill v wants. Sign-in is name-only for now (no passwords); real auth comes with the DB phase.
