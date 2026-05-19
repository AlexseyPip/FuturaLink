#pragma once
#include <string>
#include <chrono>

struct Message {
    std::string from;
    std::string to;
    std::string content;
    std::string timestamp;
    bool isFile;
    std::string filePath;
    
    Message() : isFile(false) {}
    Message(const std::string& f, const std::string& t, const std::string& c)
        : from(f), to(t), content(c), isFile(false) {
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        timestamp = std::to_string(ms);
    }
};
