#include "consensus/blockchain.h"
#include "crypto/sha256.h"
#include "crypto/ripemd160.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <algorithm>
#include <random>
#include <unordered_set>

namespace BitMundi {

static std::string RandomHex(size_t bytes) {
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

uint256 Blockchain::BitsToTarget(uint32_t bits) {
    uint32_t exponent = bits >> 24;
    uint32_t mantissa = bits & 0x007fffff;
    if (exponent <= 3) {
        return uint256(mantissa >> (8 * (3 - exponent)));
    } else {
        // Shift left
        std::vector<uint8_t> bytes(32, 0);
        int shift = exponent - 3;
        if (shift >= 0 && shift <= 29) {
            bytes[31 - shift] = (mantissa >> 16) & 0xFF;
            bytes[30 - shift] = (mantissa >> 8) & 0xFF;
            bytes[29 - shift] = mantissa & 0xFF;
        }
        // Build uint256 from big-endian bytes
        uint256 target;
        for (int i = 0; i < 32; ++i) {
            ((uint8_t*)&target)[31 - i] = bytes[i];
        }
        return target;
    }
}

uint32_t Blockchain::TargetToBits(const uint256& target) {
    // Convert target to compact bits format
    const uint8_t* p = (const uint8_t*)&target;
    int size = 32;
    while (size > 0 && p[size - 1] == 0) {
        size--;
    }
    if (size == 0) return 0;

    uint32_t compact = 0;
    if (p[size - 1] >= 0x80) {
        compact |= (size + 1) << 24;
        compact |= (p[size - 1]) << 8;
        if (size >= 2) compact |= p[size - 2];
    } else {
        compact |= size << 24;
        compact |= (p[size - 1]) << 16;
        if (size >= 2) compact |= (p[size - 2]) << 8;
        if (size >= 3) compact |= p[size - 3];
    }
    return compact;
}

double Blockchain::CalculateDifficulty(uint32_t bits) {
    uint32_t powBits = Consensus::Params::POW_LIMIT_BITS;
    if (bits == 0) return 0.0;
    double maxTarget = static_cast<double>(powBits & 0x00ffffff) * std::pow(256.0, (powBits >> 24) - 3);
    double curTarget = static_cast<double>(bits & 0x00ffffff) * std::pow(256.0, (bits >> 24) - 3);
    if (curTarget <= 0.0) return 1.0;
    return std::max(1.0, std::round((maxTarget / curTarget) * 10000.0) / 10000.0);
}

int64_t Blockchain::GetBlockSubsidy(int32_t height) {
    int halvings = height / Consensus::Params::HALVING_INTERVAL;
    if (halvings >= 64) return 0;
    return Consensus::Params::INITIAL_SUBSIDY >> halvings;
}

Blockchain::Blockchain(const std::string& dataDir)
    : m_dataDir(dataDir), m_wallet(dataDir) {
    m_chainFile = (std::filesystem::path(dataDir) / "chain.json").string();
    std::filesystem::create_directories(dataDir);
}

void Blockchain::LoadOrInit() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_wallet.LoadOrInit();

    if (std::filesystem::exists(m_chainFile)) {
        try {
            std::ifstream file(m_chainFile);
            nlohmann::json j;
            file >> j;
            if (j.contains("blocks") && j["blocks"].is_array() && !j["blocks"].empty()) {
                m_blocks = j["blocks"].get<std::vector<nlohmann::json>>();
                RebuildUTXO();
                return;
            }
        } catch (...) {
            // fallback to genesis
        }
    }
    InitGenesis(m_wallet.GetPrimaryAddress());
}

void Blockchain::InitGenesis(const std::string& minerAddress) {
    std::string addr = minerAddress.empty() ? m_wallet.GetPrimaryAddress() : minerAddress;
    
    nlohmann::json genesisTx = {
        {"txid", "93bf979857d42ea40b49cbcfb5a37466ebc1c7ae9eb5d8c3fb9fceb20dd6f4ea"},
        {"is_coinbase", true},
        {"vin", nlohmann::json::array({{{"coinbase", Consensus::Params::GENESIS_HEADLINE}}})},
        {"vout", nlohmann::json::array({{{"value", Consensus::Params::INITIAL_SUBSIDY}, {"address", addr}}})}
    };

    std::stringstream bitsStream;
    bitsStream << "0x" << std::hex << Consensus::Params::GENESIS_BITS;

    nlohmann::json genesisBlock = {
        {"height", 0},
        {"hash", Consensus::Params::GENESIS_HASH},
        {"prev_hash", "0000000000000000000000000000000000000000000000000000000000000000"},
        {"merkle_root", Consensus::Params::GENESIS_MERKLE},
        {"time", Consensus::Params::GENESIS_TIME},
        {"bits", bitsStream.str()},
        {"nonce", Consensus::Params::GENESIS_NONCE},
        {"headline", Consensus::Params::GENESIS_HEADLINE},
        {"miner", addr},
        {"tx", nlohmann::json::array({genesisTx})}
    };

    m_blocks = {genesisBlock};
    RebuildUTXO();
    SaveChain();
}

void Blockchain::RebuildUTXO() {
    m_utxo.clear();
    for (const auto& blk : m_blocks) {
        int32_t h = blk.value("height", 0);
        if (!blk.contains("tx") || !blk["tx"].is_array()) continue;

        for (const auto& tx : blk["tx"]) {
            std::string txid = tx.value("txid", "");
            bool isCoinbase = tx.value("is_coinbase", false);

            if (!isCoinbase && tx.contains("vin") && tx["vin"].is_array()) {
                for (const auto& vin : tx["vin"]) {
                    std::string inTxid = vin.value("txid", "");
                    int vout = vin.value("vout", 0);
                    std::string key = inTxid + ":" + std::to_string(vout);
                    m_utxo.erase(key);
                }
            }

            if (tx.contains("vout") && tx["vout"].is_array()) {
                int voutIdx = 0;
                for (const auto& out : tx["vout"]) {
                    std::string key = txid + ":" + std::to_string(voutIdx);
                    UTXOEntry entry;
                    entry.address = out.value("address", "");
                    entry.amount = out.value("value", (int64_t)0);
                    entry.height = h;
                    entry.is_coinbase = isCoinbase;
                    m_utxo[key] = entry;
                    voutIdx++;
                }
            }
        }
    }
}

void Blockchain::SaveChain() {
    nlohmann::json j;
    j["blocks"] = m_blocks;
    std::ofstream file(m_chainFile);
    file << j.dump(2);
}

uint32_t Blockchain::GetNextWorkRequired() const {
    if (m_blocks.empty()) return Consensus::Params::POW_LIMIT_BITS;
    const auto& lastBlock = m_blocks.back();
    int32_t lastHeight = lastBlock.value("height", 0);

    if ((lastHeight + 1) % Consensus::Params::RETARGET_INTERVAL != 0) {
        std::string bitsStr = lastBlock.value("bits", "0x1e0ffff0");
        return static_cast<uint32_t>(std::stoul(bitsStr, nullptr, 16));
    }

    size_t firstIdx = m_blocks.size() - Consensus::Params::RETARGET_INTERVAL;
    const auto& firstBlock = m_blocks[firstIdx];

    int64_t actualTimespan = lastBlock.value("time", (int64_t)0) - firstBlock.value("time", (int64_t)0);
    int64_t targetTimespan = Consensus::Params::TARGET_TIMESPAN;

    if (actualTimespan < targetTimespan / 4) actualTimespan = targetTimespan / 4;
    if (actualTimespan > targetTimespan * 4) actualTimespan = targetTimespan * 4;

    std::string bitsStr = lastBlock.value("bits", "0x1e0ffff0");
    uint32_t lastBits = static_cast<uint32_t>(std::stoul(bitsStr, nullptr, 16));
    
    // Retarget
    double factor = static_cast<double>(actualTimespan) / static_cast<double>(targetTimespan);
    double curDiff = CalculateDifficulty(lastBits);
    double newDiff = std::max(1.0, curDiff / factor);

    if (newDiff <= 1.0) {
        return Consensus::Params::POW_LIMIT_BITS;
    }

    uint32_t mantissa = lastBits & 0x00ffffff;
    uint32_t exponent = lastBits >> 24;
    uint64_t scaledMantissa = static_cast<uint64_t>(mantissa * factor);
    while (scaledMantissa > 0x007fffff) {
        scaledMantissa >>= 8;
        exponent++;
    }
    return (exponent << 24) | static_cast<uint32_t>(scaledMantissa & 0x007fffff);
}

BalanceResult Blockchain::GetBalance(const std::string& address) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    int32_t currentHeight = static_cast<int32_t>(m_blocks.size()) - 1;
    int64_t mature = 0;
    int64_t immature = 0;

    const auto& addrs = m_wallet.GetAddressesMap();

    for (const auto& [key, utxo] : m_utxo) {
        bool matches = false;
        if (address.empty()) {
            matches = (addrs.find(utxo.address) != addrs.end());
        } else {
            matches = (utxo.address == address);
        }

        if (matches) {
            if (utxo.is_coinbase) {
                int32_t confs = currentHeight - utxo.height + 1;
                if (confs >= Consensus::Params::COINBASE_MATURITY) {
                    mature += utxo.amount;
                } else {
                    immature += utxo.amount;
                }
            } else {
                mature += utxo.amount;
            }
        }
    }

    double matureBmd = static_cast<double>(mature) / Consensus::Params::COIN;
    double immatureBmd = static_cast<double>(immature) / Consensus::Params::COIN;
    return BalanceResult{matureBmd, immatureBmd, matureBmd + immatureBmd};
}

std::string Blockchain::CreateTransaction(const std::string& fromAddr, const std::string& toAddr, double amountBmd, double feeBmd) {
    std::lock_guard<std::mutex> lock(m_mutex);
    int64_t amountSats = static_cast<int64_t>(std::round(amountBmd * Consensus::Params::COIN));
    int64_t feeSats = static_cast<int64_t>(std::round(feeBmd * Consensus::Params::COIN));
    int64_t needed = amountSats + feeSats;

    int32_t currentHeight = static_cast<int32_t>(m_blocks.size()) - 1;
    const auto& addrs = m_wallet.GetAddressesMap();

    std::string sender = fromAddr;
    if (sender.empty()) {
        sender = m_wallet.GetPrimaryAddress();
    }

    int64_t accumulated = 0;
    std::vector<nlohmann::json> selectedInputs;

    for (const auto& [k, v] : m_utxo) {
        bool matches = false;
        if (!fromAddr.empty()) {
            matches = (v.address == fromAddr);
        } else {
            matches = (addrs.find(v.address) != addrs.end());
        }

        if (matches) {
            if (v.is_coinbase && (currentHeight - v.height + 1) < Consensus::Params::COINBASE_MATURITY) {
                continue;
            }
            accumulated += v.amount;
            size_t colon = k.find(':');
            std::string txid = k.substr(0, colon);
            int vout = std::stoi(k.substr(colon + 1));

            selectedInputs.push_back({
                {"txid", txid},
                {"vout", vout},
                {"amount", v.amount},
                {"address", v.address}
            });

            if (accumulated >= needed) break;
        }
    }

    if (accumulated < needed) {
        std::stringstream ss;
        ss << "Insufficient mature funds in active wallet! Available: "
           << std::fixed << std::setprecision(8) << (static_cast<double>(accumulated) / Consensus::Params::COIN)
           << " BMD, Needed: " << (static_cast<double>(needed) / Consensus::Params::COIN) << " BMD";
        throw std::runtime_error(ss.str());
    }

    std::vector<nlohmann::json> vouts = {
        {{"value", amountSats}, {"address", toAddr}}
    };

    int64_t change = accumulated - needed;
    if (change > 0) {
        vouts.push_back({{"value", change}, {"address", sender}});
    }

    std::vector<std::string> usedAddrs;
    for (const auto& inp : selectedInputs) {
        std::string a = inp["address"];
        if (std::find(usedAddrs.begin(), usedAddrs.end(), a) == usedAddrs.end()) {
            usedAddrs.push_back(a);
        }
    }

    std::string txid = RandomHex(32);
    int64_t nowTime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    nlohmann::json txData = {
        {"txid", txid},
        {"is_coinbase", false},
        {"time", nowTime},
        {"from_addrs", usedAddrs},
        {"to_addr", toAddr},
        {"amount", amountSats},
        {"vin", selectedInputs},
        {"vout", vouts},
        {"fee", feeSats}
    };

    m_mempool.push_back(txData);
    return txid;
}

nlohmann::json Blockchain::MineSingleBlockSync(const std::string& minerAddress) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string miner = minerAddress.empty() ? m_wallet.GetPrimaryAddress() : minerAddress;

    int32_t height = static_cast<int32_t>(m_blocks.size());
    std::string prevHash = m_blocks.back().value("hash", "");
    int64_t subsidy = GetBlockSubsidy(height);
    int64_t nowTime = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    int64_t totalFees = 0;
    std::vector<nlohmann::json> sortedMempool = m_mempool;
    std::sort(sortedMempool.begin(), sortedMempool.end(), [](const nlohmann::json& a, const nlohmann::json& b) {
        return a.value("fee", (int64_t)0) > b.value("fee", (int64_t)0);
    });

    for (const auto& tx : sortedMempool) {
        totalFees += tx.value("fee", (int64_t)0);
    }

    nlohmann::json coinbaseTx = {
        {"txid", RandomHex(32)},
        {"is_coinbase", true},
        {"time", nowTime},
        {"vin", nlohmann::json::array({{{"coinbase", "Mined by MundiChain C++ Native Node Block #" + std::to_string(height)}}})},
        {"vout", nlohmann::json::array({{{"value", subsidy + totalFees}, {"address", miner}}})}
    };

    std::vector<nlohmann::json> blockTxs = {coinbaseTx};
    for (const auto& tx : sortedMempool) {
        blockTxs.push_back(tx);
    }
    m_mempool.clear();

    std::string txHashes;
    for (const auto& tx : blockTxs) {
        txHashes += tx.value("txid", "");
    }
    uint256 merkle = Hash256(txHashes.begin(), txHashes.end());
    std::string merkleRoot = merkle.GetHex();

    uint32_t bitsInt = GetNextWorkRequired();
    std::stringstream bitsStream;
    bitsStream << "0x" << std::hex << bitsInt;
    std::string bitsHex = bitsStream.str();

    std::string prefixStr = std::to_string(height) + prevHash + merkleRoot + std::to_string(nowTime) + bitsHex;

    // Proof of work
    uint32_t targetExponent = bitsInt >> 24;
    uint32_t targetMantissa = bitsInt & 0x007fffff;
    // Simple hex target comparison: 64 - 2 * (exponent - 3) leading zeroes
    // More precisely: check target prefix
    uint64_t nonce = 0;
    std::string winnerHash;

    while (true) {
        std::string s = prefixStr + std::to_string(nonce);
        uint256 h = Hash256(s.begin(), s.end());
        std::string hStr = h.GetHex();
        
        // Fast check: target 0x1e0ffff0 requires hash starting with "00000" or similar
        // Let's verify: 0x1e is 30 bytes, so top 2 bytes are 0 -> at least 4-5 hex zeros
        if (hStr.substr(0, 5) == "00000" || (hStr.substr(0, 4) == "0000" && hStr[4] <= 'f')) {
            // Check compact target:
            // bits = 0x1e0ffff0 -> target exponent = 30 -> 32 - 30 = 2 bytes = 4 hex zeros
            // If top 5 hex digits are "00000", int(h, 16) <= target_int is guaranteed for 0x1e0ffff0
            winnerHash = hStr;
            break;
        }
        nonce++;
    }

    nlohmann::json newBlock = {
        {"height", height},
        {"hash", winnerHash},
        {"prev_hash", prevHash},
        {"merkle_root", merkleRoot},
        {"time", nowTime},
        {"bits", bitsHex},
        {"nonce", nonce},
        {"miner", miner},
        {"tx", blockTxs}
    };

    m_blocks.push_back(newBlock);
    RebuildUTXO();
    SaveChain();
    return newBlock;
}

void Blockchain::AddMinedBlock(const nlohmann::json& block) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_blocks.push_back(block);

    if (block.contains("tx") && block["tx"].is_array()) {
        std::unordered_set<std::string> minedTxids;
        for (const auto& tx : block["tx"]) {
            minedTxids.insert(tx.value("txid", ""));
        }
        auto it = std::remove_if(m_mempool.begin(), m_mempool.end(), [&](const nlohmann::json& tx) {
            return minedTxids.count(tx.value("txid", "")) > 0;
        });
        m_mempool.erase(it, m_mempool.end());
    } else {
        m_mempool.clear();
    }

    RebuildUTXO();
    SaveChain();
}

std::vector<nlohmann::json> Blockchain::ListTransactions(const std::unordered_map<std::string, AddressEntry>& myAddrs) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<nlohmann::json> txs;
    int32_t currentHeight = static_cast<int32_t>(m_blocks.size()) - 1;

    // 1. Mempool
    for (auto it = m_mempool.rbegin(); it != m_mempool.rend(); ++it) {
        const auto& tx = *it;
        std::vector<std::string> fromAddrs = tx.value("from_addrs", std::vector<std::string>());
        bool isSent = false;
        for (const auto& a : fromAddrs) {
            if (myAddrs.find(a) != myAddrs.end()) { isSent = true; break; }
        }

        if (isSent) {
            std::string destAddr = tx.value("to_addr", "");
            double amount = - (static_cast<double>(tx.value("amount", (int64_t)0)) / Consensus::Params::COIN);
            double fee = static_cast<double>(tx.value("fee", (int64_t)0)) / Consensus::Params::COIN;
            txs.push_back({
                {"category", "send"},
                {"amount", amount},
                {"fee", fee},
                {"confirmations", 0},
                {"blockheight", -1},
                {"blockhash", "mempool"},
                {"txid", tx.value("txid", "")},
                {"time", tx.value("time", (int64_t)0)},
                {"status", "Pending (In Mempool)"},
                {"address", destAddr},
                {"to_address", destAddr},
                {"from_address", fromAddrs.empty() ? "" : fromAddrs[0]}
            });
        }

        if (tx.contains("vout") && tx["vout"].is_array()) {
            for (const auto& vout : tx["vout"]) {
                std::string outAddr = vout.value("address", "");
                if (myAddrs.find(outAddr) != myAddrs.end() && !isSent) {
                    double amount = static_cast<double>(vout.value("value", (int64_t)0)) / Consensus::Params::COIN;
                    txs.push_back({
                        {"category", "receive"},
                        {"amount", amount},
                        {"confirmations", 0},
                        {"blockheight", -1},
                        {"blockhash", "mempool"},
                        {"txid", tx.value("txid", "")},
                        {"time", tx.value("time", (int64_t)0)},
                        {"status", "Pending (In Mempool)"},
                        {"address", outAddr},
                        {"to_address", outAddr},
                        {"from_address", fromAddrs.empty() ? "" : fromAddrs[0]}
                    });
                }
            }
        }
    }

    // 2. Confirmed blocks
    for (auto bit = m_blocks.rbegin(); bit != m_blocks.rend(); ++bit) {
        const auto& blk = *bit;
        int32_t h = blk.value("height", 0);
        if (!blk.contains("tx") || !blk["tx"].is_array()) continue;

        for (const auto& tx : blk["tx"]) {
            bool isCoinbase = tx.value("is_coinbase", false);
            if (isCoinbase) {
                if (tx.contains("vout") && tx["vout"].is_array() && !tx["vout"].empty()) {
                    std::string outAddr = tx["vout"][0].value("address", "");
                    if (myAddrs.find(outAddr) != myAddrs.end()) {
                        int32_t confs = currentHeight - h + 1;
                        double amount = static_cast<double>(tx["vout"][0].value("value", (int64_t)0)) / Consensus::Params::COIN;
                        std::string status = (confs >= Consensus::Params::COINBASE_MATURITY)
                            ? "Confirmed"
                            : "Immature (" + std::to_string(confs) + "/" + std::to_string(Consensus::Params::COINBASE_MATURITY) + ")";
                        txs.push_back({
                            {"category", "generate"},
                            {"amount", amount},
                            {"confirmations", confs},
                            {"blockheight", h},
                            {"blockhash", blk.value("hash", "")},
                            {"txid", tx.value("txid", "")},
                            {"time", blk.value("time", (int64_t)0)},
                            {"status", status},
                            {"address", outAddr},
                            {"to_address", outAddr},
                            {"from_address", "Coinbase (Mining Reward)"}
                        });
                    }
                }
            } else {
                std::vector<std::string> fromAddrs = tx.value("from_addrs", std::vector<std::string>());
                if (fromAddrs.empty() && tx.contains("vin") && tx["vin"].is_array()) {
                    for (const auto& inp : tx["vin"]) {
                        std::string a = inp.value("address", "");
                        if (!a.empty()) fromAddrs.push_back(a);
                    }
                }
                bool isSent = false;
                for (const auto& a : fromAddrs) {
                    if (myAddrs.find(a) != myAddrs.end()) { isSent = true; break; }
                }
                int32_t confs = currentHeight - h + 1;

                if (isSent) {
                    std::string toAddr = tx.value("to_addr", "");
                    int64_t sentSats = tx.value("amount", (int64_t)0);
                    if (sentSats == 0 && tx.contains("vout") && tx["vout"].is_array() && !tx["vout"].empty()) {
                        sentSats = tx["vout"][0].value("value", (int64_t)0);
                    }
                    double amount = - (static_cast<double>(sentSats) / Consensus::Params::COIN);
                    double fee = static_cast<double>(tx.value("fee", (int64_t)0)) / Consensus::Params::COIN;
                    txs.push_back({
                        {"category", "send"},
                        {"amount", amount},
                        {"fee", fee},
                        {"confirmations", confs},
                        {"blockheight", h},
                        {"blockhash", blk.value("hash", "")},
                        {"txid", tx.value("txid", "")},
                        {"time", blk.value("time", (int64_t)0)},
                        {"status", "Confirmed"},
                        {"address", toAddr},
                        {"to_address", toAddr},
                        {"from_address", fromAddrs.empty() ? "" : fromAddrs[0]}
                    });
                }

                if (tx.contains("vout") && tx["vout"].is_array()) {
                    for (const auto& vout : tx["vout"]) {
                        std::string outAddr = vout.value("address", "");
                        if (myAddrs.find(outAddr) != myAddrs.end() && !isSent) {
                            double amount = static_cast<double>(vout.value("value", (int64_t)0)) / Consensus::Params::COIN;
                            txs.push_back({
                                {"category", "receive"},
                                {"amount", amount},
                                {"confirmations", confs},
                                {"blockheight", h},
                                {"blockhash", blk.value("hash", "")},
                                {"txid", tx.value("txid", "")},
                                {"time", blk.value("time", (int64_t)0)},
                                {"status", "Confirmed"},
                                {"address", outAddr},
                                {"to_address", outAddr},
                                {"from_address", fromAddrs.empty() ? "" : fromAddrs[0]}
                            });
                        }
                    }
                }
            }
        }
    }
    return txs;
}

size_t Blockchain::GetBlockCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_blocks.empty() ? 0 : m_blocks.size() - 1;
}

size_t Blockchain::GetMempoolCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_mempool.size();
}

nlohmann::json Blockchain::GetBlock(size_t index) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (index >= m_blocks.size()) {
        throw std::runtime_error("Block index out of range!");
    }
    return m_blocks[index];
}

nlohmann::json Blockchain::GetBlockByHash(const std::string& hash) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& b : m_blocks) {
        if (b.value("hash", "") == hash) {
            return b;
        }
    }
    throw std::runtime_error("Block hash not found!");
}

} // namespace BitMundi
