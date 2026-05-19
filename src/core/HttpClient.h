#pragma once
#include <string>
#include <functional>
#include <memory>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

class HttpClient {
public:
    HttpClient();
    ~HttpClient();
    
    struct Response {
        int statusCode;
        std::string body;
        bool success;
        Response() : statusCode(0), success(false) {}
    };
    
    Response get(const std::string& url);
    Response post(const std::string& url, const std::string& data);
    Response postFile(const std::string& url, const std::string& filePath);
    std::string urlEncode(const std::string& str);  // Made public
    
private:
    std::string parseUrl(const std::string& url, std::string& host, std::string& path, int& port);
    Response sendRequest(const std::string& host, int port, const std::string& request);
    
#ifdef _WIN32
    WSADATA wsaData;
    bool winsockInitialized;
#endif
};
