/*
 * auth_core.h
 * ------------
 * Shared cryptographic and password-security logic for CipherGuard.
 * Used by both the CLI build (main.cpp) and the WebAssembly build
 * (wasm_bindings.cpp) so the exact same logic runs in the terminal
 * and in the browser.
 *
 * Contents:
 *  - Minimal SHA1 implementation
 *  - HMAC-SHA1
 *  - TOTP generator (RFC 6238 style, simplified: raw ASCII secret,
 *    no Base32 decoding, for zero external dependencies)
 *  - Password strength scorer
 *  - Password generator
 */

#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <random>
#include <cstdint>
#include <cmath>
#include <ctime>
#include <algorithm>
#include <cctype>

// ======================= Minimal SHA1 implementation =======================
class SHA1 {
public:
    SHA1() { reset(); }

    void update(const uint8_t* data, size_t len) {
        for (size_t i = 0; i < len; i++) {
            block[blockIndex++] = data[i];
            totalLen += 8;
            if (blockIndex == 64) { processBlock(); blockIndex = 0; }
        }
    }

    std::vector<uint8_t> finalize() {
        uint64_t bitLen = totalLen;
        uint8_t pad = 0x80;
        update(&pad, 1);
        uint8_t zero = 0x00;
        while (blockIndex != 56) update(&zero, 1);
        for (int i = 7; i >= 0; i--) {
            uint8_t b = static_cast<uint8_t>((bitLen >> (i * 8)) & 0xFF);
            update(&b, 1);
        }
        std::vector<uint8_t> digest(20);
        for (int i = 0; i < 5; i++) {
            digest[i * 4]     = (h[i] >> 24) & 0xFF;
            digest[i * 4 + 1] = (h[i] >> 16) & 0xFF;
            digest[i * 4 + 2] = (h[i] >> 8) & 0xFF;
            digest[i * 4 + 3] = h[i] & 0xFF;
        }
        return digest;
    }

private:
    uint32_t h[5];
    uint8_t block[64];
    size_t blockIndex = 0;
    uint64_t totalLen = 0;

    void reset() {
        h[0] = 0x67452301; h[1] = 0xEFCDAB89; h[2] = 0x98BADCFE;
        h[3] = 0x10325476; h[4] = 0xC3D2E1F0;
        blockIndex = 0; totalLen = 0;
    }

    static uint32_t rol(uint32_t v, int n) { return (v << n) | (v >> (32 - n)); }

    void processBlock() {
        uint32_t w[80];
        for (int i = 0; i < 16; i++)
            w[i] = (block[i*4] << 24) | (block[i*4+1] << 16) | (block[i*4+2] << 8) | block[i*4+3];
        for (int i = 16; i < 80; i++)
            w[i] = rol(w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16], 1);

        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4];
        for (int i = 0; i < 80; i++) {
            uint32_t f, k;
            if (i < 20)      { f = (b & c) | ((~b) & d);        k = 0x5A827999; }
            else if (i < 40) { f = b ^ c ^ d;                   k = 0x6ED9EBA1; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
            else             { f = b ^ c ^ d;                   k = 0xCA62C1D6; }
            uint32_t temp = rol(a, 5) + f + e + k + w[i];
            e = d; d = c; c = rol(b, 30); b = a; a = temp;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e;
    }
};

inline std::vector<uint8_t> hmacSHA1(const std::vector<uint8_t>& key, const std::vector<uint8_t>& msg) {
    const size_t blockSize = 64;
    std::vector<uint8_t> k = key;
    if (k.size() > blockSize) {
        SHA1 sha; sha.update(k.data(), k.size());
        k = sha.finalize();
    }
    k.resize(blockSize, 0);

    std::vector<uint8_t> oKeyPad(blockSize), iKeyPad(blockSize);
    for (size_t i = 0; i < blockSize; i++) {
        oKeyPad[i] = k[i] ^ 0x5c;
        iKeyPad[i] = k[i] ^ 0x36;
    }

    SHA1 inner;
    inner.update(iKeyPad.data(), iKeyPad.size());
    inner.update(msg.data(), msg.size());
    std::vector<uint8_t> innerHash = inner.finalize();

    SHA1 outer;
    outer.update(oKeyPad.data(), oKeyPad.size());
    outer.update(innerHash.data(), innerHash.size());
    return outer.finalize();
}

// ======================= TOTP Generator =======================
inline uint32_t generateTOTP(const std::string& secret, int digits = 6, int timeStep = 30, int64_t timeOffset = 0) {
    uint64_t timeCounter = static_cast<uint64_t>(std::time(nullptr)) / timeStep + timeOffset;
    uint8_t msg[8];
    uint64_t tc = timeCounter;
    for (int i = 7; i >= 0; i--) { msg[i] = tc & 0xFF; tc >>= 8; }

    std::vector<uint8_t> key(secret.begin(), secret.end());
    std::vector<uint8_t> msgVec(msg, msg + 8);
    std::vector<uint8_t> hash = hmacSHA1(key, msgVec);

    int offset = hash[hash.size() - 1] & 0x0F;
    uint32_t binCode = ((hash[offset] & 0x7f) << 24) |
                        ((hash[offset + 1] & 0xff) << 16) |
                        ((hash[offset + 2] & 0xff) << 8) |
                        (hash[offset + 3] & 0xff);
    uint32_t mod = static_cast<uint32_t>(std::pow(10, digits));
    return binCode % mod;
}

inline int secondsRemainingInStep(int timeStep = 30) {
    return timeStep - (static_cast<int>(std::time(nullptr)) % timeStep);
}

// ======================= Password Strength Checker =======================
inline std::set<std::string>& commonPasswordList() {
    static std::set<std::string> list = {
        "password", "123456", "12345678", "qwerty", "abc123", "letmein",
        "monkey", "111111", "iloveyou", "admin", "welcome", "password1",
        "123456789", "12345", "1234567", "sunshine", "princess", "football"
    };
    return list;
}

inline std::string toLowerStr(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

inline int scorePassword(const std::string& pwd, std::vector<std::string>& feedback) {
    int score = 0;

    if (pwd.length() >= 12) score += 25;
    else if (pwd.length() >= 8) score += 15;
    else feedback.push_back("Too short - use 12+ characters");

    bool hasLower = false, hasUpper = false, hasDigit = false, hasSpecial = false;
    for (char c : pwd) {
        if (islower(static_cast<unsigned char>(c))) hasLower = true;
        else if (isupper(static_cast<unsigned char>(c))) hasUpper = true;
        else if (isdigit(static_cast<unsigned char>(c))) hasDigit = true;
        else hasSpecial = true;
    }
    int variety = hasLower + hasUpper + hasDigit + hasSpecial;
    score += variety * 15;
    if (!hasUpper) feedback.push_back("Add uppercase letters");
    if (!hasDigit) feedback.push_back("Add numbers");
    if (!hasSpecial) feedback.push_back("Add special characters");

    if (commonPasswordList().count(toLowerStr(pwd))) {
        score = 5;
        feedback.push_back("This is a commonly used password!");
    }

    std::set<char> uniqueChars(pwd.begin(), pwd.end());
    if (pwd.length() > 4 && uniqueChars.size() < pwd.length() / 2)
        feedback.push_back("Too many repeated characters");

    return std::min(score, 100);
}

inline std::string strengthLabel(int score) {
    if (score >= 80) return "Very Strong";
    if (score >= 60) return "Strong";
    if (score >= 40) return "Moderate";
    if (score >= 20) return "Weak";
    return "Very Weak";
}

// ======================= Password Generator =======================
inline std::string generatePassword(int length, bool useUpper, bool useDigits, bool useSpecial) {
    std::string chars = "abcdefghijklmnopqrstuvwxyz";
    if (useUpper) chars += "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    if (useDigits) chars += "0123456789";
    if (useSpecial) chars += "!@#$%^&*()-_=+";

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, static_cast<int>(chars.size()) - 1);

    std::string pwd;
    for (int i = 0; i < length; i++) pwd += chars[dist(gen)];
    return pwd;
}
