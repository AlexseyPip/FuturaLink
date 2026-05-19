#include "Logger.h"
#include <chrono>
#include <iomanip>
#include <iostream>

Logger::Logger() {
    logFile.open("messenger.log", std::ios::out | std::ios::app);
    if (logFile.is_open()) {
        log("Logger initialized");
    }
}

Logger::~Logger() {
    if (logFile.is_open()) {
        log("Logger shutting down");
        logFile.close();
    }
}

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::log(const std::string& message) {
    std::lock_guard<std::mutex> lock(mtx);
    if (logFile.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        logFile << "[INFO] " << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << " - " << message << std::endl;
        logFile.flush();
    }
    std::cout << "[INFO] " << message << std::endl;
}

void Logger::error(const std::string& message) {
    std::lock_guard<std::mutex> lock(mtx);
    if (logFile.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        logFile << "[ERROR] " << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S") << " - " << message << std::endl;
        logFile.flush();
    }
    std::cerr << "[ERROR] " << message << std::endl;
}
