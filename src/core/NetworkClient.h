#pragma once
#include <string>
#include <vector>
#include "../models/Message.h"
#include "HttpClient.h"

class NetworkClient {
public:
    NetworkClient();
    
    bool connect(const std::string& host = "127.0.0.1", int port = 8765);
    bool isConnected() const { return connected; }
    std::string getServerUrl() const { return serverUrl; }
    
    bool sendMessage(const Message& msg);
    std::vector<Message> fetchMessages(const std::string& user, const std::string& contact, long long since = 0);
    std::vector<std::string> fetchUsers();
    bool registerOnline(const std::string& username);
    
private:
    HttpClient http;
    std::string serverUrl;
    bool connected = false;
    
    static std::string encodeField(const std::string& s);
    static std::string decodeField(const std::string& s);
};
