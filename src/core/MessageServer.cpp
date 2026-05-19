#include "MessageServer.h"
#include "Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <ctime>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#define closesocket close
#endif

static const char FIELD_SEP = '\x1E';

MessageServer& MessageServer::getInstance() {
    static MessageServer instance;
    return instance;
}

bool MessageServer::start(int p) {
    if (running) return true;
    port = p;
    loadFromDisk();
    running = true;
    serverThread = std::thread(&MessageServer::serverLoop, this);
    Logger::getInstance().log("Message server started on port " + std::to_string(port));
    return true;
}

void MessageServer::stop() {
    if (!running) return;
    running = false;
#ifdef _WIN32
    SOCKET probe = socket(AF_INET, SOCK_STREAM, 0);
    if (probe != INVALID_SOCKET) {
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        connect(probe, (sockaddr*)&addr, sizeof(addr));
        closesocket(probe);
    }
#endif
    if (serverThread.joinable()) serverThread.join();
    saveToDisk();
}

void MessageServer::registerUser(const std::string& username) {
    std::lock_guard<std::mutex> lock(dataMutex);
    if (std::find(onlineUsers.begin(), onlineUsers.end(), username) == onlineUsers.end()) {
        onlineUsers.push_back(username);
    }
}

void MessageServer::addMessage(const Message& msg) {
    std::lock_guard<std::mutex> lock(dataMutex);
    messages.push_back(msg);
    saveToDisk();
}

std::vector<Message> MessageServer::getMessages(const std::string& user, const std::string& contact, long long since) {
    std::lock_guard<std::mutex> lock(dataMutex);
    std::vector<Message> result;
    for (const auto& msg : messages) {
        long long ts = 0;
        try { ts = std::stoll(msg.timestamp); } catch (...) {}
        if (since > 0 && ts <= since) continue;
        if ((msg.from == user && msg.to == contact) || (msg.from == contact && msg.to == user)) {
            result.push_back(msg);
        }
    }
    return result;
}

std::vector<std::string> MessageServer::getOnlineUsers() {
    std::lock_guard<std::mutex> lock(dataMutex);
    return onlineUsers;
}

std::string MessageServer::urlDecode(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '%' && i + 2 < str.size()) {
            int val = 0;
            sscanf(str.substr(i + 1, 2).c_str(), "%x", &val);
            result += static_cast<char>(val);
            i += 2;
        } else if (str[i] == '+') {
            result += ' ';
        } else {
            result += str[i];
        }
    }
    return result;
}

std::string MessageServer::getQueryParam(const std::string& path, const std::string& key) {
    size_t qpos = path.find('?');
    if (qpos == std::string::npos) return "";
    std::string query = path.substr(qpos + 1);
    std::string search = key + "=";
    size_t pos = query.find(search);
    if (pos == std::string::npos) return "";
    std::string val = query.substr(pos + search.size());
    size_t amp = val.find('&');
    if (amp != std::string::npos) val = val.substr(0, amp);
    return urlDecode(val);
}

std::string MessageServer::handleRequest(const std::string& method, const std::string& path, const std::string& body) {
    if (method == "POST" && path.find("/api/send") == 0) {
        std::stringstream ss(body);
        std::string field;
        std::vector<std::string> fields;
        while (std::getline(ss, field, FIELD_SEP)) {
            fields.push_back(field);
        }
        if (fields.size() >= 3) {
            Message msg;
            msg.from = fields[0];
            msg.to = fields[1];
            msg.content = fields[2];
            msg.timestamp = fields.size() > 3 ? fields[3] : std::to_string(time(nullptr));
            addMessage(msg);
        }
        return "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nOK";
    }
    
    if (method == "GET" && path.find("/api/messages") == 0) {
        std::string user = getQueryParam(path, "user");
        std::string contact = getQueryParam(path, "with");
        std::string sinceStr = getQueryParam(path, "since");
        long long since = 0;
        if (!sinceStr.empty()) try { since = std::stoll(sinceStr); } catch (...) {}
        
        auto msgs = getMessages(user, contact, since);
        std::stringstream bodyOut;
        for (const auto& m : msgs) {
            bodyOut << m.from << FIELD_SEP << m.to << FIELD_SEP << m.content << FIELD_SEP << m.timestamp << "\n";
        }
        std::string bodyStr = bodyOut.str();
        return "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\nContent-Length: " +
               std::to_string(bodyStr.size()) + "\r\n\r\n" + bodyStr;
    }
    
    if (method == "GET" && path.find("/api/users") == 0) {
        auto users = getOnlineUsers();
        std::string bodyStr;
        for (size_t i = 0; i < users.size(); ++i) {
            if (i > 0) bodyStr += ",";
            bodyStr += users[i];
        }
        return "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\nContent-Length: " +
               std::to_string(bodyStr.size()) + "\r\n\r\n" + bodyStr;
    }
    
    if (method == "POST" && path.find("/api/register") == 0) {
        registerUser(body);
        return "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nOK";
    }
    
    if (method == "GET" && path.find("/api/ping") == 0) {
        return "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\npong";
    }
    
    return "HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\nNot Found";
}

void MessageServer::serverLoop() {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    
    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSock == INVALID_SOCKET) {
        running = false;
        bindOk = false;
        return;
    }
    
    int opt = 1;
    setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(listenSock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(listenSock);
        running = false;
        bindOk = false;
        return;
    }
    
    listen(listenSock, 10);
    bindOk = true;
    
    while (running) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(listenSock, &readSet);
        timeval tv{1, 0};
        
        if (select(0, &readSet, nullptr, nullptr, &tv) <= 0) continue;
        
        SOCKET clientSock = accept(listenSock, nullptr, nullptr);
        if (clientSock == INVALID_SOCKET) continue;
        
        char buffer[8192] = {};
        int received = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
        if (received > 0) {
            std::string request(buffer, received);
            
            std::string method, path, body;
            size_t lineEnd = request.find("\r\n");
            if (lineEnd != std::string::npos) {
                std::string requestLine = request.substr(0, lineEnd);
                std::stringstream rl(requestLine);
                rl >> method >> path;
            }
            
            size_t bodyStart = request.find("\r\n\r\n");
            if (bodyStart != std::string::npos) {
                body = request.substr(bodyStart + 4);
            }
            
            std::string response = handleRequest(method, path, body);
            send(clientSock, response.c_str(), response.size(), 0);
        }
        closesocket(clientSock);
    }
    
    closesocket(listenSock);
#ifdef _WIN32
    WSACleanup();
#endif
}

void MessageServer::saveToDisk() {
    std::ofstream file("server_messages.dat", std::ios::binary);
    if (!file.is_open()) return;
    for (const auto& msg : messages) {
        file << msg.from << FIELD_SEP << msg.to << FIELD_SEP << msg.content << FIELD_SEP << msg.timestamp << "\n";
    }
}

void MessageServer::loadFromDisk() {
    std::ifstream file("server_messages.dat", std::ios::binary);
    if (!file.is_open()) return;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string field;
        std::vector<std::string> fields;
        while (std::getline(ss, field, FIELD_SEP)) {
            fields.push_back(field);
        }
        if (fields.size() >= 4) {
            Message msg;
            msg.from = fields[0];
            msg.to = fields[1];
            msg.content = fields[2];
            msg.timestamp = fields[3];
            messages.push_back(msg);
        }
    }
}
