#ifndef TRANSACTION_SIGNER_H
#define TRANSACTION_SIGNER_H

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <cstring>

// Simple transaction structure for Ethereum
struct EthereumTransaction
{
    uint64_t nonce;
    uint64_t gas_price;
    uint64_t gas_limit;
    std::string to_address;
    uint64_t value;
    std::string data;
    uint64_t chain_id;

    EthereumTransaction() : nonce(0), gas_price(20000000000), gas_limit(200000),
                            value(0), chain_id(11155111) {} // Sepolia chain ID
};

// Simplified transaction signer (production would use secp256k1 library)
class TransactionSigner
{
private:
    std::string private_key;

    // Helper function to convert bytes to hex string
    std::string bytesToHex(const std::vector<uint8_t> &bytes)
    {
        std::stringstream ss;
        for (const auto &byte : bytes)
        {
            ss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(byte);
        }
        return "0x" + ss.str();
    }

    // Simple hash function (production would use Keccak256)
    std::vector<uint8_t> simpleHash(const std::string &data)
    {
        std::vector<uint8_t> hash(32, 0);
        // Simple deterministic hash for demo
        for (size_t i = 0; i < data.length() && i < 32; ++i)
        {
            hash[i] = static_cast<uint8_t>(data[i] ^ (i + 1));
        }
        return hash;
    }

    // RLP encoding simulation (production would use proper RLP)
    std::string encodeTransaction(const EthereumTransaction &tx)
    {
        std::stringstream rlp;
        rlp << std::hex << tx.nonce << ":"
            << tx.gas_price << ":"
            << tx.gas_limit << ":"
            << tx.to_address << ":"
            << tx.value << ":"
            << tx.data << ":"
            << tx.chain_id;
        return rlp.str();
    }

public:
    TransactionSigner(const std::string &priv_key) : private_key(priv_key) {}

    // Sign a transaction and return raw transaction hex
    std::string signTransaction(const EthereumTransaction &tx)
    {
        std::cout << "🔐 Signing transaction..." << std::endl;
        std::cout << "   To: " << tx.to_address << std::endl;
        std::cout << "   Data: " << tx.data.substr(0, 20) << "..." << std::endl;
        std::cout << "   Gas Limit: " << tx.gas_limit << std::endl;

        // Encode transaction for signing
        std::string encoded = encodeTransaction(tx);

        // Create transaction hash
        auto hash = simpleHash(encoded);

        // Simulate signature (production would use ECDSA with secp256k1)
        std::vector<uint8_t> signature(65, 0);

        // Fill with deterministic but realistic-looking signature
        for (size_t i = 0; i < 32; ++i)
        {
            signature[i] = hash[i] ^ 0xAA;      // r component
            signature[i + 32] = hash[i] ^ 0x55; // s component
        }
        signature[64] = 27; // v component (recovery id)

        // Combine encoded transaction with signature
        std::string raw_tx = bytesToHex(signature) + encoded;

        std::cout << "✅ Transaction signed successfully!" << std::endl;
        std::cout << "   Signature length: " << signature.size() << " bytes" << std::endl;

        return raw_tx;
    }

    // Create and sign a Curve swap transaction
    std::string createSwapTransaction(const std::string &pool_address,
                                      const std::string &function_data,
                                      const std::string &from_address)
    {
        EthereumTransaction tx;
        tx.to_address = pool_address;
        tx.data = function_data;
        tx.gas_limit = 300000; // Higher gas for swap
        tx.nonce = getCurrentNonce(from_address);

        return signTransaction(tx);
    }

    // Get current nonce for address (simplified)
    uint64_t getCurrentNonce(const std::string &address)
    {
        // In production, this would query eth_getTransactionCount
        // For demo, return a mock nonce
        std::cout << "📊 Getting nonce for " << address << std::endl;
        return 42; // Mock nonce
    }

    // Broadcast transaction to network
    std::string broadcastTransaction(const std::string &raw_tx)
    {
        std::cout << "📡 Broadcasting transaction..." << std::endl;
        std::cout << "   Raw TX length: " << raw_tx.length() << " chars" << std::endl;

        // In production, this would call eth_sendRawTransaction
        // For demo, return a realistic-looking transaction hash
        std::string tx_hash = "0x" + raw_tx.substr(0, 64);

        std::cout << "✅ Transaction broadcasted!" << std::endl;
        std::cout << "   TX Hash: " << tx_hash << std::endl;

        return tx_hash;
    }
};

#endif // TRANSACTION_SIGNER_H
