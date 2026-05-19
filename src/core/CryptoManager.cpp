#include "CryptoManager.h"
#include <cstring>
#include <sstream>
#include <iomanip>
#include <random>
#include <chrono>

std::string CryptoManager::hashPassword(const std::string& password) {
    // Simple SHA-256 like hash (for demonstration)
    unsigned int hash = 0;
    for (char c : password) {
        hash = ((hash << 5) + hash) + c;
    }
    
    std::stringstream ss;
    ss << std::hex << hash;
    return ss.str();
}

std::string CryptoManager::generateKey(const std::string& password) {
    return hashPassword(password);
}

void CryptoManager::xorCipher(std::vector<unsigned char>& data, const std::string& key) {
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] ^= key[i % key.length()];
    }
}

std::string CryptoManager::toHex(const std::vector<unsigned char>& data) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (unsigned char c : data) {
        ss << std::setw(2) << (int)c;
    }
    return ss.str();
}

std::vector<unsigned char> CryptoManager::fromHex(const std::string& hex) {
    std::vector<unsigned char> data;
    for (size_t i = 0; i < hex.length(); i += 2) {
        unsigned int byte;
        std::stringstream ss;
        ss << std::hex << hex.substr(i, 2);
        ss >> byte;
        data.push_back((unsigned char)byte);
    }
    return data;
}

std::string CryptoManager::encrypt(const std::string& data, const std::string& key) {
    std::vector<unsigned char> bytes(data.begin(), data.end());
    xorCipher(bytes, key);
    return toHex(bytes);
}

std::string CryptoManager::decrypt(const std::string& data, const std::string& key) {
    std::vector<unsigned char> bytes = fromHex(data);
    xorCipher(bytes, key);
    return std::string(bytes.begin(), bytes.end());
}

