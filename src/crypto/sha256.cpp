#include "sha256.h"
#include <cstring>

inline uint32_t ROR32(uint32_t val, int shift) {
    return (val >> shift) | (val << (32 - shift));
}

inline uint32_t Ch(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (~x & z); }
inline uint32_t Maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
inline uint32_t Sigma0(uint32_t x) { return ROR32(x, 2) ^ ROR32(x, 13) ^ ROR32(x, 22); }
inline uint32_t Sigma1(uint32_t x) { return ROR32(x, 6) ^ ROR32(x, 11) ^ ROR32(x, 25); }
inline uint32_t sigma0(uint32_t x) { return ROR32(x, 7) ^ ROR32(x, 18) ^ (x >> 3); }
inline uint32_t sigma1(uint32_t x) { return ROR32(x, 17) ^ ROR32(x, 19) ^ (x >> 10); }

static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static void Transform(uint32_t s[8], const uint8_t chunk[64]) {
    uint32_t w[64];
    for (int i = 0; i < 16; i++) {
        w[i] = ((uint32_t)chunk[i * 4] << 24) |
               ((uint32_t)chunk[i * 4 + 1] << 16) |
               ((uint32_t)chunk[i * 4 + 2] << 8) |
               ((uint32_t)chunk[i * 4 + 3]);
    }
    for (int i = 16; i < 64; i++) {
        w[i] = sigma1(w[i - 2]) + w[i - 7] + sigma0(w[i - 15]) + w[i - 16];
    }
    uint32_t a = s[0], b = s[1], c = s[2], d = s[3], e = s[4], f = s[5], g = s[6], h = s[7];
    for (int i = 0; i < 64; i++) {
        uint32_t t1 = h + Sigma1(e) + Ch(e, f, g) + K[i] + w[i];
        uint32_t t2 = Sigma0(a) + Maj(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    s[0] += a; s[1] += b; s[2] += c; s[3] += d;
    s[4] += e; s[5] += f; s[6] += g; s[7] += h;
}

CSHA256::CSHA256() { Reset(); }

CSHA256& CSHA256::Reset() {
    s[0] = 0x6a09e667; s[1] = 0xbb67ae85; s[2] = 0x3c6ef372; s[3] = 0xa54ff53a;
    s[4] = 0x510e527f; s[5] = 0x9b05688c; s[6] = 0x1f83d9ab; s[7] = 0x5be0cd19;
    bytes = 0;
    return *this;
}

CSHA256& CSHA256::Write(const uint8_t* data, size_t len) {
    size_t buf_fill = bytes % 64;
    bytes += len;
    while (len > 0) {
        size_t to_copy = 64 - buf_fill;
        if (len < to_copy) to_copy = len;
        std::memcpy(buf + buf_fill, data, to_copy);
        buf_fill += to_copy;
        data += to_copy;
        len -= to_copy;
        if (buf_fill == 64) {
            Transform(s, buf);
            buf_fill = 0;
        }
    }
    return *this;
}

void CSHA256::Finalize(uint8_t hash[OUTPUT_SIZE]) {
    uint8_t pad[64] = {0};
    pad[0] = 0x80;
    uint64_t bit_len = bytes * 8;
    uint8_t len_bytes[8];
    for (int i = 0; i < 8; i++) {
        len_bytes[7 - i] = (uint8_t)(bit_len >> (i * 8));
    }
    size_t pad_len = (bytes % 64 < 56) ? (56 - (bytes % 64)) : (120 - (bytes % 64));
    Write(pad, pad_len);
    Write(len_bytes, 8);
    for (int i = 0; i < 8; i++) {
        hash[i * 4] = (uint8_t)(s[i] >> 24);
        hash[i * 4 + 1] = (uint8_t)(s[i] >> 16);
        hash[i * 4 + 2] = (uint8_t)(s[i] >> 8);
        hash[i * 4 + 3] = (uint8_t)(s[i]);
    }
}

uint256 Hash256(const uint8_t* data, size_t len) {
    uint8_t h1[32];
    uint8_t h2[32];
    CSHA256().Write(data, len).Finalize(h1);
    CSHA256().Write(h1, 32).Finalize(h2);
    return uint256(h2);
}

uint256 Hash256(const std::vector<uint8_t>& data) {
    return Hash256(data.data(), data.size());
}

uint256 Hash256(const std::string& str) {
    return Hash256((const uint8_t*)str.data(), str.size());
}
