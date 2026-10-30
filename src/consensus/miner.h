#pragma once
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include "consensus/blockchain.h"
#include "crypto/json.hpp"

namespace BitMundi {

class Miner {
public:
    Miner(Blockchain& blockchain);
    ~Miner();

    void StartMining(int threads = 1);
    void StopMining();
    bool IsMining() const { return m_active; }

    nlohmann::json GetMiningInfo() const;
    int64_t GetHashrate() const { return m_hashrate.load(); }
    int64_t GetTotalMinedBlocks() const { return m_totalMinedBlocks.load(); }

private:
    void MiningSupervisorLoop();
    void WorkerLoop(int workerId, int numThreads, const std::string& prefixStr, uint32_t bitsInt, int32_t height, const std::string& prevHash, const std::string& merkleRoot, int64_t blockTime, const std::string& bitsHex, const std::string& minerAddr, const std::vector<nlohmann::json>& blockTxs);

    Blockchain& m_blockchain;
    std::atomic<bool> m_active{false};
    std::atomic<bool> m_stopRound{false};
    std::atomic<int> m_threads{1};
    std::atomic<int64_t> m_hashrate{0};
    std::atomic<int64_t> m_totalMinedBlocks{0};
    std::atomic<uint64_t> m_totalHashesDone{0};

    std::thread m_supervisorThread;
    std::vector<std::thread> m_workerThreads;
};

} // namespace BitMundi
