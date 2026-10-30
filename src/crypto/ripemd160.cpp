#include "ripemd160.h"
#include "sha256.h"
#include <cstring>

inline uint32_t ROL(uint32_t x, int n) { return (x << n) | (x >> (32 - n)); }

inline uint32_t f1(uint32_t x, uint32_t y, uint32_t z) { return x ^ y ^ z; }
inline uint32_t f2(uint32_t x, uint32_t y, uint32_t z) { return (x & y) | (~x & z); }
inline uint32_t f3(uint32_t x, uint32_t y, uint32_t z) { return (x | ~y) ^ z; }
inline uint32_t f4(uint32_t x, uint32_t y, uint32_t z) { return (x & z) | (y & ~z); }
inline uint32_t f5(uint32_t x, uint32_t y, uint32_t z) { return x ^ (y | ~z); }

CRIPEMD160::CRIPEMD160() { Reset(); }

CRIPEMD160& CRIPEMD160::Reset() {
    s[0] = 0x67452301; s[1] = 0xefcdab89; s[2] = 0x98badcfe; s[3] = 0x10325476; s[4] = 0xc3d2e1f0;
    bytes = 0;
    return *this;
}

static void TransformR160(uint32_t s[5], const uint8_t chunk[64]) {
    uint32_t x[16];
    for (int i = 0; i < 16; i++) {
        x[i] = ((uint32_t)chunk[i*4]) |
               (((uint32_t)chunk[i*4+1]) << 8) |
               (((uint32_t)chunk[i*4+2]) << 16) |
               (((uint32_t)chunk[i*4+3]) << 24);
    }
    uint32_t a1 = s[0], b1 = s[1], c1 = s[2], d1 = s[3], e1 = s[4];
    uint32_t a2 = s[0], b2 = s[1], c2 = s[2], d2 = s[3], e2 = s[4];

    // Left line
    #define R_L(a, b, c, d, e, f, k, x_idx, s_amt)         a = ROL(a + f(b, c, d) + x[x_idx] + k, s_amt) + e; c = ROL(c, 10);

    // Round 1
    R_L(a1, b1, c1, d1, e1, f1, 0x00000000, 0, 11);
    R_L(e1, a1, b1, c1, d1, f1, 0x00000000, 1, 14);
    R_L(d1, e1, a1, b1, c1, f1, 0x00000000, 2, 15);
    R_L(c1, d1, e1, a1, b1, f1, 0x00000000, 3, 12);
    R_L(b1, c1, d1, e1, a1, f1, 0x00000000, 4, 5);
    R_L(a1, b1, c1, d1, e1, f1, 0x00000000, 5, 8);
    R_L(e1, a1, b1, c1, d1, f1, 0x00000000, 6, 7);
    R_L(d1, e1, a1, b1, c1, f1, 0x00000000, 7, 9);
    R_L(c1, d1, e1, a1, b1, f1, 0x00000000, 8, 11);
    R_L(b1, c1, d1, e1, a1, f1, 0x00000000, 9, 13);
    R_L(a1, b1, c1, d1, e1, f1, 0x00000000, 10, 14);
    R_L(e1, a1, b1, c1, d1, f1, 0x00000000, 11, 15);
    R_L(d1, e1, a1, b1, c1, f1, 0x00000000, 12, 6);
    R_L(c1, d1, e1, a1, b1, f1, 0x00000000, 13, 7);
    R_L(b1, c1, d1, e1, a1, f1, 0x00000000, 14, 9);
    R_L(a1, b1, c1, d1, e1, f1, 0x00000000, 15, 8);

    // Combine
    uint32_t t = s[1] + c1 + d2;
    s[1] = s[2] + d1 + e2;
    s[2] = s[3] + e1 + a2;
    s[3] = s[4] + a1 + b2;
    s[4] = s[0] + b1 + c2;
    s[0] = t;
}

CRIPEMD160& CRIPEMD160::Write(const uint8_t* data, size_t len) {
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
            TransformR160(s, buf);
            buf_fill = 0;
        }
    }
    return *this;
}

void CRIPEMD160::Finalize(uint8_t hash[OUTPUT_SIZE]) {
    uint8_t pad[64] = {0};
    pad[0] = 0x80;
    uint64_t bit_len = bytes * 8;
    uint8_t len_bytes[8];
    for (int i = 0; i < 8; i++) {
        len_bytes[i] = (uint8_t)(bit_len >> (i * 8));
    }
    size_t pad_len = (bytes % 64 < 56) ? (56 - (bytes % 64)) : (120 - (bytes % 64));
    Write(pad, pad_len);
    Write(len_bytes, 8);
    for (int i = 0; i < 5; i++) {
        hash[i * 4] = (uint8_t)(s[i]);
        hash[i * 4 + 1] = (uint8_t)(s[i] >> 8);
        hash[i * 4 + 2] = (uint8_t)(s[i] >> 16);
        hash[i * 4 + 3] = (uint8_t)(s[i] >> 24);
    }
}

uint160 Hash160(const uint8_t* data, size_t len) {
    uint8_t h_sha[32];
    uint8_t h_ripemd[20];
    CSHA256().Write(data, len).Finalize(h_sha);
    CRIPEMD160().Write(h_sha, 32).Finalize(h_ripemd);
    return uint160(h_ripemd);
}

uint160 Hash160(const std::vector<uint8_t>& data) {
    return Hash160(data.data(), data.size());
}
