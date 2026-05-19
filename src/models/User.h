#pragma once
#include <string>

struct User {
    std::string username;
    std::string passwordHash;
    std::string displayName;
    bool online;
    
    User() : online(false) {}
    User(const std::string& uname, const std::string& pwdHash) 
        : username(uname), passwordHash(pwdHash), online(false) {}
};
