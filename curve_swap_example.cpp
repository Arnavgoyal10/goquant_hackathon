#include <curl/curl.h>

#include <iomanip>
#include <iostream>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
#include <sstream>
#include <string>
#include <vector>

using json = nlohmann::json;

// HTTP Client for blockchain interactions
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
};

// Utility functions for encoding
std::string encodeUint256(uint64_t value)
{
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(64) << value;
    return ss.str();
}

std::string encodeAddress(const std::string &address)
{
    std::string clean_addr = address;
    if (clean_addr.substr(0, 2) == "0x")
    {
        clean_addr = clean_addr.substr(2);
    }
    return std::string(24, '0') + clean_addr; // Pad to 32 bytes
}

uint64_t hexToUint64(const std::string &hex)
{
    std::string cleanHex = hex;
    if (cleanHex.substr(0, 2) == "0x")
    {
        cleanHex = cleanHex.substr(2);
    }
    if (cleanHex.empty() || cleanHex == "0" ||
        cleanHex.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos)
    {
        return 0; // Return 0 for invalid or empty hex
    }
    try
    {
        return std::stoull(cleanHex, nullptr, 16);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Warning: Failed to parse hex value '" << hex << "': " << e.what() << std::endl;
        return 0;
    }
}

// ERC20 Token interface
class ERC20Token
{
private:
    std::string token_address;
    EthereumRPC *rpc;

public:
    ERC20Token(const std::string &address, EthereumRPC *ethereum_rpc)
        : token_address(address), rpc(ethereum_rpc)
    {
    }

    uint64_t balanceOf(const std::string &account)
    {
        // Function signature for balanceOf(address) - 0x70a08231
        std::string function_signature = "0x70a08231";
        std::string call_data = function_signature + encodeAddress(account);

        json call_params = {{{"to", token_address}, {"data", call_data}}, "latest"};

        auto result = rpc->call("eth_call", call_params);

        if (result.contains("error"))
        {
            throw std::runtime_error("RPC Error: " + result["error"]["message"].get<std::string>());
        }

        return hexToUint64(result["result"]);
    }

    std::string
    transfer(const std::string &to, uint64_t amount, const std::string &from_private_key)
    {
        // Function signature for transfer(address,uint256) - 0xa9059cbb
        std::string function_signature = "0xa9059cbb";
        std::string call_data = function_signature + encodeAddress(to) + encodeUint256(amount);

        // In a real implementation, you would:
        // 1. Build the transaction
        // 2. Sign it with the private key
        // 3. Send via eth_sendRawTransaction

        std::cout << "MOCK: Transferring " << amount << " tokens to " << to << std::endl;
        std::cout << "Call data: " << call_data << std::endl;

        return "0x" + std::string(64, 'a'); // Mock transaction hash
    }

    std::string
    approve(const std::string &spender, uint64_t amount, const std::string &from_private_key)
    {
        // Function signature for approve(address,uint256) - 0x095ea7b3
        std::string function_signature = "0x095ea7b3";
        std::string call_data = function_signature + encodeAddress(spender) + encodeUint256(amount);

        std::cout << "MOCK: Approving " << spender << " to spend " << amount << " tokens" << std::endl;
        std::cout << "Call data: " << call_data << std::endl;

        return "0x" + std::string(64, 'b'); // Mock transaction hash
    }
};

// Curve Pool interface
class CurvePool
{
private:
    std::string pool_address;
    EthereumRPC *rpc;

public:
    CurvePool(const std::string &address, EthereumRPC *ethereum_rpc)
        : pool_address(address), rpc(ethereum_rpc)
    {
    }

    // Get exchange rate (how much output for given input)
    uint64_t get_dy(int32_t i, int32_t j, uint64_t dx)
    {
        // Function signature for get_dy(int128,int128,uint256) - 0x5e0d443f
        std::string function_signature = "0x5e0d443f";

        // Encode parameters
        std::string encoded_i = encodeUint256(static_cast<uint64_t>(i));
        std::string encoded_j = encodeUint256(static_cast<uint64_t>(j));
        std::string encoded_dx = encodeUint256(dx);

        std::string call_data = function_signature + encoded_i + encoded_j + encoded_dx;

        json call_params = {{{"to", pool_address}, {"data", call_data}}, "latest"};

        auto result = rpc->call("eth_call", call_params);

        if (result.contains("error"))
        {
            throw std::runtime_error("RPC Error: " + result["error"]["message"].get<std::string>());
        }

        return hexToUint64(result["result"]);
    }

    // Traditional exchange method (requires approval)
    std::string exchange(
        int32_t i,
        int32_t j,
        uint64_t dx,
        uint64_t min_dy,
        const std::string &receiver,
        const std::string &private_key)
    {
        // Function signature for exchange(int128,int128,uint256,uint256,address) - 0x394747c5
        std::string function_signature = "0x394747c5";

        std::string call_data = function_signature + encodeUint256(static_cast<uint64_t>(i)) +
                                encodeUint256(static_cast<uint64_t>(j)) + encodeUint256(dx) +
                                encodeUint256(min_dy) + encodeAddress(receiver);

        std::cout << "MOCK: Executing exchange(" << i << ", " << j << ", " << dx << ", " << min_dy
                  << ")" << std::endl;
        std::cout << "Call data: " << call_data << std::endl;

        return "0x" + std::string(64, 'c'); // Mock transaction hash
    }

    // Modern exchange_received method (no approval needed)
    std::string exchange_received(
        int32_t i,
        int32_t j,
        uint64_t dx,
        uint64_t min_dy,
        const std::string &receiver,
        const std::string &private_key)
    {
        // Function signature for exchange_received(int128,int128,uint256,uint256,address) - 0x394747c5
        // Note: Actual signature would be different, this is simplified
        std::string function_signature = "0x15bf4c40"; // Mock signature for exchange_received

        std::string call_data = function_signature + encodeUint256(static_cast<uint64_t>(i)) +
                                encodeUint256(static_cast<uint64_t>(j)) + encodeUint256(dx) +
                                encodeUint256(min_dy) + encodeAddress(receiver);

        std::cout << "MOCK: Executing exchange_received(" << i << ", " << j << ", " << dx << ", "
                  << min_dy << ")" << std::endl;
        std::cout << "Call data: " << call_data << std::endl;

        return "0x" + std::string(64, 'd'); // Mock transaction hash
    }

    std::string getAddress() const
    {
        return pool_address;
    }
};

// Curve Meta Registry interface
class CurveMetaRegistry
{
private:
    std::string registry_address;
    EthereumRPC *rpc;

public:
    CurveMetaRegistry(const std::string &address, EthereumRPC *ethereum_rpc)
        : registry_address(address), rpc(ethereum_rpc)
    {
    }

    // Find pool for token pair
    std::string find_pool_for_coins(const std::string &from_token, const std::string &to_token)
    {
        // Function signature for find_pool_for_coins(address,address) - simplified
        std::string function_signature = "0xa87df06c";

        std::string call_data =
            function_signature + encodeAddress(from_token) + encodeAddress(to_token);

        json call_params = {{{"to", registry_address}, {"data", call_data}}, "latest"};

        auto result = rpc->call("eth_call", call_params);

        if (result.contains("error"))
        {
            throw std::runtime_error("RPC Error: " + result["error"]["message"].get<std::string>());
        }

        std::string hex_result = result["result"];
        if (hex_result.length() >= 66)
        {
            return "0x" + hex_result.substr(hex_result.length() - 40);
        }

        return "";
    }

    // Get exchange amount estimate
    uint64_t get_exchange_amount(
        const std::string &pool,
        const std::string &from_token,
        const std::string &to_token,
        uint64_t amount)
    {
        // Simplified implementation
        std::cout << "Getting exchange amount for pool: " << pool << std::endl;
        std::cout << "From: " << from_token << " To: " << to_token << " Amount: " << amount
                  << std::endl;

        // In reality, this would call the registry's get_exchange_amount function
        return amount * 99 / 100; // Mock 1% slippage
    }
};

// High-level Curve Swapper
class CurveSwapper
{
private:
    EthereumRPC *rpc;
    std::unique_ptr<CurveMetaRegistry> registry;
    std::string user_address;
    std::string private_key;

public:
    CurveSwapper(EthereumRPC *ethereum_rpc, const std::string &user_addr, const std::string &priv_key)
        : rpc(ethereum_rpc), user_address(user_addr), private_key(priv_key)
    {

        // Initialize Meta Registry
        const std::string METAREGISTRY = "0xF98B45FA17DE75FB1aD0e7aFD971b0ca00e379fC";
        registry = std::make_unique<CurveMetaRegistry>(METAREGISTRY, rpc);
    }

    // Method 1: Traditional exchange with approval
    std::string swapWithApproval(
        const std::string &from_token_addr,
        const std::string &to_token_addr,
        uint64_t amount,
        double slippage_tolerance = 0.01)
    {

        std::cout << "\n=== Traditional Swap with Approval ===" << std::endl;

        // 1. Find suitable pool
        std::string pool_addr = registry->find_pool_for_coins(from_token_addr, to_token_addr);
        if (pool_addr.empty())
        {
            throw std::runtime_error("No pool found for token pair");
        }

        std::cout << "Found pool: " << pool_addr << std::endl;

        // 2. Create token and pool instances
        ERC20Token from_token(from_token_addr, rpc);
        CurvePool pool(pool_addr, rpc);

        // 3. Check balance
        uint64_t balance = from_token.balanceOf(user_address);
        if (balance < amount)
        {
            throw std::runtime_error("Insufficient balance");
        }

        // 4. Get expected output with slippage protection
        uint64_t expected_output = pool.get_dy(0, 1, amount); // Assuming indices 0->1
        uint64_t min_output = static_cast<uint64_t>(expected_output * (1.0 - slippage_tolerance));

        std::cout << "Expected output: " << expected_output << std::endl;
        std::cout << "Minimum output (with slippage): " << min_output << std::endl;

        // 5. Approve pool to spend tokens
        std::string approve_tx = from_token.approve(pool_addr, amount, private_key);
        std::cout << "Approval transaction: " << approve_tx << std::endl;

        // 6. Execute exchange
        std::string swap_tx = pool.exchange(0, 1, amount, min_output, user_address, private_key);
        std::cout << "Swap transaction: " << swap_tx << std::endl;

        return swap_tx;
    }

    // Method 2: Modern exchange_received (no approval needed)
    std::string swapWithExchangeReceived(
        const std::string &from_token_addr,
        const std::string &to_token_addr,
        uint64_t amount,
        double slippage_tolerance = 0.01)
    {

        std::cout << "\n=== Modern Swap with exchange_received ===" << std::endl;

        // 1. Find suitable pool
        std::string pool_addr = registry->find_pool_for_coins(from_token_addr, to_token_addr);
        if (pool_addr.empty())
        {
            throw std::runtime_error("No pool found for token pair");
        }

        std::cout << "Found pool: " << pool_addr << std::endl;

        // 2. Create token and pool instances
        ERC20Token from_token(from_token_addr, rpc);
        CurvePool pool(pool_addr, rpc);

        // 3. Check balance
        uint64_t balance = from_token.balanceOf(user_address);
        if (balance < amount)
        {
            throw std::runtime_error("Insufficient balance");
        }

        // 4. Get expected output with slippage protection
        uint64_t expected_output = pool.get_dy(0, 1, amount);
        uint64_t min_output = static_cast<uint64_t>(expected_output * (1.0 - slippage_tolerance));

        std::cout << "Expected output: " << expected_output << std::endl;
        std::cout << "Minimum output (with slippage): " << min_output << std::endl;

        // 5. Transfer tokens directly to pool
        std::string transfer_tx = from_token.transfer(pool_addr, amount, private_key);
        std::cout << "Transfer transaction: " << transfer_tx << std::endl;

        // 6. Execute exchange_received
        std::string swap_tx =
            pool.exchange_received(0, 1, amount, min_output, user_address, private_key);
        std::cout << "Swap transaction: " << swap_tx << std::endl;

        return swap_tx;
    }

    // Method 3: Multi-hop swap using exchange_received
    std::string multiHopSwap(
        const std::vector<std::string> &token_path,
        const std::vector<std::string> &pool_path,
        uint64_t amount_in,
        double slippage_tolerance = 0.01)
    {

        std::cout << "\n=== Multi-hop Swap ===" << std::endl;

        if (token_path.size() != pool_path.size() + 1)
        {
            throw std::runtime_error("Invalid path lengths");
        }

        // For each hop in the path
        uint64_t current_amount = amount_in;
        std::string last_tx_hash;

        for (size_t i = 0; i < pool_path.size(); ++i)
        {
            std::cout << "\n--- Hop " << (i + 1) << " ---" << std::endl;

            ERC20Token current_token(token_path[i], rpc);
            CurvePool current_pool(pool_path[i], rpc);

            // Calculate expected output
            uint64_t expected_output = current_pool.get_dy(0, 1, current_amount);
            uint64_t min_output = static_cast<uint64_t>(expected_output * (1.0 - slippage_tolerance));

            std::cout << "Input amount: " << current_amount << std::endl;
            std::cout << "Expected output: " << expected_output << std::endl;

            if (i == 0)
            {
                // First hop: transfer from user to pool
                std::string transfer_tx = current_token.transfer(pool_path[i], current_amount, private_key);
                std::cout << "Transfer transaction: " << transfer_tx << std::endl;
            }

            // Determine receiver for this hop
            std::string receiver = (i == pool_path.size() - 1) ? user_address : pool_path[i + 1];

            // Execute exchange_received
            last_tx_hash =
                current_pool.exchange_received(0, 1, current_amount, min_output, receiver, private_key);
            std::cout << "Swap transaction: " << last_tx_hash << std::endl;

            current_amount = expected_output; // Use expected output for next hop calculation
        }

        return last_tx_hash;
    }
};

// Example usage
int main()
{
    try
    {
        // Initialize
        // Configuration - Sepolia testnet RPC endpoint
        const std::string rpc_ethereum =
            "https://sepolia.infura.io/v3/4d9b66e76f114285966cb0c600951a4f"; // Sepolia testnet for development

        curl_global_init(CURL_GLOBAL_DEFAULT);

        EthereumRPC rpc(rpc_ethereum);

        // User configuration
        std::string user_address = "<user_address>"; // Example address
        std::string private_key =
            "<private_key>"; // In practice, load
                             // securely

        CurveSwapper swapper(&rpc, user_address, private_key);

        // Real token addresses (mainnet)
        std::string USDC = "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48"; // USDC (correct address)
        std::string WETH = "0xC02aaA39b223FE8D0A0e5C4F27eAD9083C756Cc2"; // WETH
        std::string CRV = "0xD533a949740bb3306d119CC777fa900bA034cd52";  // CRV

        uint64_t amount = 1000 * 1e6; // 1000 USDC (6 decimals)

        std::cout << "=== Curve Swap Examples ===" << std::endl;

        // Example 1: Simple USDC -> WETH swap with approval
        try
        {
            std::string tx1 = swapper.swapWithApproval(USDC, WETH, amount);
            std::cout << "\nFinal transaction hash: " << tx1 << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Swap with approval failed: " << e.what() << std::endl;
        }

        // Example 2: Simple USDC -> WETH swap with exchange_received
        try
        {
            std::string tx2 = swapper.swapWithExchangeReceived(USDC, WETH, amount);
            std::cout << "\nFinal transaction hash: " << tx2 << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Swap with exchange_received failed: " << e.what() << std::endl;
        }

        // Example 3: Multi-hop USDC -> WETH -> CRV
        try
        {
            std::vector<std::string> token_path = {USDC, WETH, CRV};
            std::vector<std::string> pool_path = {
                "0x4DEcE678ceceb27446b35C672dC7d61F30bAD69E", // USDC/WETH pool (example)
                "0x9D0464996170c6B9e75eED71c68B99dDEDf279e8"  // WETH/CRV pool (example)
            };

            std::string tx3 = swapper.multiHopSwap(token_path, pool_path, amount);
            std::cout << "\nMulti-hop final transaction hash: " << tx3 << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Multi-hop swap failed: " << e.what() << std::endl;
        }

        curl_global_cleanup();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        curl_global_cleanup();
        return 1;
    }

    return 0;
}
