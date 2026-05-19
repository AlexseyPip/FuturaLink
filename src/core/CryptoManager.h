#pragma once
#include <string>
#include <vector>

class CryptoManager {
public:
    static std::string encrypt(const std::string& data, const std::string& key);
    static std::string decrypt(const std::string& data, const std::string& key);
    static std::string hashPassword(const std::string& password);
    static std::string generateKey(const std::string& password);
    
private:
    static void xorCipher(std::vector<unsigned char>& data, const std::string& key);
    static std::string toHex(const std::vector<unsigned char>& data);
    static std::vector<unsigned char> fromHex(const std::string& hex);
};
