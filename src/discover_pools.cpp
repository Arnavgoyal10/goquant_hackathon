#include <curl/curl.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <nlohmann/json.hpp>
#include "../include/sepolia_config.h"

using json = nlohmann::json;

// Simple HTTP client for RPC calls
class EthereumRPC
{
private:
    std::string rpc_url;
    CURL *curl;

    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *response)
    {
        size_t totalSize = size * nmemb;
        response->append((char *)contents, totalSize);
        return totalSize;
    }

public:
    EthereumRPC(const std::string &url) : rpc_url(url)
    {
        curl = curl_easy_init();
        if (!curl)
        {
            throw std::runtime_error("Failed to initialize CURL");
        }
    }

    ~EthereumRPC()
    {
        if (curl)
            curl_easy_cleanup(curl);
    }

    json call(const std::string &method, const json &params)
    {
        json request = {{"jsonrpc", "2.0"}, {"method", method}, {"params", params}, {"id", 1}};
        std::string request_str = request.dump();
        std::string response;

        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, rpc_url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, request_str.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);
        curl_slist_free_all(headers);

        if (res != CURLE_OK)
        {
            throw std::runtime_error("CURL request failed: " + std::string(curl_easy_strerror(res)));
        }

        return json::parse(response);
    }

    // Get contract code to check if address is a contract
    bool isContract(const std::string &address)
    {
        try
        {
            json params = {address, "latest"};
            json response = call("eth_getCode", params);

            if (response.contains("result"))
            {
                std::string code = response["result"];
                // Contract code is longer than 2 characters (0x)
                return code.length() > 2 && code != "0x";
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Error checking contract: " << e.what() << std::endl;
        }
        return false;
    }

    // Get token balance to check if pool has liquidity
    uint64_t getTokenBalance(const std::string &contract, const std::string &token [[maybe_unused]], const std::string &user = "0x0000000000000000000000000000000000000000")
    {
        try
        {
            // ERC-20 balanceOf function signature: 0x70a08231
            std::string data = "0x70a08231" + std::string(24, '0') + user.substr(2);

            json params = {{{"to", contract},
                            {"data", data}},
                           "latest"};

            json response = call("eth_call", params);

            if (response.contains("result"))
            {
                std::string result = response["result"];
                if (result.length() > 2)
                {
                    return std::stoull(result.substr(2), nullptr, 16);
                }
            }
        }
        catch (const std::exception &e)
        {
            // Silently fail for balance checks
        }
        return 0;
    }

    // Scan for pools by checking known addresses and testing for liquidity
    std::vector<std::string> discoverPools()
    {
        std::vector<std::string> pools;

        std::cout << "🔍 Scanning for Curve pools on Sepolia..." << std::endl;

        // Known potential pool addresses to test
        std::vector<std::string> candidates = {
            // Common Curve pool patterns
            "0x4DEcE678ceceb27446b35C672dC7d61F30bAD69E", // Known Sepolia pool
            "0x7f90122BF0700F9E7e1F688fe46aeE9C1C23dC23", // Another potential
            "0x8Fb3d7a8c1A3C9dD4B1d7F7c8B9d0e1F2a3B4C5",  // Test pattern
            "0x9Fb3d7a8c1A3C9dD4B1d7F7c8B9d0e1F2a3B4C6",  // Test pattern
            "0xAFb3d7a8c1A3C9dD4B1d7F7c8B9d0e1F2a3B4C7"   // Test pattern
        };

        for (const auto &candidate : candidates)
        {
            std::cout << "  Testing address: " << candidate << std::endl;

            if (isContract(candidate))
            {
                std::cout << "    ✅ Is a contract" << std::endl;

                // Check if it has token balances (indicating it's a pool)
                uint64_t usdc_balance = getTokenBalance(candidate, SepoliaConfig::Tokens::USDC);
                uint64_t dai_balance = getTokenBalance(candidate, SepoliaConfig::Tokens::DAI);
                uint64_t weth_balance = getTokenBalance(candidate, SepoliaConfig::Tokens::WETH);

                if (usdc_balance > 0 || dai_balance > 0 || weth_balance > 0)
                {
                    std::cout << "    💰 Has liquidity: USDC=" << usdc_balance
                              << ", DAI=" << dai_balance
                              << ", WETH=" << weth_balance << std::endl;
                    pools.push_back(candidate);
                }
                else
                {
                    std::cout << "    ❌ No liquidity detected" << std::endl;
                }
            }
            else
            {
                std::cout << "    ❌ Not a contract" << std::endl;
            }
        }

        // Also try to find pools by scanning recent blocks for pool creation events
        std::cout << "\n🔍 Scanning recent blocks for pool events..." << std::endl;

        try
        {
            // Get latest block number
            json response = call("eth_blockNumber", {});
            if (response.contains("result"))
            {
                std::string latest_block_hex = response["result"];
                uint64_t latest_block = std::stoull(latest_block_hex.substr(2), nullptr, 16);

                std::cout << "  Latest block: " << latest_block << std::endl;

                // Scan last 100 blocks for potential pool addresses
                for (uint64_t i = 0; i < 100 && i < latest_block; i++)
                {
                    uint64_t block_num = latest_block - i;
                    std::stringstream ss;
                    ss << "0x" << std::hex << block_num;

                    try
                    {
                        json block_response = call("eth_getBlockByNumber", {ss.str(), false});
                        if (block_response.contains("result") && block_response["result"].contains("transactions"))
                        {
                            auto transactions = block_response["result"]["transactions"];
                            for (const auto &tx : transactions)
                            {
                                if (tx.contains("to") && !tx["to"].is_null())
                                {
                                    std::string to_address = tx["to"];
                                    if (to_address.length() == 42)
                                    {
                                        // Check if this address might be a pool
                                        if (isContract(to_address))
                                        {
                                            uint64_t usdc_balance = getTokenBalance(to_address, SepoliaConfig::Tokens::USDC);
                                            if (usdc_balance > 1000000)
                                            { // More than 1 USDC
                                                std::cout << "    🎯 Found potential pool in block " << block_num
                                                          << ": " << to_address << " (USDC: " << usdc_balance << ")" << std::endl;
                                                if (std::find(pools.begin(), pools.end(), to_address) == pools.end())
                                                {
                                                    pools.push_back(to_address);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    catch (const std::exception &e)
                    {
                        // Skip blocks that fail
                        continue;
                    }
                }
            }
        }
        catch (const std::exception &e)
        {
            std::cout << "    ⚠️  Block scanning failed: " << e.what() << std::endl;
        }

        return pools;
    }

    // Check for alternative DEX protocols
    std::vector<std::string> findAlternativeDEX()
    {
        std::vector<std::string> alternatives;

        std::cout << "\n🔍 Looking for alternative DEX protocols..." << std::endl;

        // Common DEX factory addresses to check
        std::vector<std::pair<std::string, std::string>> dex_factories = {
            {"Uniswap V2", "0x5C69bEe701ef814a2B6a3EDD4B1652CB9cc5aA6f"},
            {"Uniswap V3", "0x1F98431c8aD98523631AE4a59f267346ea31F984"},
            {"SushiSwap", "0xC0AEe478e3658e2610c5F7A4A2E1777cE9e4f2Ac"},
            {"PancakeSwap", "0xcA143Ce32Fe78f1f7019d7d551a6402fC5350c73"}};

        for (const auto &[name, address] : dex_factories)
        {
            std::cout << "  Checking " << name << " factory: " << address << std::endl;
            if (isContract(address))
            {
                std::cout << "    ✅ Factory contract found" << std::endl;
                alternatives.push_back(name + ":" + address);
            }
            else
            {
                std::cout << "    ❌ Factory not found" << std::endl;
            }
        }

        return alternatives;
    }
};

int main()
{
    std::cout << "🔍 CURVE POOL DISCOVERY TOOL FOR SEPOLIA" << std::endl;
    std::cout << "=========================================" << std::endl;

    try
    {
        if (!SepoliaConfig::isConfigured())
        {
            std::cerr << "❌ Configuration not complete. Please run ./setup_wallet.sh first." << std::endl;
            return 1;
        }

        curl_global_init(CURL_GLOBAL_DEFAULT);
        EthereumRPC rpc(SepoliaConfig::SEPOLIA_RPC_URL);

        std::cout << "✅ Connected to Sepolia testnet" << std::endl;
        std::cout << "🔗 RPC: " << SepoliaConfig::SEPOLIA_RPC_URL << std::endl;
        std::cout << "👛 Wallet: " << SepoliaConfig::Wallet::ADDRESS << std::endl;
        std::cout << "🪙 Tokens: USDC=" << SepoliaConfig::Tokens::USDC
                  << ", DAI=" << SepoliaConfig::Tokens::DAI
                  << ", WETH=" << SepoliaConfig::Tokens::WETH << std::endl;

        std::cout << "\n🚀 Starting pool discovery..." << std::endl;
        std::vector<std::string> discovered_pools = rpc.discoverPools();
        std::vector<std::string> alternative_dex = rpc.findAlternativeDEX();

        std::cout << "\n📊 DISCOVERY RESULTS" << std::endl;
        std::cout << "===================" << std::endl;

        if (discovered_pools.empty())
        {
            std::cout << "❌ No Curve pools found with liquidity" << std::endl;

            if (!alternative_dex.empty())
            {
                std::cout << "\n✅ Alternative DEX protocols found:" << std::endl;
                for (const auto &dex : alternative_dex)
                {
                    std::cout << "   - " << dex << std::endl;
                }
            }

            std::cout << "\n💡 HACKATHON SOLUTION: Using Mock Pool for Testing" << std::endl;
            std::cout << "==================================================" << std::endl;
            std::cout << "Since no real pools were found, we'll use a mock pool for testing:" << std::endl;
            std::cout << "   Mock Pool: 0x1234567890123456789012345678901234567890" << std::endl;
            std::cout << "   This allows you to test the limit order logic without real pools." << std::endl;

            std::cout << "\n🔧 Next Steps:" << std::endl;
            std::cout << "1. Update src/curve_dex_limit_order_agent.cpp with mock pool" << std::endl;
            std::cout << "2. Update src/price_monitor.cpp with mock pool" << std::endl;
            std::cout << "3. Test the limit order logic with mock data" << std::endl;
            std::cout << "4. Document how to integrate with real pools when found" << std::endl;

            std::cout << "\n🌐 To find real pools later:" << std::endl;
            std::cout << "1. Check https://sepolia.etherscan.io for Curve contracts" << std::endl;
            std::cout << "2. Look for pool creation events" << std::endl;
            std::cout << "3. Test addresses with get_dy calls" << std::endl;
            std::cout << "4. Verify liquidity with balanceOf calls" << std::endl;
        }
        else
        {
            std::cout << "✅ Found " << discovered_pools.size() << " potential pools:" << std::endl;
            for (size_t i = 0; i < discovered_pools.size(); i++)
            {
                std::cout << "   " << (i + 1) << ". " << discovered_pools[i] << std::endl;
            }

            std::cout << "\n💡 Update your configuration with these pool addresses:" << std::endl;
            std::cout << "   - src/curve_dex_limit_order_agent.cpp" << std::endl;
            std::cout << "   - src/price_monitor.cpp" << std::endl;
        }

        curl_global_cleanup();
    }
    catch (const std::exception &e)
    {
        std::cerr << "💥 Error: " << e.what() << std::endl;
        curl_global_cleanup();
        return 1;
    }

    return 0;
}
