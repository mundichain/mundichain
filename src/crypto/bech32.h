#ifndef MUNDICHAIN_CRYPTO_BECH32_H
#define MUNDICHAIN_CRYPTO_BECH32_H

#include <string>
#include <vector>
#include <cstdint>

namespace bech32 {

std::string Encode(const std::string& hrp, const std::vector<uint8_t>& values);
std::pair<std::string, std::vector<uint8_t>> Decode(const std::string& str);

bool ConvertBits(std::vector<uint8_t>& out, const std::vector<uint8_t>& in, int frombits, int tobits, bool pad);

/** Encode MundiChain PubKey Hash to bmd1q... address */
std::string EncodeMundiAddress(const std::vector<uint8_t>& pubkey_hash);
std::vector<uint8_t> DecodeMundiAddress(const std::string& address);

} // namespace bech32

#endif // MUNDICHAIN_CRYPTO_BECH32_H
