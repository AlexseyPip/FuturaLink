#include "HttpClient.h"
#include <sstream>
#include <fstream>
#include <cstring>
#include <cstdio>

HttpClient::HttpClient() {
#ifdef _WIN32
    winsockInitialized = (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0);
#else
    // Unix/Linux не требует инициализации
#endif
}

HttpClient::~HttpClient() {
#ifdef _WIN32
    if (winsockInitialized) {
        WSACleanup();
    }
#endif
}

std::string HttpClient::urlEncode(const std::string& str) {
    std::string encoded;
    for (char c : str) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        } else {
            char hex[4];
            snprintf(hex, sizeof(hex), "%%%02X", (unsigned char)c);
            encoded += hex;
        }
    }
    return encoded;
}

std::string HttpClient::parseUrl(const std::string& url, std::string& host, std::string& path, int& port) {
    std::string tempUrl = url;
    
    // Remove http:// or https://
    bool isHttps = false;
    if (tempUrl.find("https://") == 0) {
        isHttps = true;
        tempUrl = tempUrl.substr(8);
        port = 443;
    } else if (tempUrl.find("http://") == 0) {
        tempUrl = tempUrl.substr(7);
        port = 80;
    } else {
        port = 80;
    }
    
    // Find host and path
    size_t slashPos = tempUrl.find('/');
    if (slashPos == std::string::npos) {
        host = tempUrl;
        path = "/";
    } else {
        host = tempUrl.substr(0, slashPos);
        path = tempUrl.substr(slashPos);
    }
    
    // Remove port from host if present
    size_t colonPos = host.find(':');
    if (colonPos != std::string::npos) {
        port = std::stoi(host.substr(colonPos + 1));
        host = host.substr(0, colonPos);
    }
    
    return isHttps ? "https" : "http";
}

HttpClient::Response HttpClient::sendRequest(const std::string& host, int port, const std::string& request) {
    Response response;
    response.success = false;
    response.statusCode = 0;
    
#ifdef _WIN32
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) return response;
    
    struct hostent* server = gethostbyname(host.c_str());
    if (server == nullptr) {
        closesocket(sock);
        return response;
    }
    
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    memcpy(&serverAddr.sin_addr.s_addr, server->h_addr, server->h_length);
    serverAddr.sin_port = htons(port);
    
    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(sock);
        return response;
    }
    
    if (send(sock, request.c_str(), request.length(), 0) == SOCKET_ERROR) {
        closesocket(sock);
        return response;
    }
    
    char buffer[4096];
    std::string fullResponse;
    int bytesReceived;
    while ((bytesReceived = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        fullResponse.append(buffer, bytesReceived);
    }
    
    closesocket(sock);
#else
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return response;
    
    struct hostent* server = gethostbyname(host.c_str());
    if (server == nullptr) {
        close(sock);
        return response;
    }
    
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    memcpy(&serverAddr.sin_addr.s_addr, server->h_addr, server->h_length);
    serverAddr.sin_port = htons(port);
    
    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        close(sock);
        return response;
    }
    
    send(sock, request.c_str(), request.length(), 0);
    
    char buffer[4096];
    std::string fullResponse;
    int bytesReceived;
    while ((bytesReceived = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
        fullResponse.append(buffer, bytesReceived);
    }
    
    close(sock);
#endif
    
    // Parse response
    size_t headerEnd = fullResponse.find("\r\n\r\n");
    if (headerEnd != std::string::npos) {
        std::string headers = fullResponse.substr(0, headerEnd);
        response.body = fullResponse.substr(headerEnd + 4);
        
        // Parse status code
        size_t statusPos = headers.find(' ');
        if (statusPos != std::string::npos) {
            response.statusCode = std::stoi(headers.substr(statusPos + 1));
            response.success = (response.statusCode >= 200 && response.statusCode < 300);
        }
    }
    
    return response;
}

static std::string makeHostHeader(const std::string& host, int port) {
    if (port == 80) return host;
    return host + ":" + std::to_string(port);
}

HttpClient::Response HttpClient::get(const std::string& url) {
    std::string host, path;
    int port;
    parseUrl(url, host, path, port);
    
    std::string request = "GET " + path + " HTTP/1.1\r\n"
                         "Host: " + makeHostHeader(host, port) + "\r\n"
                         "Connection: close\r\n"
                         "\r\n";
    
    return sendRequest(host, port, request);
}

HttpClient::Response HttpClient::post(const std::string& url, const std::string& data) {
    std::string host, path;
    int port;
    parseUrl(url, host, path, port);
    
    std::string request = "POST " + path + " HTTP/1.1\r\n"
                         "Host: " + makeHostHeader(host, port) + "\r\n"
                         "Content-Type: text/plain\r\n"
                         "Content-Length: " + std::to_string(data.length()) + "\r\n"
                         "Connection: close\r\n"
                         "\r\n" + data;
    
    return sendRequest(host, port, request);
}

HttpClient::Response HttpClient::postFile(const std::string& url, const std::string& filePath) {
    // Read file
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        Response resp;
        resp.success = false;
        return resp;
    }
    
    std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::string body = "--" + boundary + "\r\n"
                      "Content-Disposition: form-data; name=\"file\"; filename=\"" + filePath + "\"\r\n"
                      "Content-Type: application/octet-stream\r\n\r\n" +
                      fileContent + "\r\n--" + boundary + "--\r\n";
    
    std::string host, path;
    int port;
    parseUrl(url, host, path, port);
    
    std::string request = "POST " + path + " HTTP/1.1\r\n"
                         "Host: " + host + "\r\n"
                         "Content-Type: multipart/form-data; boundary=" + boundary + "\r\n"
                         "Content-Length: " + std::to_string(body.length()) + "\r\n"
                         "Connection: close\r\n"
                         "\r\n" + body;
    
    return sendRequest(host, port, request);
}
