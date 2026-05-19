#pragma once
#include <string>
#include <fstream>
#include <mutex>

class Logger {
public:
    static Logger& getInstance();
    void log(const std::string& message);
    void error(const std::string& message);
    
private:
    Logger();
    ~Logger();
    std::ofstream logFile;
    std::mutex mtx;
};
