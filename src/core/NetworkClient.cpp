#include "NetworkClient.h"
#include "MessageServer.h"
#include "Logger.h"
#include <sstream>
#include <thread>
#include <chrono>

static const char FIELD_SEP = '\x1E';

NetworkClient::NetworkClient() {}

bool NetworkClient::connect(const std::string& host, int port) {
    serverUrl = "http://" + host + ":" + std::to_string(port);

    for (int attempt = 0; attempt < 8; ++attempt) {
        auto resp = http.get(serverUrl + "/api/ping");
        if (resp.success && resp.body.find("pong") != std::string::npos) {
            connected = true;
            Logger::getInstance().log("Connected to server: " + serverUrl);
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    Logger::getInstance().log("Server not reachable, starting embedded server...");
    MessageServer::getInstance().start(port);

    for (int attempt = 0; attempt < 10; ++attempt) {
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        auto resp = http.get(serverUrl + "/api/ping");
        if (resp.success && resp.body.find("pong") != std::string::npos) {
            connected = true;
            Logger::getInstance().log("Connected to embedded server: " + serverUrl);
            return true;
        }
    }

    connected = false;
    Logger::getInstance().error("Failed to connect to messaging server at " + serverUrl);
    return false;
}

bool NetworkClient::registerOnline(const std::string& username) {
    if (!connected) return false;
    auto resp = http.post(serverUrl + "/api/register", username);
    return resp.success;
}

bool NetworkClient::sendMessage(const Message& msg) {
    if (!connected) return false;
    std::string body = msg.from + FIELD_SEP + msg.to + FIELD_SEP + msg.content + FIELD_SEP + msg.timestamp;
    auto resp = http.post(serverUrl + "/api/send", body);
    if (!resp.success) {
        Logger::getInstance().error("Failed to send message to server, status=" + std::to_string(resp.statusCode));
    } else {
        Logger::getInstance().log("Message sent to server: " + msg.from + " -> " + msg.to);
    }
    return resp.success;
}

std::vector<Message> NetworkClient::fetchMessages(const std::string& user, const std::string& contact, long long since) {
    std::vector<Message> result;
    if (!connected) return result;

    std::string url = serverUrl + "/api/messages?user=" + http.urlEncode(user) +
                      "&with=" + http.urlEncode(contact) + "&since=" + std::to_string(since);
    auto resp = http.get(url);
    if (!resp.success) {
        Logger::getInstance().error("Failed to fetch messages, status=" + std::to_string(resp.statusCode));
        return result;
    }
    if (resp.body.empty()) return result;

    std::stringstream ss(resp.body);
    std::string line;
    while (std::getline(ss, line)) {
        if (line.empty() || line == "\r") continue;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::stringstream ls(line);
        std::string field;
        std::vector<std::string> fields;
        while (std::getline(ls, field, FIELD_SEP)) {
            fields.push_back(field);
        }
        if (fields.size() >= 4) {
            Message msg;
            msg.from = fields[0];
            msg.to = fields[1];
            msg.content = fields[2];
            msg.timestamp = fields[3];
            result.push_back(msg);
        }
    }
    return result;
}

std::vector<std::string> NetworkClient::fetchUsers() {
    std::vector<std::string> result;
    if (!connected) return result;

    auto resp = http.get(serverUrl + "/api/users");
    if (!resp.success) return result;

    std::stringstream ss(resp.body);
    std::string user;
    while (std::getline(ss, user, ',')) {
        if (!user.empty()) result.push_back(user);
    }
    return result;
}
