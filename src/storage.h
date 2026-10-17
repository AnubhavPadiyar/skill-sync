#pragma once
#include <string>
#include <vector>
#include "linked_list.h"
#include "types.h"

// File handling layer (plain text files, '|' separated, ',' for lists)
namespace storage {
std::string normalize(const std::string& s);  // trim + lowercase
std::vector<User> loadUsers(const std::string& path);
bool saveUsers(const std::string& path, const std::vector<User>& users);
// returns new request id
int appendRequest(const std::string& path, const SwapMatch& m, const std::vector<User>& users);
void appendNotification(const std::string& path, int userId, const std::string& msg);
void loadNotifications(const std::string& path, int userId, LinkedList<std::string>& out);
}
