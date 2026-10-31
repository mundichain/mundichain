#include "crypto/uint256.h"
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>
#include "crypto/json.hpp"
#include "wallet/wallet.h"
#include "consensus/params.h"

namespace BitMundi {

struct UTXOEntry {
    std::string address;
    int64_t amount;
    int32_t height;
    bool is_coinbase;
};

class Blockchain {
public:
    Blockchain(const std::string& dataDir);
    ~Blockchain() = default;

    void LoadOrInit();
    void InitGenesis(const std::string& minerAddress);
    void RebuildUTXO();
    void SaveChain();

    uint32_t GetNextWorkRequired() const;
    BalanceResult GetBalance(const std::string& address = "") const;

    std::string CreateTransaction(const std::string& fromAddr, const std::string& toAddr, double amountBmd, double feeBmd = 0.0001);
    nlohmann::json MineSingleBlockSync(const std::string& minerAddress = "");

    std::vector<nlohmann::json> ListTransactions(const std::unordered_map<std::string, AddressEntry>& myAddrs) const;

    size_t GetBlockCount() const;
    nlohmann::json GetBlock(size_t index) const;
    nlohmann::json GetBlockByHash(const std::string& hash) const;

    const std::vector<nlohmann::json>& GetBlocks() const { return m_blocks; }
    size_t GetMempoolCount() const;
    const std::vector<nlohmann::json>& GetMempool() const { return m_mempool; }
    std::vector<nlohmann::json>& GetMempool() { return m_mempool; }

    CWallet& GetWallet() { return m_wallet; }
    const CWallet& GetWallet() const { return m_wallet; }

    std::string GetDataDir() const { return m_dataDir; }

    static double CalculateDifficulty(uint32_t bits);
    static int64_t GetBlockSubsidy(int32_t height);
    static uint256 BitsToTarget(uint32_t bits);
    static uint32_t TargetToBits(const uint256& target);
    static std::string BitsToTargetHex(uint32_t bits);
    static uint32_t DifficultyToBits(double diff);

    std::mutex& GetMutex() { return m_mutex; }

    // Direct block insertion from miner
    void AddMinedBlock(const nlohmann::json& block);

private:
    std::string m_dataDir;
    std::string m_chainFile;
    CWallet m_wallet;

    std::vector<nlohmann::json> m_blocks;
    std::unordered_map<std::string, UTXOEntry> m_utxo;
    std::vector<nlohmann::json> m_mempool;

    mutable std::mutex m_mutex;
};

} // namespace BitMundi
