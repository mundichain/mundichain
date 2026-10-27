# 🌍 MundiChain (BMD) Core

[![Release](https://img.shields.io/github/v/release/mundichain/mundichain?color=blue&label=Latest%20Release)](https://github.com/mundichain/mundichain/releases/latest)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Network](https://img.shields.io/badge/Network-Mainnet-green)](https://bitmundi.pages.dev)

The next generation of decentralized, lightweight, and low-latency blockchain powered by native Proof-of-Work (PoW) consensus.

---

## ⚡ Technical Specifications

- **Ticker:** `BMD`
- **Max Supply:** 21,000,000 BMD
- **Initial Block Reward (Subsidy):** 5.0 BMD
- **Average Block Time:** ~60 seconds
- **Hashing Algorithm:** SHA-256d
- **P2P Port:** `18444`
- **RPC Port:** `18445`
- **Bootstrap Node:** `mundichain.duckdns.org:18444`

---

## 💻 Official Desktop Wallet Download (Windows)

The official **BitMundi Core Desktop Wallet** for Windows includes:
- Guided setup wizard with Desktop and Start Menu shortcuts
- Integrated native node with automatic silent background bootstrap
- Automatic update checker via GitHub Releases API
- Multi-server auto-failover (Local node & Dedicated Mainnet node)
- Multi-language support and hardware-accelerated GUI

📥 **[Download Official Installer (.exe)](https://github.com/mundichain/mundichain/releases/latest)**

---

## 🚀 How to Run a Node (Daemon)

### On Linux (Ubuntu / Debian)
```bash
# Install dependencies
sudo apt update && sudo apt install -y build-essential libssl-dev

# Build and start the daemon
make
./bin/mundichaind
```

### On Windows
Run the official installer or launch the executable directly via Command Prompt / PowerShell:
```cmd
mundichaind.exe
```

---

## 🔍 CLI Commands & Wallet Management

To check node and network status or manage your wallet:

```bash
# Get node info, blockchain height, and difficulty
mundichain-cli getinfo

# Check wallet balance
mundichain-cli getbalance

# Generate a new receiving address
mundichain-cli getnewaddress
```

---

## 🌐 Ecosystem & Official Links

- **Official Website:** [bitmundi.pages.dev](https://bitmundi.pages.dev)
- **Blockchain Explorer (MundiScan):** [bitmundi.pages.dev/mundiscan.html](https://bitmundi.pages.dev/mundiscan.html)
- **Mining Pool Portal (MundiPool):** [bitmundi.pages.dev/mundipool.html](https://bitmundi.pages.dev/mundipool.html)
- **Live REST API Status:** [bitmundi.pages.dev/api-status.html](https://bitmundi.pages.dev/api-status.html)
- **Whitepaper:** [WHITEPAPER_MUNDICHAIN.md](WHITEPAPER_MUNDICHAIN.md)

---

## 📜 License

Distributed under the **MIT Open Source License**. See [LICENSE](LICENSE) for more details.
