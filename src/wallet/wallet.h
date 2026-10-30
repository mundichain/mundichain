#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstdint>
#include "crypto/json.hpp"

namespace BitMundi {

struct AddressEntry {
    std::string privkey;
    std::string pubkey;
    std::string label;
    int64_t created_at;
};

struct BalanceResult {
    double mature;
    double immature;
    double total;
};

struct WalletInfo {
    std::string wallet_file;
    std::string address;
    std::string label;
    double balance;
    double immature;
    double total;
    bool has_password;
    std::string wallets_dir;
};

class CWallet {
public:
    CWallet(const std::string& dataDir);
    ~CWallet() = default;

    void LoadOrInit();
    void InitWallet();
    void SaveWallet();

    std::string GenerateNewAddress(const std::string& label = "New Address");
    std::vector<nlohmann::json> GetAddresses(const std::function<BalanceResult(const std::string&)>& balFunc);

    nlohmann::json DumpPrivKey(const std::string& address, const std::string& passphrase = "");
    nlohmann::json ImportPrivKey(const std::string& privInput, const std::string& label = "Imported Key", const std::string& passphrase = "");

    bool SetPassword(const std::string& newPassword);
    bool CheckPassword(const std::string& password) const;
    bool HasPassword() const { return !m_passwordHash.empty(); }

    std::vector<std::string> ListWallets();
    bool LoadWallet(const std::string& walletName);
    bool CreateWallet(const std::string& walletName);
    bool ImportWallet(const std::string& srcPath);
    bool BackupWallet(const std::string& destPath);

    std::string GetActiveWalletName() const { return m_activeWalletName; }
    std::string GetWalletsDir() const { return m_walletsDir; }
    std::string GetPrimaryAddress() const;

    const std::unordered_map<std::string, AddressEntry>& GetAddressesMap() const { return m_addresses; }
    bool HasAddress(const std::string& addr) const { return m_addresses.find(addr) != m_addresses.end(); }

private:
    std::string m_dataDir;
    std::string m_walletsDir;
    std::string m_activeWalletName;
    std::string m_walletFile;

    std::string m_seed;
    std::string m_passwordHash;
    std::unordered_map<std::string, AddressEntry> m_addresses;

    mutable std::mutex m_mutex;
};

} // namespace BitMundi
