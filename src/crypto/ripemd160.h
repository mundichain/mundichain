#ifndef MUNDICHAIN_CRYPTO_RIPEMD160_H
#define MUNDICHAIN_CRYPTO_RIPEMD160_H

#include <cstdint>
#include <vector>
#include "uint256.h"

class CRIPEMD160
{
private:
    uint32_t s[5];
    uint8_t buf[64];
    uint64_t bytes;

public:
    static const size_t OUTPUT_SIZE = 20;

    CRIPEMD160();
    CRIPEMD160& Write(const uint8_t* data, size_t len);
    void Finalize(uint8_t hash[OUTPUT_SIZE]);
    CRIPEMD160& Reset();
};

/** Hash160(x) = RIPEMD160(SHA256(x)) (Bitcoin Standard for PubKey Hashes) */
uint160 Hash160(const uint8_t* data, size_t len);
uint160 Hash160(const std::vector<uint8_t>& data);

#endif // MUNDICHAIN_CRYPTO_RIPEMD160_H
