#pragma once
#include <string>
#include <vector>
#include <map>
#include "../models/User.h"
#include "../models/Message.h"

class Database {
public:
    static Database& getInstance();
    
    bool init(const std::string& password);
    void setCurrentUser(const std::string& username);
    std::string getCurrentUser() const { return currentUser; }
    bool saveUser(const User& user);
    User* getUser(const std::string& username);
    bool saveMessage(const Message& message);
    std::vector<Message> getMessages(const std::string& contact);
    bool addContact(const std::string& username);
    bool hasMessage(const Message& message) const;
    std::vector<std::string> getContacts();
    std::vector<std::string> searchUsers(const std::string& query);
    
private:
    Database() = default;
    ~Database() = default;
    
    std::string dataPath;
    std::string encryptionKey;
    std::string currentUser;
    std::map<std::string, User> users;
    std::vector<Message> messages;
    std::map<std::string, std::vector<std::string>> userContacts;
    
    bool loadData();
    bool saveData();
    std::string encryptData(const std::string& data);
    std::string decryptData(const std::string& data);
};
