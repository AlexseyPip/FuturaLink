#pragma once
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include "../models/Message.h"

class MessageServer {
public:
    static MessageServer& getInstance();
    
    bool start(int port = 8765);
    void stop();
    bool isRunning() const { return running && bindOk; }
    int getPort() const { return port; }
    
    void addMessage(const Message& msg);
    std::vector<Message> getMessages(const std::string& user, const std::string& contact, long long since = 0);
    std::vector<std::string> getOnlineUsers();
    void registerUser(const std::string& username);
    
    std::string handleRequest(const std::string& method, const std::string& path, const std::string& body);
    
private:
    MessageServer() = default;
    
    std::atomic<bool> running{false};
    std::atomic<bool> bindOk{false};
    int port = 8765;
    std::thread serverThread;
    std::mutex dataMutex;
    std::vector<Message> messages;
    std::vector<std::string> onlineUsers;
    
    void serverLoop();
    void saveToDisk();
    void loadFromDisk();
    static std::string urlDecode(const std::string& str);
    static std::string getQueryParam(const std::string& path, const std::string& key);
};
