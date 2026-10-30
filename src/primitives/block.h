#ifndef MUNDICHAIN_PRIMITIVES_BLOCK_H
#define MUNDICHAIN_PRIMITIVES_BLOCK_H

#include <cstdint>
#include <vector>
#include <string>
#include "../crypto/uint256.h"
#include "../crypto/sha256.h"

class CTxIn {
public:
    uint256 prevout_hash;
    uint32_t prevout_n;
    std::string signature;
    std::string pubkey;

    CTxIn() : prevout_n(0) {}
};

class CTxOut {
public:
    int64_t nValue;
    std::string address;

    CTxOut() : nValue(0) {}
    CTxOut(int64_t val, const std::string& addr) : nValue(val), address(addr) {}
};

class CTransaction {
public:
    int32_t nVersion;
    std::vector<CTxIn> vin;
    std::vector<CTxOut> vout;
    uint32_t nLockTime;
    bool is_coinbase;

    CTransaction() : nVersion(1), nLockTime(0), is_coinbase(false) {}

    uint256 GetHash() const {
        std::ostringstream ss;
        ss << nVersion << vin.size() << vout.size() << nLockTime << is_coinbase;
        for (const auto& in : vin) ss << in.prevout_hash.ToString() << in.prevout_n;
        for (const auto& out : vout) ss << out.nValue << out.address;
        return Hash256(ss.str());
    }
};

class CBlockHeader {
public:
    int32_t nVersion;
    uint256 hashPrevBlock;
    uint256 hashMerkleRoot;
    uint32_t nTime;
    uint32_t nBits;
    uint32_t nNonce;

    CBlockHeader() : nVersion(1), nTime(0), nBits(0x1e0ffff0), nNonce(0) {}

    uint256 GetHash() const {
        std::vector<uint8_t> bytes;
        bytes.resize(80);
        std::memcpy(bytes.data(), &nVersion, 4);
        std::memcpy(bytes.data() + 4, hashPrevBlock.data(), 32);
        std::memcpy(bytes.data() + 36, hashMerkleRoot.data(), 32);
        std::memcpy(bytes.data() + 68, &nTime, 4);
        std::memcpy(bytes.data() + 72, &nBits, 4);
        std::memcpy(bytes.data() + 76, &nNonce, 4);
        return Hash256(bytes);
    }
};

class CBlock : public CBlockHeader {
public:
    int64_t nHeight;
    std::vector<CTransaction> vtx;

    CBlock() : nHeight(0) {}

    uint256 BuildMerkleTree() {
        if (vtx.empty()) return uint256();
        std::vector<uint256> hashes;
        for (const auto& tx : vtx) hashes.push_back(tx.GetHash());
        while (hashes.size() > 1) {
            if (hashes.size() % 2 != 0) hashes.push_back(hashes.back());
            std::vector<uint256> new_hashes;
            for (size_t i = 0; i < hashes.size(); i += 2) {
                std::vector<uint8_t> pair(64);
                std::memcpy(pair.data(), hashes[i].data(), 32);
                std::memcpy(pair.data() + 32, hashes[i+1].data(), 32);
                new_hashes.push_back(Hash256(pair));
            }
            hashes = new_hashes;
        }
        hashMerkleRoot = hashes[0];
        return hashMerkleRoot;
    }
};

#endif // MUNDICHAIN_PRIMITIVES_BLOCK_H
