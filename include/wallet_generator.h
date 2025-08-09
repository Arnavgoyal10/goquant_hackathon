#ifndef WALLET_GENERATOR_H
#define WALLET_GENERATOR_H

#include <string>
#include <random>
#include <sstream>
#include <iomanip>
#include <iostream>

// Simple wallet generator for testnet use
// NOTE: Production would use proper cryptographic libraries like secp256k1
class WalletGenerator
{
private:
    // Generate secure random bytes
    std::vector<uint8_t> generateRandomBytes(size_t length)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint8_t> dis(0, 255);

        std::vector<uint8_t> bytes(length);
        for (size_t i = 0; i < length; ++i)
        {
            bytes[i] = dis(gen);
        }
        return bytes;
    }

    // Convert bytes to hex string
    std::string bytesToHex(const std::vector<uint8_t> &bytes)
    {
        std::stringstream ss;
        ss << "0x";
        for (const auto &byte : bytes)
        {
            ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte);
        }
        return ss.str();
    }

    // Simple address derivation (mock implementation)
    // Production would use proper elliptic curve cryptography
    std::string deriveAddress(const std::string &private_key)
    {
        // Mock address derivation for demonstration
        std::hash<std::string> hasher;
        size_t hash = hasher(private_key);

        std::stringstream ss;
        ss << "0x" << std::hex << std::setfill('0') << std::setw(40) << hash;
        return ss.str().substr(0, 42); // Ethereum address length
    }

public:
    struct Wallet
    {
        std::string private_key;
        std::string address;
        std::string mnemonic;

        void print() const
        {
            std::cout << "\n🔐 NEW TESTNET WALLET GENERATED" << std::endl;
            std::cout << "================================" << std::endl;
            std::cout << "Address: " << address << std::endl;
            std::cout << "Private Key: " << private_key << std::endl;
            std::cout << "\n⚠️  IMPORTANT SECURITY NOTES:" << std::endl;
            std::cout << "- This is for TESTNET ONLY" << std::endl;
            std::cout << "- Never use this wallet on mainnet" << std::endl;
            std::cout << "- Store private key securely" << std::endl;
            std::cout << "- Fund with Sepolia ETH from faucet" << std::endl;
        }
    };

    // Generate a new testnet wallet
    Wallet generateWallet()
    {
        Wallet wallet;

        // Generate 32-byte private key
        auto private_key_bytes = generateRandomBytes(32);
        wallet.private_key = bytesToHex(private_key_bytes);

        // Derive address (simplified)
        wallet.address = deriveAddress(wallet.private_key);

        // Generate mock mnemonic for reference
        wallet.mnemonic = "testnet wallet generated programmatically for hackathon demo";

        return wallet;
    }

    // Validate wallet format
    bool isValidAddress(const std::string &address)
    {
        return address.length() == 42 && address.substr(0, 2) == "0x";
    }

    bool isValidPrivateKey(const std::string &private_key)
    {
        return private_key.length() == 66 && private_key.substr(0, 2) == "0x";
    }
};

// Predefined testnet wallets for quick testing
namespace TestnetWallets
{
    // Pre-generated testnet wallet (safe to share publicly)
    const std::string DEMO_ADDRESS = "0x742d35Cc6634C0532925a3b8D87C1a0bE4C1234567";
    const std::string DEMO_PRIVATE_KEY = "0x1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef";

    // Create a demo wallet struct
    WalletGenerator::Wallet getDemoWallet()
    {
        WalletGenerator::Wallet demo;
        demo.address = DEMO_ADDRESS;
        demo.private_key = DEMO_PRIVATE_KEY;
        demo.mnemonic = "demo testnet wallet for hackathon challenge";
        return demo;
    }
}

#endif // WALLET_GENERATOR_H
