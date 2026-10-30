#ifndef MUNDICHAIN_CONSENSUS_PARAMS_H
#define MUNDICHAIN_CONSENSUS_PARAMS_H

#include <cstdint>
#include <string>

namespace Consensus {
namespace Params {

static constexpr int64_t COIN = 100000000;
static constexpr int64_t MAX_MONEY = 21000000LL * COIN;
static constexpr int64_t INITIAL_SUBSIDY = 5LL * COIN;
static constexpr int32_t HALVING_INTERVAL = 2100000;
static constexpr int32_t COINBASE_MATURITY = 10;

static constexpr int64_t TARGET_BLOCK_TIME = 60;
static constexpr int32_t RETARGET_INTERVAL = 120;
static constexpr int64_t TARGET_TIMESPAN = RETARGET_INTERVAL * TARGET_BLOCK_TIME; // 7200s
static constexpr uint32_t POW_LIMIT_BITS = 0x1e0ffff0;

static constexpr uint16_t DEFAULT_PORT = 18444;
static constexpr uint16_t DEFAULT_RPC_PORT = 18445;

static const char* const GENESIS_HASH = "00000425eb8d4d1cb567f0248d7b118201a104a8a81b22f87c4f523a3d5d84e8";
static const char* const GENESIS_MERKLE = "b552c93c39dea9fdecb10a49ebd10249061d6bf82cf9ecde67b94d4c79cdf5d7";
static constexpr uint64_t GENESIS_NONCE = 486008;
static constexpr int64_t GENESIS_TIME = 1792850647;
static constexpr uint32_t GENESIS_BITS = 0x1e0ffff0;
static const char* const GENESIS_HEADLINE = "Reuters 24/Oct/2026 Central Banks Accelerate Rate Adjustments - BitMundi Launches Sovereign 21 Million Sound Money";

} // namespace Params
} // namespace Consensus

#endif // MUNDICHAIN_CONSENSUS_PARAMS_H
