// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2026 The MundiChain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MUNDICHAIN_UINT256_H
#define MUNDICHAIN_UINT256_H

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>

template<unsigned int BITS>
class base_blob
{
protected:
    static constexpr int WIDTH = BITS / 8;
    uint8_t m_data[WIDTH];

public:
    constexpr base_blob() : m_data{0} {}

    explicit base_blob(const std::vector<uint8_t>& vch)
    {
        if (vch.size() == sizeof(m_data))
            std::memcpy(m_data, vch.data(), sizeof(m_data));
        else
            std::memset(m_data, 0, sizeof(m_data));
    }

    explicit base_blob(const uint8_t* pdata)
    {
        std::memcpy(m_data, pdata, sizeof(m_data));
    }

    bool IsNull() const
    {
        for (int i = 0; i < WIDTH; i++)
            if (m_data[i] != 0)
                return false;
        return true;
    }

    void SetNull()
    {
        std::memset(m_data, 0, sizeof(m_data));
    }

    inline int Compare(const base_blob& other) const
    {
        for (int i = WIDTH - 1; i >= 0; i--) {
            if (m_data[i] < other.m_data[i])
                return -1;
            if (m_data[i] > other.m_data[i])
                return 1;
        }
        return 0;
    }

    friend inline bool operator==(const base_blob& a, const base_blob& b) { return a.Compare(b) == 0; }
    friend inline bool operator!=(const base_blob& a, const base_blob& b) { return a.Compare(b) != 0; }
    friend inline bool operator<(const base_blob& a, const base_blob& b)  { return a.Compare(b) < 0; }
    friend inline bool operator<=(const base_blob& a, const base_blob& b) { return a.Compare(b) <= 0; }
    friend inline bool operator>(const base_blob& a, const base_blob& b)  { return a.Compare(b) > 0; }
    friend inline bool operator>=(const base_blob& a, const base_blob& b) { return a.Compare(b) >= 0; }

    std::string GetHex() const
    {
        std::ostringstream ss;
        for (int i = WIDTH - 1; i >= 0; i--) {
            ss << std::hex << std::setw(2) << std::setfill('0') << (int)m_data[i];
        }
        return ss.str();
    }

    void SetHex(const std::string& str)
    {
        SetNull();
        int digits = 0;
        const char* psz = str.c_str();
        while (*psz) {
            digits++;
            psz++;
        }
        psz = str.c_str();
        int i = 0;
        while (digits > 0 && i < WIDTH) {
            char c = psz[--digits];
            uint8_t val = 0;
            if (c >= '0' && c <= '9') val = c - '0';
            else if (c >= 'a' && c <= 'f') val = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F') val = c - 'A' + 10;
            m_data[i] |= val;

            if (digits > 0) {
                c = psz[--digits];
                val = 0;
                if (c >= '0' && c <= '9') val = c - '0';
                else if (c >= 'a' && c <= 'f') val = c - 'a' + 10;
                else if (c >= 'A' && c <= 'F') val = c - 'A' + 10;
                m_data[i] |= (val << 4);
            }
            i++;
        }
    }

    std::string ToString() const { return GetHex(); }

    const uint8_t* data() const { return m_data; }
    uint8_t* data() { return m_data; }
    const uint8_t* begin() const { return &m_data[0]; }
    uint8_t* begin() { return &m_data[0]; }
    const uint8_t* end() const { return &m_data[WIDTH]; }
    uint8_t* end() { return &m_data[WIDTH]; }
    static constexpr unsigned int size() { return WIDTH; }
};

class uint160 : public base_blob<160> {
public:
    constexpr uint160() : base_blob<160>() {}
    explicit uint160(const std::vector<uint8_t>& vch) : base_blob<160>(vch) {}
    explicit uint160(const uint8_t* pdata) : base_blob<160>(pdata) {}
};

class uint256 : public base_blob<256> {
public:
    constexpr uint256() : base_blob<256>() {}
    explicit uint256(const std::vector<uint8_t>& vch) : base_blob<256>(vch) {}
    explicit uint256(const uint8_t* pdata) : base_blob<256>(pdata) {}
    explicit uint256(const std::string& hex) { SetHex(hex); }
    explicit uint256(uint64_t b) {
        SetNull();
        m_data[0] = (uint8_t)b;
        m_data[1] = (uint8_t)(b >> 8);
        m_data[2] = (uint8_t)(b >> 16);
        m_data[3] = (uint8_t)(b >> 24);
        m_data[4] = (uint8_t)(b >> 32);
        m_data[5] = (uint8_t)(b >> 40);
        m_data[6] = (uint8_t)(b >> 48);
        m_data[7] = (uint8_t)(b >> 56);
    }
};

#endif // MUNDICHAIN_UINT256_H
