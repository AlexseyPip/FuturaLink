#include "Database.h"
#include "CryptoManager.h"
#include <fstream>
#include <sstream>
#include <iostream>

static const char FIELD_SEP = '\x1E';

Database& Database::getInstance() {
    static Database instance;
    return instance;
}

bool Database::init(const std::string& password) {
    dataPath = "messenger_data.db";
    encryptionKey = CryptoManager::generateKey(password);
    currentUser.clear();
    return loadData();
}

void Database::setCurrentUser(const std::string& username) {
    currentUser = username;
}

bool Database::loadData() {
    std::ifstream file(dataPath, std::ios::binary);
    if (!file.is_open()) {
        users.clear();
        messages.clear();
        userContacts.clear();
        User defaultUser("admin", CryptoManager::hashPassword("admin123"));
        users["admin"] = defaultUser;
        User testUser("admin2", CryptoManager::hashPassword("admin123"));
        users["admin2"] = testUser;
        return saveData();
    }
    
    std::string encryptedData((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    std::string decryptedData = decryptData(encryptedData);
    if (decryptedData.empty()) {
        users.clear();
        messages.clear();
        userContacts.clear();
        User defaultUser("admin", CryptoManager::hashPassword("admin123"));
        users["admin"] = defaultUser;
        User testUser("admin2", CryptoManager::hashPassword("admin123"));
        users["admin2"] = testUser;
        return saveData();
    }
    
    std::stringstream ss(decryptedData);
    std::string segment;
    
    while (std::getline(ss, segment, '|')) {
        if (segment.find("USERS:") == 0) {
            std::string usersData = segment.substr(6);
            std::stringstream userStream(usersData);
            std::string userEntry;
            while (std::getline(userStream, userEntry, ';')) {
                if (userEntry.empty()) continue;
                size_t colonPos = userEntry.find(':');
                if (colonPos != std::string::npos) {
                    User user;
                    user.username = userEntry.substr(0, colonPos);
                    user.passwordHash = userEntry.substr(colonPos + 1);
                    users[user.username] = user;
                }
            }
        } else if (segment.find("CONTACTS:") == 0) {
            std::string contactsData = segment.substr(9);
            std::stringstream contactStream(contactsData);
            std::string userEntry;
            while (std::getline(contactStream, userEntry, ';')) {
                if (userEntry.empty()) continue;
                size_t colonPos = userEntry.find(':');
                if (colonPos == std::string::npos) continue;
                std::string owner = userEntry.substr(0, colonPos);
                std::string list = userEntry.substr(colonPos + 1);
                std::stringstream listStream(list);
                std::string contact;
                while (std::getline(listStream, contact, ',')) {
                    if (!contact.empty()) userContacts[owner].push_back(contact);
                }
            }
        } else if (segment.find("MESSAGES:") == 0) {
            std::string messagesData = segment.substr(9);
            std::stringstream msgStream(messagesData);
            std::string msgEntry;
            while (std::getline(msgStream, msgEntry, ';')) {
                if (msgEntry.empty()) continue;
                std::stringstream msgFieldStream(msgEntry);
                std::string field;
                std::vector<std::string> fields;
                while (std::getline(msgFieldStream, field, FIELD_SEP)) {
                    fields.push_back(field);
                }
                if (fields.size() >= 4) {
                    Message msg;
                    msg.from = fields[0];
                    msg.to = fields[1];
                    msg.content = fields[2];
                    msg.timestamp = fields[3];
                    if (fields.size() > 4) msg.isFile = (fields[4] == "1");
                    if (fields.size() > 5) msg.filePath = fields[5];
                    messages.push_back(msg);
                }
            }
        }
    }
    
    return true;
}

bool Database::saveData() {
    std::stringstream ss;
    
    ss << "USERS:";
    for (const auto& pair : users) {
        ss << pair.first << ":" << pair.second.passwordHash << ";";
    }
    ss << "|";
    
    ss << "CONTACTS:";
    for (const auto& pair : userContacts) {
        ss << pair.first << ":";
        for (size_t i = 0; i < pair.second.size(); ++i) {
            if (i > 0) ss << ",";
            ss << pair.second[i];
        }
        ss << ";";
    }
    ss << "|";
    
    ss << "MESSAGES:";
    for (const auto& msg : messages) {
        ss << msg.from << FIELD_SEP << msg.to << FIELD_SEP << msg.content << FIELD_SEP
           << msg.timestamp << FIELD_SEP << (msg.isFile ? "1" : "0") << FIELD_SEP << msg.filePath << ";";
    }
    ss << "|";
    
    std::string encryptedData = encryptData(ss.str());
    if (encryptedData.empty()) return false;
    
    std::ofstream file(dataPath, std::ios::binary);
    if (!file.is_open()) return false;
    
    file.write(encryptedData.c_str(), encryptedData.length());
    file.close();
    return true;
}

std::string Database::encryptData(const std::string& data) {
    return CryptoManager::encrypt(data, encryptionKey);
}

std::string Database::decryptData(const std::string& data) {
    return CryptoManager::decrypt(data, encryptionKey);
}

bool Database::saveUser(const User& user) {
    users[user.username] = user;
    return saveData();
}

User* Database::getUser(const std::string& username) {
    auto it = users.find(username);
    if (it != users.end()) {
        return &it->second;
    }
    return nullptr;
}

bool Database::saveMessage(const Message& message) {
    if (hasMessage(message)) return true;
    messages.push_back(message);
    return saveData();
}

bool Database::hasMessage(const Message& message) const {
    for (const auto& msg : messages) {
        if (msg.from == message.from && msg.to == message.to &&
            msg.content == message.content && msg.timestamp == message.timestamp) {
            return true;
        }
    }
    return false;
}

std::vector<Message> Database::getMessages(const std::string& contact) {
    std::vector<Message> result;
    for (const auto& msg : messages) {
        if ((msg.from == contact && msg.to == currentUser) ||
            (msg.to == contact && msg.from == currentUser)) {
            result.push_back(msg);
        }
    }
    return result;
}

bool Database::addContact(const std::string& username) {
    if (username.empty() || username == currentUser) return false;
    auto& list = userContacts[currentUser];
    for (const auto& contact : list) {
        if (contact == username) return true;
    }
    list.push_back(username);
    return saveData();
}

std::vector<std::string> Database::getContacts() {
    auto it = userContacts.find(currentUser);
    if (it == userContacts.end()) return {};
    return it->second;
}

std::vector<std::string> Database::searchUsers(const std::string& query) {
    std::vector<std::string> results;
    const auto& myContacts = userContacts[currentUser];
    for (const auto& pair : users) {
        if (pair.first != currentUser &&
            pair.first.find(query) != std::string::npos) {
            bool alreadyContact = false;
            for (const auto& contact : myContacts) {
                if (contact == pair.first) {
                    alreadyContact = true;
                    break;
                }
            }
            if (!alreadyContact) {
                results.push_back(pair.first);
            }
        }
    }
    return results;
}
