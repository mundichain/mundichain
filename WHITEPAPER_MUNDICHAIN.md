# BitMundi & MundiChain (BMD)
## Official Whitepaper: The Next Frontier of Decentralized Sound Money
**Version 1.5.0 — October 2026**  
**BitMundi Foundation** — [https://bitmundi.pages.dev](https://bitmundi.pages.dev)  
**P2P / Bootstrap Network:** `mundichain.duckdns.org:18444`  
**Official Repository:** [https://github.com/mundichain/mundichain](https://github.com/mundichain/mundichain)

---

## 1. Executive Summary (Abstract)

**MundiChain (BMD)** is a public, decentralized, auditable Layer-1 blockchain protocol engineered to restore the foundational pillars of sovereign digital sound money introduced by Satoshi Nakamoto in 2008. While modern cryptocurrency networks have largely migrated toward centralized Proof-of-Stake (PoS) models—characterized by validator censorship, foundation gatekeeping, and pre-mined token allocations—MundiChain reaffirms the unyielding guarantees of **Proof-of-Work (PoW)** consensus.

MundiChain features an immutable mathematical supply cap of **21,000,000 BMD**, a high-velocity **60-second block time**, a predictable halving schedule, smooth algorithmic difficulty adjustments, a native zero-overhead C++20 core engine, and an intuitive desktop wallet with integrated mining and automatic multi-server failover.

---

## 2. Vision and Core Principles

MundiChain is built upon four non-negotiable architectural mandates:

1. **Censorship-Resistant Decentralization:** No intermediary, preferred validator, or governing foundation can reverse finalized blocks or prohibit valid transactions. The ledger is protected by physical computation and thermodynamics.
2. **Unyielding Mathematical Scarcity:** Total issuance is mathematically capped at 21 million units. No corporate entity, committee, or algorithm can alter this supply cap.
3. **Native Zero-Overhead Performance:** The core node daemon (`mundichaind`) is written entirely in optimized modern C++ (`-std=c++2a`), delivering microsecond-scale transaction verification, low network overhead, and ultra-lean memory usage (< 100 MB RAM).
4. **Frictionless User Accessibility:** A native cross-platform GUI wallet (`bitmundi-wallet.exe`) with integrated one-click mining, automatic daemon bootstrapping, Bech32 SegWit addresses, and instant encrypted key backups.

---

## 3. Technical Network Specifications

| Parameter | Specification | Technical Rationale |
| :--- | :--- | :--- |
| **Asset Name** | BitMundi | Sovereign network currency |
| **Ticker Symbol** | **BMD** | Universal market identifier |
| **Architecture** | Native Layer-1 Blockchain | Independent sovereign chain |
| **Consensus Algorithm** | Proof-of-Work (PoW) | Double SHA-256 (`SHA-256d`) |
| **Maximum Supply** | **21,000,000 BMD** | Hard mathematical scarcity |
| **Target Block Time** | **60 seconds** | 10x faster settlement than Bitcoin |
| **Initial Block Subsidy** | **5.0 BMD per block** | Fair emission without predatory inflation |
| **Halving Interval** | **2,100,000 blocks** | Subsidy reduction approximately every 4 years |
| **Difficulty Retargeting** | Every **120 blocks** (~2 hours) | Smooth responsive difficulty adjustment based on real hashrate |
| **Address Format** | Native SegWit (`bmd1q...`) | Efficient Bech32 encoding with error detection |
| **P2P Network Port** | `18444` | Decentralized node-to-node peer discovery |
| **RPC API Port** | `18445` | Secure JSON-RPC integration for wallets and explorers |
| **Genesis Block Hash** | `00000425eb8d4d1cb567f0248d7b118201a104a8a81b22f87c4f523a3d5d84e8` | Immutable cryptographic anchor |

---

## 4. Monetary Policy and Emission Schedule (Halving)

Unlike inflationary fiat currencies and discretionary tokens, MundiChain issuance follows a strictly deterministic, predictable halving curve:

$$\text{BlockSubsidy}(h) = \left\lfloor \frac{5.0 \times 10^8}{2^{\lfloor h / 2,100,000 \rfloor}} \right\rfloor \text{ satoshis}$$

* **Era 1 (Blocks 0 to 2,099,999):** 5.0 BMD per block $\rightarrow$ 10,500,000 BMD minted (50% of total supply).
* **Era 2 (Blocks 2,100,000 to 4,199,999):** 2.5 BMD per block $\rightarrow$ 5,250,000 BMD minted.
* **Era 3 (Blocks 4,200,000 to 6,299,999):** 1.25 BMD per block $\rightarrow$ 2,625,000 BMD minted.
* **Subsequent Eras:** Continue halving every 2,100,000 blocks until the final satoshi of BMD is mined, ensuring structural deflationary value preservation.

---

## 5. Cryptographic Framework and Security

Every block and transaction is secured by battle-tested cryptographic primitives:

* **Digital Signatures & Keypairs:** Elliptic Curve Cryptography using `secp256k1` (256 bits of cryptographic entropy) with standard Wallet Import Format (WIF) private key export.
* **Block Header Architecture:** Each block binds the cryptographic hash of the previous block, POSIX UNIX timestamp, compact difficulty target (`bits`), 32-bit nonce, and the cryptographic Merkle Root of all included transactions.
* **Double-Spend & Sybil Defense:** Strict UTXO (Unspent Transaction Output) ledger tracking with mandatory 100-block Coinbase Maturity before newly mined rewards can be spent.

---

## 6. Software Architecture and Ecosystem

### 6.1 Native C++ Core Daemon (`mundichaind`)
The beating heart of the network. Built in modern C++ with an asynchronous multi-threaded architecture:
- High-throughput transaction memory pool (`mempool`).
- Robust persistent local chain database (`chain.json`).
- High-performance multi-client JSON-RPC 2.0 server.
- Autonomous P2P gossip protocol with seed discovery and active peer management.

### 6.2 Desktop GUI Wallet (`bitmundi-wallet.exe`)
A state-of-the-art native desktop application with modern dark and light interfaces:
- **Real-Time Overview:** Confirmed and immature balances, recent network blocks, and synchronization progress.
- **Peer-to-Peer Transfers:** Instant wallet-to-wallet transfers with dynamic fee calculation.
- **Address Manager:** Bech32 address generation with interactive payment QR codes.
- **Integrated CPU Mining:** One-click built-in miner with multi-thread allocation.
- **Automatic Multi-Node Failover:** Transparently switches between local daemon (`127.0.0.1:18445`), dedicated server (`192.168.1.190:18445`), and public DNS fallback (`mundichain.duckdns.org:18445`).

### 6.3 Public Explorers and Mining Infrastructure (`MundiScan` & `MundiPool`)
- **MundiScan Explorer:** Transparent ledger inspection, live block monitoring, hash audits, and address balance lookup.
- **MundiPool Portal:** Stratum mining pool documentation, real-time network hashrate visualization, and miner connectivity guides.

---

## 7. Community-Driven Governance

MundiChain has:
- **Zero Venture Capital (VC) Allocation**
- **Zero Pre-Mine**
- **Zero Founder Taxes or Hidden Premine Pools**

The network belongs entirely to its global miners, node operators, and coin holders. Anyone with standard consumer hardware can run a full sovereign node or participate in mining.

---

## 8. Conclusion

**MundiChain (BMD)** combines the mathematical rigor and immutability of Satoshi Nakamoto's original sound money vision with modern execution speed and accessibility. With 60-second block intervals, a strictly auditable 21-million cap, a complete C++ software ecosystem, and transparent decentralized governance, BitMundi represents a robust, practical, and censorship-resistant digital currency for the real world.

---

*© 2026 BitMundi Foundation. Open source software distributed under the MIT License.*
