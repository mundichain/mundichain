#include "consensus/miner.h"
#include "crypto/sha256.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <random>

namespace BitMundi {

static std::string MakeRandomHex(size_t bytes) {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint32_t> dis(0, 255);
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < bytes; ++i) {
        ss << std::setw(2) << dis(gen);
    }
    return ss.str();
}

Miner::Miner(Blockchain& blockchain)
    : m_blockchain(blockchain) {
}

Miner::~Miner() {
    StopMining();
}

void Miner::StartMining(int threads) {
    if (m_active) {
        StopMining();
    }
    m_threads = std::max(1, threads);
    m_active = true;
    m_stopRound = false;

    m_supervisorThread = std::thread(&Miner::MiningSupervisorLoop, this);
}

void Miner::StopMining() {
    m_active = false;
    m_stopRound = true;

    for (auto& t : m_workerThreads) {
        if (t.joinable()) t.join();
    }
    m_workerThreads.clear();

    if (m_supervisorThread.joinable()) {
        m_supervisorThread.join();
    }
    m_hashrate = 0;
}

void Miner::MiningSupervisorLoop() {
    while (m_active) {
        int32_t height;
        std::string prevHash;
        int64_t subsidy;
        std::string minerAddr;
        int64_t blockTime;
        uint32_t bitsInt;
        std::string bitsHex;
        std::string merkleRoot;
        std::vector<nlohmann::json> blockTxs;
        std::string prefixStr;

        {
            std::lock_guard<std::mutex> lock(m_blockchain.GetMutex());
            height = static_cast<int32_t>(m_blockchain.GetBlocks().size());
            prevHash = m_blockchain.GetBlocks().back().value("hash", "");
            subsidy = Blockchain::GetBlockSubsidy(height);
            minerAddr = m_blockchain.GetWallet().GetPrimaryAddress();

            blockTime = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();

            int64_t totalFees = 0;
            std::vector<nlohmann::json> sortedMempool = m_blockchain.GetMempool();
            std::sort(sortedMempool.begin(), sortedMempool.end(), [](const nlohmann::json& a, const nlohmann::json& b) {
                return a.value("fee", (int64_t)0) > b.value("fee", (int64_t)0);
            });

            for (const auto& tx : sortedMempool) {
                totalFees += tx.value("fee", (int64_t)0);
            }

            nlohmann::json coinbaseTx = {
                {"txid", MakeRandomHex(32)},
                {"is_coinbase", true},
                {"time", blockTime},
                {"vin", nlohmann::json::array({{{"coinbase", "Mined by MundiChain Multi-Core C++ Native Miner Block #" + std::to_string(height)}}})},
                {"vout", nlohmann::json::array({{{"value", subsidy + totalFees}, {"address", minerAddr}}})}
            };

            blockTxs.push_back(coinbaseTx);
            for (const auto& tx : sortedMempool) {
                blockTxs.push_back(tx);
            }

            std::string txHashes;
            for (const auto& tx : blockTxs) {
                txHashes += tx.value("txid", "");
            }
            uint256 merkle = Hash256(txHashes.begin(), txHashes.end());
            merkleRoot = merkle.GetHex();

            bitsInt = m_blockchain.GetNextWorkRequired();
            std::stringstream bitsStream;
            bitsStream << "0x" << std::hex << bitsInt;
            bitsHex = bitsStream.str();

            prefixStr = std::to_string(height) + prevHash + merkleRoot + std::to_string(blockTime) + bitsHex;
        }

        m_stopRound = false;
        m_workerThreads.clear();
        int numThreads = m_threads.load();
        auto t0 = std::chrono::steady_clock::now();
        uint64_t startHashes = m_totalHashesDone.load();

        for (int wid = 0; wid < numThreads; ++wid) {
            m_workerThreads.emplace_back(&Miner::WorkerLoop, this, wid, numThreads,
                prefixStr, bitsInt, height, prevHash, merkleRoot, blockTime, bitsHex, minerAddr, blockTxs);
        }

        // Wait for workers to finish current block or stop
        while (m_active && !m_stopRound) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            auto curTime = std::chrono::steady_clock::now();
            double elapsed = std::max(0.1, std::chrono::duration<double>(curTime - t0).count());
            uint64_t done = m_totalHashesDone.load() - startHashes;
            m_hashrate = static_cast<int64_t>(done / elapsed);
        }

        m_stopRound = true;
        for (auto& t : m_workerThreads) {
            if (t.joinable()) t.join();
        }
        m_workerThreads.clear();

        if (!m_active) break;
    }
}

void Miner::WorkerLoop(int workerId, int numThreads, const std::string& prefixStr, uint32_t bitsInt,
    int32_t height, const std::string& prevHash, const std::string& merkleRoot, int64_t blockTime,
    const std::string& bitsHex, const std::string& minerAddr, const std::vector<nlohmann::json>& blockTxs) {

    uint64_t nonce = workerId;
    uint64_t localBatch = 0;

    while (!m_stopRound && m_active) {
        std::string s = prefixStr + std::to_string(nonce);
        uint256 h = Hash256(s.begin(), s.end());
        std::string hStr = h.GetHex();

        // Target match condition
        if (hStr.substr(0, 5) == "00000" || (hStr.substr(0, 4) == "0000" && hStr[4] <= 'f')) {
            // Winner!
            m_stopRound = true;
            m_totalHashesDone += localBatch;

            nlohmann::json newBlock = {
                {"height", height},
                {"hash", hStr},
                {"prev_hash", prevHash},
                {"merkle_root", merkleRoot},
                {"time", blockTime},
                {"bits", bitsHex},
                {"nonce", nonce},
                {"miner", minerAddr},
                {"tx", blockTxs}
            };

            m_blockchain.AddMinedBlock(newBlock);
            m_totalMinedBlocks++;
            std::cout << "[NATIVE MINER] Block #" << height << " mined! Hash: " << hStr.substr(0, 24) << "... Nonce: " << nonce << std::endl;
            return;
        }

        nonce += numThreads;
        localBatch++;
        if (localBatch >= 50000) {
            m_totalHashesDone += localBatch;
            localBatch = 0;
        }
    }
    m_totalHashesDone += localBatch;
}

nlohmann::json Miner::GetMiningInfo() const {
    uint32_t lastBits = Consensus::Params::POW_LIMIT_BITS;
    if (m_blockchain.GetBlockCount() > 0) {
        std::string bStr = m_blockchain.GetBlocks().back().value("bits", "0x1e0ffff0");
        lastBits = static_cast<uint32_t>(std::stoul(bStr, nullptr, 16));
    }
    double diff = Blockchain::CalculateDifficulty(lastBits);

    nlohmann::json info;
    info["active"] = m_active.load();
    info["threads"] = m_threads.load();
    info["hashrate"] = m_active ? m_hashrate.load() : 0;
    info["difficulty"] = diff;
    info["target_block_time"] = Consensus::Params::TARGET_BLOCK_TIME;
    info["retarget_interval"] = Consensus::Params::RETARGET_INTERVAL;
    info["blocks_mined"] = m_totalMinedBlocks.load();
    info["mempool"] = m_blockchain.GetMempoolCount();
    return info;
}

} // namespace BitMundi
