# 🌍 MundiChain Core (BitMundi — BMD)

[![C++20](https://img.shields.io/badge/Language-C%2B%2B20-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B20)
[![Consensus](https://img.shields.io/badge/Consensus-Nakamoto%20PoW%20(SHA--256d)-gold.svg)](https://bitmundi.pages.dev/whitepaper.html)
[![Supply](https://img.shields.io/badge/Max%20Supply-21%2C000%2C000%20BMD-orange.svg)](https://bitmundi.pages.dev)
[![Block Time](https://img.shields.io/badge/Block%20Time-60s-green.svg)](https://bitmundi.pages.dev/mundiscan.html)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Network](https://img.shields.io/badge/Mainnet-Active-brightgreen)](https://bitmundi.pages.dev)

Official reference implementation of **BitMundi (BMD)** on the **MundiChain Network**. BitMundi is an institutional-grade, zero-overhead, decentralized Proof-of-Work Layer-1 cryptocurrency engineered in native **C++20**.

Designed as a modern evolution of Satoshi Nakamoto's peer-to-peer electronic sound money, BitMundi features a strict 21,000,000 hard cap, rapid 60-second block finality, responsive 120-block difficulty adjustments, and native Bech32 SegWit address encoding (`bmd1q...`).

---

## ⚡ Technical Specifications

| Parameter | MundiChain Value | Bitcoin Comparison |
| :--- | :--- | :--- |
| **Ticker** | `BMD` | `BTC` |
| **Consensus Algorithm** | **Double SHA-256 (`SHA-256d`)** | Double SHA-256 (`SHA-256d`) |
| **Total Max Supply** | **21,000,000 BMD** | 21,000,000 BTC |
| **Block Interval** | **60 seconds** | 10 minutes (600s) |
| **Block Subsidy (Reward)** | **5.0 BMD** | 50 BTC (Genesis) / 3.125 BTC |
| **Halving Schedule** | **Every 2,100,000 blocks** (~4 years) | Every 210,000 blocks (~4 years) |
| **Difficulty Retargeting** | **Dynamic every 120 blocks** (~2 hours) | Every 2,016 blocks (~2 weeks) |
| **Premine / Dev Tax** | **0.00 BMD (100% Fair Launch)** | 0.00 BTC |
| **Address Format** | **Native Bech32 (`bmd1q...`)** | `bc1q...` |
| **Default P2P Port** | `18444` | `8333` |
| **Default RPC Port** | `18445` | `8332` |
| **Genesis Headline** | *"Reuters 24/Oct/2026 Central Banks Accelerate Rate Adjustments - BitMundi Launches Sovereign 21 Million Sound Money"* | The Times 03/Jan/2009 |

---

## 🛠️ Building MundiChain Core from Source

MundiChain Core is built with zero unnecessary third-party dependencies, requiring only OpenSSL and a modern C++20 compiler (`g++ 10+`, `clang 12+`, or MSVC 2019+).

### Linux (Ubuntu 20.04+, Debian 11+, Fedora, Arch)

```bash
# 1. Install build essentials and OpenSSL development headers
sudo apt update
sudo apt install -y build-essential libssl-dev git

# 2. Clone the official repository
git clone https://github.com/mundichain/mundichain.git
cd mundichain

# 3. Compile both the daemon and the CLI tool
make -j$(nproc)

# Binaries are generated in bin/
ls -la bin/
# bin/mundichaind      (Core Blockchain Daemon)
# bin/mundichain-cli   (Command Line RPC Client)
```

### Windows (Visual Studio / MSVC or MinGW)

Using CMake:
```cmd
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

---

## 🚀 Running the Node

### 1. Start the Daemon

```bash
# Run in foreground
./bin/mundichaind

# Or run in background via PM2 (recommended for servers)
pm2 start ./bin/mundichaind --name mundichain-node
```

The node will automatically load `mundichain.conf`, generate your local wallet if needed, and start listening on:
- P2P Port: `18444` (inter-node peer discovery)
- RPC Port: `18445` (local authenticated JSON-RPC interface)

### 2. Configuration (`mundichain.conf`)

Place `mundichain.conf` in your working directory or data folder:

```ini
# Network settings
port=18444
rpcport=18445
rpcallowip=127.0.0.1

# Mining settings
gen=0
genproclimit=-1
```

---

## 💻 CLI Commands & Wallet Management

Interact with your running daemon using `mundichain-cli`:

```bash
# Check network block height, difficulty, and consensus status
./bin/mundichain-cli getinfo

# Check spendable and immature mining balance
./bin/mundichain-cli getbalance

# Generate a new native SegWit receiving address (bmd1q...)
./bin/mundichain-cli getnewaddress

# View detailed wallet info
./bin/mundichain-cli getwalletinfo

# Inspect any transaction on the blockchain
./bin/mundichain-cli getrawtransaction <txid>

# Mine blocks locally (Solo Mining Mode)
./bin/mundichain-cli generatetoaddress <num_blocks> <your_bmd1q_address>
```

---

## 🌐 Official Ecosystem Links

- **Official Portal & Wallet Download:** [https://bitmundi.pages.dev](https://bitmundi.pages.dev)
- **MundiScan Blockchain Explorer:** [https://bitmundi.pages.dev/mundiscan.html](https://bitmundi.pages.dev/mundiscan.html)
- **P2P Transfer & Address Inspector:** [https://bitmundi.pages.dev/transactions.html](https://bitmundi.pages.dev/transactions.html)
- **MundiPool Portal:** [https://bitmundi.pages.dev/mundipool.html](https://bitmundi.pages.dev/mundipool.html)
- **REST API Live Status:** [https://bitmundi.pages.dev/api-status.html](https://bitmundi.pages.dev/api-status.html)
- **Technical Whitepaper:** [WHITEPAPER_MUNDICHAIN.md](WHITEPAPER_MUNDICHAIN.md)

---

## 📄 License

MundiChain Core is released under the terms of the **MIT license**. See [LICENSE](LICENSE) for more information.
