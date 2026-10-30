cmake_minimum_required(VERSION 3.16)
project(MundiChainCore VERSION 1.5.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

include_directories(src)

set(CRYPTO_SRCS
    src/crypto/sha256.cpp
    src/crypto/ripemd160.cpp
    src/crypto/bech32.cpp
    src/crypto/base58.cpp
)

set(DAEMON_SRCS
    ${CRYPTO_SRCS}
    src/wallet/wallet.cpp
    src/consensus/blockchain.cpp
    src/consensus/miner.cpp
    src/rpc/server.cpp
    src/daemon/main.cpp
)

add_executable(mundichaind ${DAEMON_SRCS})
target_link_libraries(mundichaind PRIVATE OpenSSL::SSL OpenSSL::Crypto Threads::Threads)

set(CLI_SRCS
    src/cli/main.cpp
)

add_executable(mundichain-cli ${CLI_SRCS})
target_link_libraries(mundichain-cli PRIVATE OpenSSL::SSL OpenSSL::Crypto Threads::Threads)
