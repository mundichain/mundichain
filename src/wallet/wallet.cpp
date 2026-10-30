#include "wallet/wallet.h"
#include "crypto/sha256.h"
#include "crypto/ripemd160.h"
#include "crypto/base58.h"
#include <fstream>
#include <random>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

namespace BitMundi {

static std::string GenerateRandomHex(size_t numBytes) {
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint32_t> dis(0, 255);
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (size_t i = 0; i < numBytes; ++i) {
        ss << std::setw(2) << dis(gen);
    }
    return ss.str();
}

static std::vector<uint8_t> HexToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        uint8_t byte = (uint8_t)strtol(byteString.c_str(), nullptr, 16);
        bytes.push_back(byte);
    }
    return bytes;
}

static std::string DeriveAddress(const std::string& privHex, std::string& outPubHex) {
    std::vector<uint8_t> privBytes = HexToBytes(privHex);
    uint256 pubHash = Hash256(privBytes.begin(), privBytes.end()); // SHA256 of privkey matches mundichaind.py
    
    // In mundichaind.py:
    // pubkey = hashlib.sha256(bytes.fromhex(privkey)).hexdigest()
    // addr = "bmd1q" + hashlib.new('ripemd160', bytes.fromhex(pubkey)).hexdigest()[:28]
    CSHA256 sha;
    sha.Write(privBytes.data(), privBytes.size());
    uint256 pub;
    sha.Finalize((unsigned char*)&pub);
    outPubHex = pub.GetHex();

    std::vector<uint8_t> pubBytes = HexToBytes(outPubHex);
    CRIPEMD160 rip;
    rip.Write(pubBytes.data(), pubBytes.size());
    uint160 ripHash;
    rip.Finalize((unsigned char*)&ripHash);

    std::string ripHex = ripHash.GetHex();
    return "bmd1q" + ripHex.substr(0, 28);
}

CWallet::CWallet(const std::string& dataDir)
    : m_dataDir(dataDir), m_activeWalletName("wallet.dat") {
    m_walletsDir = (fs::path(m_dataDir) / "wallets").string();
    fs::create_directories(m_walletsDir);
    m_walletFile = (fs::path(m_walletsDir) / m_activeWalletName).string();
}

void CWallet::LoadOrInit() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // Auto migration: legacy wallet.dat or wallet.json in root dataDir
    fs::path legacyDat = fs::path(m_dataDir) / "wallet.dat";
    fs::path legacyJson = fs::path(m_dataDir) / "wallet.json";

    if (fs::exists(legacyDat) && !fs::exists(m_walletFile)) {
        try { fs::copy_file(legacyDat, m_walletFile); } catch (...) {}
    } else if (fs::exists(legacyJson) && !fs::exists(m_walletFile)) {
        try { fs::copy_file(legacyJson, m_walletFile); } catch (...) {}
    }

    if (fs::exists(m_walletFile)) {
        try {
            std::ifstream file(m_walletFile);
            nlohmann::json j;
            file >> j;
            m_seed = j.value("seed", "");
            m_passwordHash = j.value("password_hash", "");
            m_addresses.clear();
            if (j.contains("addresses") && j["addresses"].is_object()) {
                for (auto& [addr, info] : j["addresses"].items()) {
                    AddressEntry entry;
                    entry.privkey = info.value("privkey", "");
                    entry.pubkey = info.value("pubkey", "");
                    entry.label = info.value("label", "Default");
                    entry.created_at = info.value("created_at", (int64_t)time(nullptr));
                    m_addresses[addr] = entry;
                }
            }
            if (m_addresses.empty()) {
                InitWallet();
            }
        } catch (...) {
            InitWallet();
        }
    } else {
        InitWallet();
    }
}

void CWallet::InitWallet() {
    m_seed = GenerateRandomHex(32);
    m_passwordHash = "";
    m_addresses.clear();

    std::string priv = GenerateRandomHex(32);
    std::string pub;
    std::string addr = DeriveAddress(priv, pub);

    AddressEntry entry;
    entry.privkey = priv;
    entry.pubkey = pub;
    entry.label = "Default Wallet";
    entry.created_at = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    m_addresses[addr] = entry;
    SaveWallet();
}

void CWallet::SaveWallet() {
    nlohmann::json j;
    j["seed"] = m_seed;
    j["password_hash"] = m_passwordHash;
    j["addresses"] = nlohmann::json::object();

    for (const auto& [addr, entry] : m_addresses) {
        nlohmann::json info;
        info["privkey"] = entry.privkey;
        info["pubkey"] = entry.pubkey;
        info["label"] = entry.label;
        info["created_at"] = entry.created_at;
        j["addresses"][addr] = info;
    }

    std::ofstream file(m_walletFile);
    file << j.dump(2);
}

std::string CWallet::GenerateNewAddress(const std::string& label) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string priv = GenerateRandomHex(32);
    std::string pub;
    std::string addr = DeriveAddress(priv, pub);

    AddressEntry entry;
    entry.privkey = priv;
    entry.pubkey = pub;
    entry.label = label.empty() ? "New Address" : label;
    entry.created_at = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    m_addresses[addr] = entry;
    SaveWallet();
    return addr;
}

std::vector<nlohmann::json> CWallet::GetAddresses(const std::function<BalanceResult(const std::string&)>& balFunc) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<nlohmann::json> list;
    for (const auto& [addr, entry] : m_addresses) {
        BalanceResult bal = balFunc ? balFunc(addr) : BalanceResult{0.0, 0.0, 0.0};
        nlohmann::json item;
        item["address"] = addr;
        item["label"] = entry.label;
        item["created_at"] = entry.created_at;
        item["balance"] = bal.mature;
        item["total"] = bal.total;
        list.push_back(item);
    }
    return list;
}

std::string CWallet::GetPrimaryAddress() const {
    if (m_addresses.empty()) return "";
    return m_addresses.begin()->first;
}

bool CWallet::CheckPassword(const std::string& password) const {
    if (m_passwordHash.empty()) return true;
    CSHA256 sha;
    sha.Write((const unsigned char*)password.data(), password.size());
    uint256 hash;
    sha.Finalize((unsigned char*)&hash);
    return hash.GetHex() == m_passwordHash;
}

bool CWallet::SetPassword(const std::string& newPassword) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (newPassword.empty()) {
        m_passwordHash = "";
    } else {
        CSHA256 sha;
        sha.Write((const unsigned char*)newPassword.data(), newPassword.size());
        uint256 hash;
        sha.Finalize((unsigned char*)&hash);
        m_passwordHash = hash.GetHex();
    }
    SaveWallet();
    return true;
}

nlohmann::json CWallet::DumpPrivKey(const std::string& address, const std::string& passphrase) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_addresses.find(address);
    if (it == m_addresses.end()) {
        throw std::runtime_error("Address does not belong to active wallet!");
    }
    if (!m_passwordHash.empty()) {
        if (!CheckPassword(passphrase)) {
            throw std::runtime_error("Wallet is encrypted: incorrect or missing passphrase!");
        }
    }
    std::string privHex = it->second.privkey;
    std::string wif = HexToWif(privHex, true);
    nlohmann::json res;
    res["address"] = address;
    res["hex"] = privHex;
    res["wif"] = wif;
    return res;
}

nlohmann::json CWallet::ImportPrivKey(const std::string& privInput, const std::string& label, const std::string& passphrase) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_passwordHash.empty()) {
        if (!CheckPassword(passphrase)) {
            throw std::runtime_error("Wallet is encrypted: incorrect or missing passphrase!");
        }
    }
    std::string privHex = ParsePrivkeyInput(privInput);
    std::string pub;
    std::string addr = DeriveAddress(privHex, pub);

    AddressEntry entry;
    entry.privkey = privHex;
    entry.pubkey = pub;
    entry.label = label.empty() ? "Imported Key" : label;
    entry.created_at = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    m_addresses[addr] = entry;
    SaveWallet();

    nlohmann::json res;
    res["address"] = addr;
    res["label"] = entry.label;
    res["hex"] = privHex;
    res["wif"] = HexToWif(privHex, true);
    return res;
}

std::vector<std::string> CWallet::ListWallets() {
    std::vector<std::string> wallets;
    if (fs::exists(m_walletsDir)) {
        for (const auto& entry : fs::directory_iterator(m_walletsDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".dat") {
                wallets.push_back(entry.path().filename().string());
            }
        }
    }
    if (wallets.empty()) {
        wallets.push_back(m_activeWalletName);
    }
    std::sort(wallets.begin(), wallets.end());
    return wallets;
}

bool CWallet::LoadWallet(const std::string& walletName) {
    std::string name = walletName;
    if (name.length() < 4 || name.substr(name.length() - 4) != ".dat") {
        name += ".dat";
    }
    fs::path target = fs::path(m_walletsDir) / name;
    if (!fs::exists(target)) {
        throw std::runtime_error("Wallet file '" + name + "' not found!");
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    m_activeWalletName = name;
    m_walletFile = target.string();
    
    std::ifstream file(m_walletFile);
    nlohmann::json j;
    file >> j;
    m_seed = j.value("seed", "");
    m_passwordHash = j.value("password_hash", "");
    m_addresses.clear();
    if (j.contains("addresses") && j["addresses"].is_object()) {
        for (auto& [addr, info] : j["addresses"].items()) {
            AddressEntry entry;
            entry.privkey = info.value("privkey", "");
            entry.pubkey = info.value("pubkey", "");
            entry.label = info.value("label", "Default");
            entry.created_at = info.value("created_at", (int64_t)time(nullptr));
            m_addresses[addr] = entry;
        }
    }
    return true;
}

bool CWallet::CreateWallet(const std::string& walletName) {
    std::string name = walletName;
    if (name.length() < 4 || name.substr(name.length() - 4) != ".dat") {
        name += ".dat";
    }
    fs::path target = fs::path(m_walletsDir) / name;
    if (fs::exists(target)) {
        throw std::runtime_error("Wallet '" + name + "' already exists!");
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    m_activeWalletName = name;
    m_walletFile = target.string();
    InitWallet();
    return true;
}

bool CWallet::ImportWallet(const std::string& srcPath) {
    if (!fs::exists(srcPath)) {
        throw std::runtime_error("Source file not found: " + srcPath);
    }
    std::string baseName = fs::path(srcPath).filename().string();
    if (baseName.length() < 4 || baseName.substr(baseName.length() - 4) != ".dat") {
        baseName += ".dat";
    }
    fs::path dest = fs::path(m_walletsDir) / baseName;
    fs::copy_file(srcPath, dest, fs::copy_options::overwrite_existing);
    return LoadWallet(baseName);
}

bool CWallet::BackupWallet(const std::string& destPath) {
    std::string target = destPath;
    if (target.length() < 4 || target.substr(target.length() - 4) != ".dat") {
        target += ".dat";
    }
    fs::path p(target);
    if (!p.has_parent_path()) {
        p = fs::path(m_walletsDir) / p;
    }
    fs::copy_file(m_walletFile, p, fs::copy_options::overwrite_existing);
    return true;
}

} // namespace BitMundi
