#include "bech32.h"

namespace bech32 {

static const char* CHARSET = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

static const int8_t CHARSET_REV[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    15, -1, 10, 17, 21, 20, 26, 30,  7,  5, -1, -1, -1, -1, -1, -1,
    -1, 29, -1, 24, 13, 25,  9,  8, 23, -1, 18, 22, 31, 27, 19, -1,
     1,  0,  3, 16, 11, 28, 12, 14,  6,  4,  2, -1, -1, -1, -1, -1,
    -1, 29, -1, 24, 13, 25,  9,  8, 23, -1, 18, 22, 31, 27, 19, -1,
     1,  0,  3, 16, 11, 28, 12, 14,  6,  4,  2, -1, -1, -1, -1, -1
};

uint32_t PolyMod(const std::vector<uint8_t>& values) {
    uint32_t chk = 1;
    for (size_t i = 0; i < values.size(); ++i) {
        uint8_t top = chk >> 25;
        chk = (chk & 0x1ffffff) << 5 ^ values[i] ^
            (-((top >> 0) & 1) & 0x3b6a57b2UL) ^
            (-((top >> 1) & 1) & 0x26508e6dUL) ^
            (-((top >> 2) & 1) & 0x1ea119faUL) ^
            (-((top >> 3) & 1) & 0x3d4233ddUL) ^
            (-((top >> 4) & 1) & 0x2a1462b3UL);
    }
    return chk;
}

std::vector<uint8_t> HrpExpand(const std::string& hrp) {
    std::vector<uint8_t> ret;
    ret.reserve(hrp.size() * 2 + 1);
    for (size_t i = 0; i < hrp.size(); ++i) ret.push_back(hrp[i] >> 5);
    ret.push_back(0);
    for (size_t i = 0; i < hrp.size(); ++i) ret.push_back(hrp[i] & 0x1f);
    return ret;
}

std::string Encode(const std::string& hrp, const std::vector<uint8_t>& values) {
    std::vector<uint8_t> enc = HrpExpand(hrp);
    enc.insert(enc.end(), values.begin(), values.end());
    enc.resize(enc.size() + 6);
    uint32_t mod = PolyMod(enc) ^ 1;
    for (size_t i = 0; i < 6; ++i) {
        enc[enc.size() - 6 + i] = (mod >> (5 * (5 - i))) & 31;
    }
    std::string ret = hrp + "1";
    ret.reserve(ret.size() + values.size() + 6);
    for (size_t i = 0; i < values.size() + 6; ++i) {
        ret += CHARSET[enc[enc.size() - values.size() - 6 + i]];
    }
    return ret;
}

bool ConvertBits(std::vector<uint8_t>& out, const std::vector<uint8_t>& in, int frombits, int tobits, bool pad) {
    int acc = 0;
    int bits = 0;
    int maxv = (1 << tobits) - 1;
    int max_acc = (1 << (frombits + tobits - 1)) - 1;
    for (size_t i = 0; i < in.size(); ++i) {
        int value = in[i];
        if (value < 0 || (value >> frombits) != 0) return false;
        acc = ((acc << frombits) | value) & max_acc;
        bits += frombits;
        while (bits >= tobits) {
            bits -= tobits;
            out.push_back((acc >> bits) & maxv);
        }
    }
    if (pad) {
        if (bits) out.push_back((acc << (tobits - bits)) & maxv);
    } else if (bits >= frombits || ((acc << (tobits - bits)) & maxv)) {
        return false;
    }
    return true;
}

std::string EncodeMundiAddress(const std::vector<uint8_t>& pubkey_hash) {
    std::vector<uint8_t> data;
    data.push_back(0); // witness version 0 (q)
    ConvertBits(data, pubkey_hash, 8, 5, true);
    return Encode("bmd", data);
}

} // namespace bech32
