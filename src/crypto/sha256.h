#ifndef MUNDICHAIN_CRYPTO_SHA256_H
#define MUNDICHAIN_CRYPTO_SHA256_H

#include <cstdint>
#include <string>
#include <vector>
#include <iterator>
#include "uint256.h"

class CSHA256
{
private:
    uint32_t s[8];
    uint8_t buf[64];
    uint64_t bytes;

public:
    static const size_t OUTPUT_SIZE = 32;

    CSHA256();
    CSHA256& Write(const uint8_t* data, size_t len);
    void Finalize(uint8_t hash[OUTPUT_SIZE]);
    CSHA256& Reset();
};

/** Double SHA-256 Hash256(x) = SHA256(SHA256(x)) (Bitcoin Standard) */
uint256 Hash256(const uint8_t* data, size_t len);
uint256 Hash256(const std::vector<uint8_t>& data);
uint256 Hash256(const std::string& str);

template<typename TIterator>
inline uint256 Hash256(const TIterator pbegin, const TIterator pend) {
    if (pbegin == pend) return Hash256(nullptr, 0);
    return Hash256(reinterpret_cast<const uint8_t*>(&pbegin[0]), (pend - pbegin) * sizeof(pbegin[0]));
}

#endif // MUNDICHAIN_CRYPTO_SHA256_H
