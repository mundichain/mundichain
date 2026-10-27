# 🌍 MundiChain (BMD) Core

[![Release](https://img.shields.io/github/v/release/mundichain/mundichain?color=blue&label=Latest%20Release)](https://github.com/mundichain/mundichain/releases/latest)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Network](https://img.shields.io/badge/Network-Mainnet-green)](https://mundichain.pages.dev)

A próxima geração de blockchain descentralizada, rápida e de baixa latência baseada em consenso híbrido Proof-of-Work (PoW) e Proof-of-Stake (PoS).

---

## ⚡ Especificações Técnicas

- **Ticker:** `BMD`
- **Fornecimento Máximo (Max Supply):** 21.000.000 BMD
- **Recompensa por Bloco Inicial:** 5.0 BMD
- **Tempo Médio de Bloco:** ~60 segundos
- **Algoritmo de Hashing:** SHA-256d
- **Porta P2P:** `18444`
- **Porta RPC:** `18332`
- **Rede Bootstrap:** `mundichain.duckdns.org:18444`

---

## 💻 Download da Carteira Oficial (Windows)

A versão oficial da **BitMundi Desktop Wallet** para Windows inclui:
- Instalador guiado com atalhos na Área de Trabalho e Menu Iniciar
- Nó local integrado com inicialização automática e silenciosa em segundo plano
- Verificador automático de novas atualizações via GitHub API
- Suporte a múltiplos idiomas e aceleração por hardware

📥 **[Baixar Instalador Oficial (.exe)](https://github.com/mundichain/mundichain/releases/latest)**

---

## 🚀 Como Iniciar um Nó (Node / Daemon)

### No Linux (Ubuntu / Debian)
```bash
# Dependências
sudo apt update && sudo apt install -y build-essential libssl-dev

# Compilação e execução
make
./bin/mundichaind
```

### No Windows
Execute o instalador oficial ou inicie o executável diretamente pelo terminal:
```cmd
mundichaind.exe
```

---

## 🔍 Consulta e Gerenciamento via CLI

Para consultar o status da rede e da sua carteira:

```bash
# Obter informações do nó e altura do bloco
mundichain-cli getinfo

# Saldo da carteira
mundichain-cli getbalance

# Gerar novo endereço
mundichain-cli getnewaddress
```

---

## 🌐 Ecossistema e Links Úteis

- **Website Oficial:** [mundichain.pages.dev](https://mundichain.pages.dev)
- **Explorador de Blocos (MundiScan):** [mundichain.pages.dev/mundiscan.html](https://mundichain.pages.dev/mundiscan.html)
- **Mineração & Pool (MundiPool):** [mundichain.pages.dev/mundipool.html](https://mundichain.pages.dev/mundipool.html)
- **Whitepaper:** [WHITEPAPER_MUNDICHAIN.md](WHITEPAPER_MUNDICHAIN.md)

---

## 📜 Licença

Distribuído sob a licença **MIT Open Source**. Consulte o arquivo [LICENSE](LICENSE) para mais detalhes.
