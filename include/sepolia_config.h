#ifndef SEPOLIA_CONFIG_H
#define SEPOLIA_CONFIG_H

#include <string>
#include <map>

// Real Sepolia testnet configuration for Curve Finance interaction
namespace SepoliaConfig
{

    // Network Configuration
    // TODO: Replace with your actual RPC endpoint
    // Options: Alchemy, Infura, or public endpoint
    const std::string SEPOLIA_RPC_URL = "https://eth-sepolia.g.alchemy.com/v2/lJ_z7pJgy80hk4TLRsFQD";
    const uint64_t SEPOLIA_CHAIN_ID = 11155111;
    const std::string SEPOLIA_EXPLORER = "https://sepolia.etherscan.io";

    // Real Sepolia Token Addresses
    namespace Tokens
    {
        // These are actual testnet token addresses on Sepolia
        const std::string WETH = "0xfFf9976782d46CC05630D1f6eBAb18b2324d6B14"; // Wrapped ETH
        const std::string USDC = "0x1c7D4B196Cb0C7B01d743Fbc6116a902379C7238"; // USD Coin (testnet)
        const std::string DAI = "0x3e622317f8C93f7328350cF0B56d9eD4C620C5d6";  // DAI (testnet)
        const std::string USDT = "0xaA8E23Fb1079EA71e0a56F48a2aA51851D8433D0"; // Tether (testnet)

        // Token metadata
        struct TokenInfo
        {
            std::string address;
            std::string symbol;
            uint8_t decimals;
            std::string name;
        };

        const std::map<std::string, TokenInfo> TOKEN_INFO = {
            {"WETH", {WETH, "WETH", 18, "Wrapped Ether"}},
            {"USDC", {USDC, "USDC", 6, "USD Coin"}},
            {"DAI", {DAI, "DAI", 18, "Dai Stablecoin"}},
            {"USDT", {USDT, "USDT", 6, "Tether USD"}}};
    }

    // Curve Finance Protocol Addresses on Sepolia
    // NOTE: These need to be verified for actual Sepolia deployment
    namespace Curve
    {
        // TODO: Verify actual Curve deployments on Sepolia
        // These are placeholder addresses - you'll need to discover real ones

        // For now, we'll use the mainnet addresses as reference
        // You'll need to find the actual Sepolia addresses or use a different testnet

        const std::string CURVE_REGISTRY = "0x90E00ACe148ca3b23Ac1bC8C240C2a7Dd9c2d7f5"; // Mainnet reference
        const std::string CURVE_FACTORY = "0xB9fC157394Af804a3578134A6585C0dc9cc990d4";  // Mainnet reference

        // Example Curve pools (these need to be discovered on Sepolia)
        namespace Pools
        {
            // TODO: Discover actual pools on Sepolia
            // For now, using mainnet addresses as reference
            const std::string USDC_DAI_POOL = "0xBebc44782C7dB0a1A60Cb6fe97d0b483032FF1C7";  // Mainnet reference
            const std::string WETH_USDC_POOL = "0xDC24316b9AE028F1497c275EB9192a3Ea0f67022"; // Mainnet reference

            struct PoolInfo
            {
                std::string address;
                std::string token0;
                std::string token1;
                std::string name;
            };

            const std::map<std::string, PoolInfo> POOL_INFO = {
                {"USDC_DAI", {USDC_DAI_POOL, Tokens::USDC, Tokens::DAI, "USDC/DAI Pool"}},
                {"WETH_USDC", {WETH_USDC_POOL, Tokens::WETH, Tokens::USDC, "WETH/USDC Pool"}}};
        }
    }

    // Faucet URLs for getting testnet tokens
    namespace Faucets
    {
        const std::string SEPOLIA_ETH_FAUCET = "https://sepoliafaucet.com";
        const std::string ALCHEMY_FAUCET = "https://sepoliafaucet.com";
        const std::string INFURA_FAUCET = "https://infura.io/faucet/sepolia";
        const std::string GOOGLE_FAUCET = "https://cloud.google.com/application/web3/faucet/ethereum/sepolia";
    }

    // Gas Configuration for Sepolia
    namespace Gas
    {
        const uint64_t DEFAULT_GAS_LIMIT = 200000;
        const uint64_t SWAP_GAS_LIMIT = 300000;
        const uint64_t APPROVE_GAS_LIMIT = 100000;
        const uint64_t DEFAULT_GAS_PRICE = 20000000000; // 20 gwei
    }

    // Wallet Configuration
    // TODO: Add your actual wallet credentials here
    namespace Wallet
    {
        // Replace these with your actual wallet details
        const std::string PRIVATE_KEY = "0xe78a25a70199b171bded4306b1a9b805d73ed22df1cfb631b60571b4aa0a757c";
        const std::string ADDRESS = "0x00Da5B17c4b3A17f787491868A6200A4bFe01DE8";

        // Security note: In production, never hardcode private keys
        // For this hackathon, we'll use a dedicated testnet wallet
    }

    // Helper function to get token info
    Tokens::TokenInfo getTokenInfo(const std::string &symbol)
    {
        auto it = Tokens::TOKEN_INFO.find(symbol);
        if (it != Tokens::TOKEN_INFO.end())
        {
            return it->second;
        }
        throw std::runtime_error("Unknown token symbol: " + symbol);
    }

    // Helper function to get pool info
    Curve::Pools::PoolInfo getPoolInfo(const std::string &pool_name)
    {
        auto it = Curve::Pools::POOL_INFO.find(pool_name);
        if (it != Curve::Pools::POOL_INFO.end())
        {
            return it->second;
        }
        throw std::runtime_error("Unknown pool name: " + pool_name);
    }

    // Validation functions
    bool isSepoliaAddress(const std::string &address)
    {
        return address.length() == 42 && address.substr(0, 2) == "0x";
    }

    bool isConfigured()
    {
        // Check if we have valid configuration values
        return !SEPOLIA_RPC_URL.empty() &&
               SEPOLIA_RPC_URL.length() > 20 && // Basic length check for valid URL
               !Wallet::PRIVATE_KEY.empty() &&
               Wallet::PRIVATE_KEY.length() == 66 && // 0x + 64 hex chars
               !Wallet::ADDRESS.empty() &&
               Wallet::ADDRESS.length() == 42; // 0x + 40 hex chars
    }
}

#endif // SEPOLIA_CONFIG_H
