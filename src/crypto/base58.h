#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace BitMundi {

std::string EncodeBase58(const unsigned char* pbegin, const unsigned char* pend);
std::string EncodeBase58(const std::vector<uint8_t>& vch);
bool DecodeBase58(const std::string& str, std::vector<uint8_t>& vchRet);

std::string EncodeBase58Check(const std::vector<uint8_t>& vchIn);
bool DecodeBase58Check(const std::string& str, std::vector<uint8_t>& vchRet);

std::string HexToWif(const std::string& hexKey, bool compressed = true);
std::string ParsePrivkeyInput(const std::string& keyStr);

} // namespace BitMundi
