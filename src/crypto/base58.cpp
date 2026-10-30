#include "crypto/base58.h"
#include "crypto/sha256.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <sstream>
#include <iomanip>

namespace BitMundi {

static const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

static const int8_t b58digits[] = {
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1, 0, 1, 2, 3, 4, 5, 6,  7, 8,-1,-1,-1,-1,-1,-1,
    -1, 9,10,11,12,13,14,15, 16,-1,17,18,19,20,21,-1,
    22,23,24,25,26,27,28,29, 30,31,32,-1,-1,-1,-1,-1,
    -1,33,34,35,36,37,38,39, 40,41,42,43,-1,44,45,46,
    47,48,49,50,51,52,53,54, 55,56,57,-1,-1,-1,-1,-1,
};

std::string EncodeBase58(const unsigned char* pbegin, const unsigned char* pend) {
    int zeroes = 0;
    int length = 0;
    while (pbegin != pend && *pbegin == 0) {
        pbegin++;
        zeroes++;
    }
    int size = static_cast<int>((pend - pbegin) * 138 / 100 + 1);
    std::vector<unsigned char> b58(size);
    while (pbegin != pend) {
        int carry = *pbegin;
        int i = 0;
        for (auto it = b58.rbegin(); (carry != 0 || i < length) && (it != b58.rend()); ++it, ++i) {
            carry += 256 * (*it);
            *it = carry % 58;
            carry /= 58;
        }
        length = i;
        pbegin++;
    }
    auto it = b58.begin() + (size - length);
    while (it != b58.end() && *it == 0)
        it++;
    std::string str;
    str.reserve(zeroes + (b58.end() - it));
    str.assign(zeroes, '1');
    while (it != b58.end())
        str += pszBase58[*(it++)];
    return str;
}

std::string EncodeBase58(const std::vector<uint8_t>& vch) {
    return EncodeBase58(vch.data(), vch.data() + vch.size());
}

bool DecodeBase58(const std::string& str, std::vector<uint8_t>& vchRet) {
    const char* psz = str.c_str();
    while (*psz && isspace(*psz)) psz++;
    int zeroes = 0;
    int length = 0;
    while (*psz == '1') {
        zeroes++;
        psz++;
    }
    int size = static_cast<int>(strlen(psz) * 733 / 1000 + 1);
    std::vector<unsigned char> b256(size);
    while (*psz && !isspace(*psz)) {
        unsigned char ch = static_cast<unsigned char>(*psz);
        if (ch >= 128 || b58digits[ch] == -1)
            return false;
        int carry = b58digits[ch];
        int i = 0;
        for (auto it = b256.rbegin(); (carry != 0 || i < length) && (it != b256.rend()); ++it, ++i) {
            carry += 58 * (*it);
            *it = carry % 256;
            carry /= 256;
        }
        length = i;
        psz++;
    }
    while (isspace(*psz)) psz++;
    if (*psz != 0) return false;
    auto it = b256.begin() + (size - length);
    while (it != b256.end() && *it == 0)
        it++;
    vchRet.clear();
    vchRet.reserve(zeroes + (b256.end() - it));
    vchRet.assign(zeroes, 0x00);
    while (it != b256.end())
        vchRet.push_back(*(it++));
    return true;
}

std::string EncodeBase58Check(const std::vector<uint8_t>& vchIn) {
    std::vector<uint8_t> vch(vchIn);
    uint256 hash = Hash256(vch.begin(), vch.end());
    vch.insert(vch.end(), (const uint8_t*)&hash, (const uint8_t*)&hash + 4);
    return EncodeBase58(vch);
}

bool DecodeBase58Check(const std::string& str, std::vector<uint8_t>& vchRet) {
    if (!DecodeBase58(str, vchRet) || (vchRet.size() < 4)) {
        vchRet.clear();
        return false;
    }
    uint256 hash = Hash256(vchRet.begin(), vchRet.end() - 4);
    if (memcmp(&hash, &vchRet[vchRet.size() - 4], 4) != 0) {
        vchRet.clear();
        return false;
    }
    vchRet.resize(vchRet.size() - 4);
    return true;
}

static std::vector<uint8_t> HexToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        uint8_t byte = (uint8_t)strtol(byteString.c_str(), nullptr, 16);
        bytes.push_back(byte);
    }
    return bytes;
}

static std::string BytesToHex(const uint8_t* data, size_t len) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i) {
        ss << std::setw(2) << static_cast<int>(data[i]);
    }
    return ss.str();
}

std::string HexToWif(const std::string& hexKey, bool compressed) {
    std::vector<uint8_t> raw = HexToBytes(hexKey);
    std::vector<uint8_t> extended;
    extended.push_back(0x80);
    extended.insert(extended.end(), raw.begin(), raw.end());
    if (compressed) {
        extended.push_back(0x01);
    }
    return EncodeBase58Check(extended);
}

std::string ParsePrivkeyInput(const std::string& keyStr) {
    std::string s = keyStr;
    // trim
    s.erase(0, s.find_first_not_of(" \t\n\r"));
    s.erase(s.find_last_not_of(" \t\n\r") + 1);

    if (s.length() == 64) {
        bool isHex = true;
        for (char c : s) {
            if (!isxdigit(c)) { isHex = false; break; }
        }
        if (isHex) {
            std::string lower = s;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
            return lower;
        }
    }

    std::vector<uint8_t> decoded;
    if (!DecodeBase58Check(s, decoded)) {
        throw std::runtime_error("Invalid private key format: must be 64-char HEX or valid WIF!");
    }

    if (decoded.size() < 33 || decoded[0] != 0x80) {
        throw std::runtime_error("Invalid WIF key header!");
    }

    std::vector<uint8_t> keyBytes(decoded.begin() + 1, decoded.end());
    if (keyBytes.size() == 33 && keyBytes[32] == 0x01) {
        keyBytes.pop_back();
    }

    if (keyBytes.size() != 32) {
        throw std::runtime_error("Decoded key is not 32 bytes!");
    }

    return BytesToHex(keyBytes.data(), keyBytes.size());
}

} // namespace BitMundi
