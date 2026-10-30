# ==============================================================================
# MundiChain Core Daemon & CLI Makefile for Linux (Ubuntu / Debian / CentOS)
# ==============================================================================
CXX = g++
CXXFLAGS = -std=c++2a -O3 -pthread -Wall -Wextra -Isrc -D_GNU_SOURCE
LDFLAGS = -lpthread -lssl -lcrypto

SRCS_COMMON = src/crypto/sha256.cpp \
              src/crypto/ripemd160.cpp \
              src/crypto/bech32.cpp \
              src/crypto/base58.cpp \
              src/wallet/wallet.cpp \
              src/consensus/blockchain.cpp \
              src/consensus/miner.cpp \
              src/rpc/server.cpp

SRCS_DAEMON = $(SRCS_COMMON) src/daemon/main.cpp
SRCS_CLI = src/cli/main.cpp

all: bin/mundichaind bin/mundichain-cli

bin/mundichaind: $(SRCS_DAEMON)
	@mkdir -p bin/data/wallets
	$(CXX) $(CXXFLAGS) $(SRCS_DAEMON) -o bin/mundichaind $(LDFLAGS)
	@echo "============================================="
	@echo " mundichaind compilado com sucesso em bin/"
	@echo "============================================="

bin/mundichain-cli: $(SRCS_CLI)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(SRCS_CLI) -o bin/mundichain-cli $(LDFLAGS)
	@echo "============================================="
	@echo " mundichain-cli compilado com sucesso em bin/"
	@echo "============================================="

clean:
	rm -f bin/mundichaind bin/mundichain-cli
