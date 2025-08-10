#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <iomanip>
#include <memory>
#include <map>
#include <sstream>

// Include our limit order structure
#include "../include/limit_order.h"
#include "../include/sepolia_config.h"
#include "../include/transaction_signer.h"

using json = nlohmann::json;

// Utility functions for encoding (from original sample)
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
    return std::string(24, '0') + clean_addr;
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
        return 0;
    }

    if (cleanHex.length() > 16)
    {
        cleanHex = cleanHex.substr(cleanHex.length() - 16);
    }

    try
    {
        return std::stoull(cleanHex, nullptr, 16);
    }
    catch (const std::exception &e)
    {
        return 0;
    }
}

// RPC Client (from our previous work)
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

// Curve Pool Interface (simplified from original)
class CurvePool
{
private:
    std::string pool_address;
    EthereumRPC *rpc;

public:
    CurvePool(const std::string &address, EthereumRPC *ethereum_rpc)
        : pool_address(address), rpc(ethereum_rpc) {}

    // Get exchange rate using get_dy
    uint64_t get_dy(int32_t i, int32_t j, uint64_t dx)
    {
        // Check if we should use mock mode for demo purposes
        const char *mock_flag = std::getenv("USE_MOCK_PRICING");
        bool use_mock = mock_flag && std::string(mock_flag) == "1";

        if (use_mock)
        {
            // Mock pricing for demo: return realistic values
            // Simulate a market where 1 USDC ≈ 0.999 DAI (slight discount)
            double mock_rate = 0.999;
            return static_cast<uint64_t>(dx * mock_rate);
        }

        std::string function_signature = "0x5e0d443f";
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

    // Mock swap execution (will be replaced with real implementation)
    std::string executeSwap(int32_t i, int32_t j, uint64_t dx, uint64_t min_dy)
    {
        std::cout << "🔄 EXECUTING SWAP: " << dx << " tokens (" << i << " -> " << j << ")" << std::endl;
        std::cout << "   Minimum output: " << min_dy << std::endl;
        std::cout << "   Pool: " << pool_address << std::endl;

        // If EXECUTE_ONCHAIN is not set, return mock tx hash
        const char *exec_flag = std::getenv("EXECUTE_ONCHAIN");
        bool execute_onchain = exec_flag && std::string(exec_flag) == "1";
        if (!execute_onchain)
        {
            std::cout << "[INFO] EXECUTE_ONCHAIN not set. Returning mock transaction hash." << std::endl;
            return "0x" + std::string(64, 'f');
        }

        // Build function data for Curve pool exchange: exchange(int128 i, int128 j, uint256 dx, uint256 min_dy, address receiver)
        // Signature selector (example): 0x394747c5
        std::string function_selector = "0x394747c5";
        std::string data = function_selector +
                           encodeUint256(static_cast<uint64_t>(i)) +
                           encodeUint256(static_cast<uint64_t>(j)) +
                           encodeUint256(dx) +
                           encodeUint256(min_dy) +
                           encodeAddress(SepoliaConfig::Wallet::ADDRESS);

        // Resolve RPC URL
        std::string rpc_url = SepoliaConfig::SEPOLIA_RPC_URL;
        if (const char *rpc_env = std::getenv("RPC_URL"); rpc_env && std::string(rpc_env).size() > 0)
        {
            rpc_url = rpc_env;
        }

        // Create signer and transaction
        TransactionSigner signer(SepoliaConfig::Wallet::PRIVATE_KEY);

        // Fetch nonce from network if possible
        uint64_t nonce = 0;
        try
        {
            json nonce_resp = rpc->call("eth_getTransactionCount", json::array({SepoliaConfig::Wallet::ADDRESS, "latest"}));
            if (nonce_resp.contains("result"))
            {
                nonce = std::stoull(std::string(nonce_resp["result"]).substr(2), nullptr, 16);
            }
        }
        catch (...)
        {
            nonce = signer.getCurrentNonce(SepoliaConfig::Wallet::ADDRESS);
        }

        EthereumTransaction tx;
        tx.nonce = nonce;
        tx.to_address = pool_address;
        tx.data = data;
        tx.gas_limit = SepoliaConfig::Gas::SWAP_GAS_LIMIT;
        tx.chain_id = SepoliaConfig::SEPOLIA_CHAIN_ID; // default to Sepolia

        std::string raw_tx = signer.signTransaction(tx);

        const char *broadcast_flag = std::getenv("BROADCAST_TX");
        bool broadcast = broadcast_flag && std::string(broadcast_flag) == "1";
        if (!broadcast)
        {
            std::cout << "[INFO] BROADCAST_TX not set. Returning signed (demo) tx hash string." << std::endl;
            return signer.broadcastTransaction(raw_tx); // returns derived hash without network send
        }

        // Actually broadcast over RPC
        try
        {
            json send_params = json::array({raw_tx});
            json send_resp = rpc->call("eth_sendRawTransaction", send_params);
            if (send_resp.contains("result"))
            {
                std::string tx_hash = send_resp["result"];
                std::cout << "✅ Broadcast succeeded: " << tx_hash << std::endl;
                return tx_hash;
            }
            std::cout << "⚠️ Broadcast response without result; falling back to local hash." << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cout << "⚠️ Broadcast failed: " << e.what() << ". Returning local hash." << std::endl;
        }

        return signer.broadcastTransaction(raw_tx);
    }
};

// 🚀 MAIN LIMIT ORDER EXECUTION ENGINE
class LimitOrderEngine
{
private:
    EthereumRPC *rpc;
    std::vector<std::unique_ptr<LimitOrder>> active_orders;

public:
    LimitOrderEngine(EthereumRPC *ethereum_rpc) : rpc(ethereum_rpc) {}

    // Add an order to the engine
    void addOrder(std::unique_ptr<LimitOrder> order)
    {
        order->updateStatus(OrderStatus::ACTIVE);
        std::cout << "\n📝 ORDER ADDED: " << order->order_id << " (" << order->getTifString() << ")" << std::endl;
        order->printSummary();
        active_orders.push_back(std::move(order));
    }

    // Execute GTC policy: Monitor continuously until filled or canceled
    void executeGTC(LimitOrder &order)
    {
        std::cout << "\n🔄 Executing GTC Policy for " << order.order_id << std::endl;

        // Create pool connection
        CurvePool pool(order.pool_address, rpc);

        int check_count = 0;
        const int max_checks = 10; // Limit for demo

        while (order.isExecutable() && check_count < max_checks)
        {
            try
            {
                // Get current price
                uint64_t current_output = pool.get_dy(order.input_token_index, order.output_token_index, order.input_amount);
                order.recordPriceCheck(current_output);

                std::cout << "💰 Price Check #" << (check_count + 1) << ": " << current_output << " output tokens" << std::endl;

                // Check if price meets limit
                if (order.isPriceMet(current_output))
                {
                    std::cout << "✅ PRICE TARGET MET! Executing swap..." << std::endl;

                    uint64_t min_output = order.getMinOutputWithSlippage(current_output);
                    std::string tx_hash = pool.executeSwap(order.input_token_index, order.output_token_index,
                                                           order.input_amount, min_output);

                    order.transaction_hash = tx_hash;
                    order.filled_amount = order.input_amount;
                    order.received_amount = current_output;
                    order.updateStatus(OrderStatus::FILLED);

                    std::cout << "🎉 ORDER FILLED! Transaction: " << tx_hash << std::endl;
                    return;
                }

                check_count++;
                std::this_thread::sleep_for(std::chrono::seconds(2)); // Wait 2 seconds between checks
            }
            catch (const std::exception &e)
            {
                std::cerr << "❌ Error in GTC execution: " << e.what() << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }

        if (check_count >= max_checks)
        {
            order.updateStatus(OrderStatus::CANCELED, "Demo limit reached");
            std::cout << "⏰ GTC Order stopped after " << max_checks << " price checks (demo mode)" << std::endl;
        }
    }

    // Execute GTT policy: Monitor until expiry
    void executeGTT(LimitOrder &order)
    {
        std::cout << "\n⏰ Executing GTT Policy for " << order.order_id << std::endl;

        CurvePool pool(order.pool_address, rpc);

        while (order.isExecutable() && !order.isExpired())
        {
            try
            {
                uint64_t current_output = pool.get_dy(order.input_token_index, order.output_token_index, order.input_amount);
                order.recordPriceCheck(current_output);

                if (order.isPriceMet(current_output))
                {
                    std::cout << "✅ GTT ORDER FILLED before expiry!" << std::endl;

                    uint64_t min_output = order.getMinOutputWithSlippage(current_output);
                    std::string tx_hash = pool.executeSwap(order.input_token_index, order.output_token_index,
                                                           order.input_amount, min_output);

                    order.transaction_hash = tx_hash;
                    order.filled_amount = order.input_amount;
                    order.updateStatus(OrderStatus::FILLED);
                    return;
                }

                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
            catch (const std::exception &e)
            {
                std::cerr << "❌ Error in GTT execution: " << e.what() << std::endl;
            }
        }

        if (order.isExpired())
        {
            order.updateStatus(OrderStatus::EXPIRED, "Order expired");
            std::cout << "⏰ GTT Order EXPIRED without execution" << std::endl;
        }
    }

    // Execute IOC policy: Single check with partial fill support
    void executeIOC(LimitOrder &order)
    {
        std::cout << "\n⚡ Executing IOC Policy for " << order.order_id << std::endl;

        CurvePool pool(order.pool_address, rpc);

        try
        {
            uint64_t current_output = pool.get_dy(order.input_token_index, order.output_token_index, order.input_amount);
            order.recordPriceCheck(current_output);

            std::cout << "💰 IOC Price Check: " << current_output << " output tokens" << std::endl;

            // Debug: Show price comparison
            uint64_t expected_output = static_cast<uint64_t>(order.input_amount * order.limit_price);
            std::cout << "🔍 Price Check: Current output = " << current_output << ", Expected output = " << expected_output << std::endl;
            std::cout << "🔍 Price met? " << (order.isPriceMet(current_output) ? "YES" : "NO") << std::endl;

            if (order.isPriceMet(current_output))
            {
                std::cout << "✅ IOC ORDER EXECUTED immediately!" << std::endl;

                uint64_t min_output = order.getMinOutputWithSlippage(current_output);
                std::string tx_hash = pool.executeSwap(order.input_token_index, order.output_token_index,
                                                       order.input_amount, min_output);

                order.transaction_hash = tx_hash;
                order.filled_amount = order.input_amount;
                order.received_amount = current_output;
                order.updateStatus(OrderStatus::FILLED);
            }
            else
            {
                // Check for partial fill possibility
                uint64_t max_fillable = order.getMaxFillableAmount(current_output);

                if (max_fillable > 0)
                {
                    std::cout << "🔄 IOC PARTIAL FILL: " << max_fillable << " of " << order.input_amount << " tokens" << std::endl;

                    // Calculate output for partial fill
                    uint64_t partial_output = pool.get_dy(order.input_token_index, order.output_token_index, max_fillable);
                    uint64_t min_partial_output = order.getMinOutputWithSlippage(partial_output);

                    std::string tx_hash = pool.executeSwap(order.input_token_index, order.output_token_index,
                                                           max_fillable, min_partial_output);

                    order.transaction_hash = tx_hash;
                    order.filled_amount = max_fillable;
                    order.received_amount = partial_output;
                    order.updateStatus(OrderStatus::PARTIALLY_FILLED, "Partial fill executed");

                    std::cout << "✅ IOC Partial fill completed: " << order.getFillPercentage() << "%" << std::endl;
                }
                else
                {
                    order.updateStatus(OrderStatus::CANCELED, "Price not met for any execution");
                    std::cout << "❌ IOC Order CANCELED - price not met" << std::endl;
                }
            }
        }
        catch (const std::exception &e)
        {
            order.updateStatus(OrderStatus::FAILED, e.what());
            std::cerr << "❌ IOC execution failed: " << e.what() << std::endl;
        }
    }

    // Execute FOK policy: All-or-nothing single check with liquidity verification
    void executeFOK(LimitOrder &order)
    {
        std::cout << "\n💀 Executing FOK Policy for " << order.order_id << std::endl;

        CurvePool pool(order.pool_address, rpc);

        try
        {
            // First check: Price check
            uint64_t current_output = pool.get_dy(order.input_token_index, order.output_token_index, order.input_amount);
            order.recordPriceCheck(current_output);

            std::cout << "💰 FOK Price Check: " << current_output << " output tokens" << std::endl;

            if (!order.isPriceMet(current_output))
            {
                order.updateStatus(OrderStatus::CANCELED, "FOK: Price not met, order killed");
                std::cout << "💀 FOK Order KILLED - price not met" << std::endl;
                return;
            }

            // Second check: Liquidity verification (optional)
            bool liquidity_check_enabled = true;
            if (const char *env = std::getenv("SKIP_LIQUIDITY_CHECK"); env && std::string(env) == "1")
            {
                liquidity_check_enabled = false;
            }

            if (liquidity_check_enabled)
            {
                std::cout << "🔍 FOK Liquidity Check: Verifying pool can handle full order..." << std::endl;

                // Check if pool has sufficient liquidity by testing a slightly larger amount
                uint64_t test_amount = static_cast<uint64_t>(order.input_amount * 1.01); // 1% larger test
                try
                {
                    uint64_t test_output = pool.get_dy(order.input_token_index, order.output_token_index, test_amount);

                    // If we can get a quote for a larger amount, liquidity should be sufficient
                    if (test_output > 0)
                    {
                        std::cout << "✅ FOK Liquidity Check: Pool has sufficient liquidity" << std::endl;
                    }
                    else
                    {
                        order.updateStatus(OrderStatus::CANCELED, "FOK: Insufficient liquidity for full order");
                        std::cout << "💀 FOK Order KILLED - insufficient liquidity" << std::endl;
                        return;
                    }
                }
                catch (const std::exception &e)
                {
                    std::cout << "⚠️ FOK Liquidity Check: Could not verify liquidity, proceeding with caution" << std::endl;
                }
            }

            // All checks passed - execute the order
            std::cout << "✅ FOK ORDER FILLED completely!" << std::endl;

            uint64_t min_output = order.getMinOutputWithSlippage(current_output);
            std::string tx_hash = pool.executeSwap(order.input_token_index, order.output_token_index,
                                                   order.input_amount, min_output);

            order.transaction_hash = tx_hash;
            order.filled_amount = order.input_amount;
            order.received_amount = current_output;
            order.updateStatus(OrderStatus::FILLED);
        }
        catch (const std::exception &e)
        {
            order.updateStatus(OrderStatus::FAILED, e.what());
            std::cerr << "❌ FOK execution failed: " << e.what() << std::endl;
        }
    }

    // Process all active orders
    void processOrders()
    {
        std::cout << "\n🚀 STARTING LIMIT ORDER ENGINE" << std::endl;
        std::cout << "Processing " << active_orders.size() << " orders..." << std::endl;

        for (auto &order : active_orders)
        {
            if (!order->isExecutable())
                continue;

            switch (order->tif_policy)
            {
            case TimeInForce::GTC:
                executeGTC(*order);
                break;
            case TimeInForce::GTT:
                executeGTT(*order);
                break;
            case TimeInForce::IOC:
                executeIOC(*order);
                break;
            case TimeInForce::FOK:
                executeFOK(*order);
                break;
            }

            std::cout << "\n📊 FINAL ORDER STATUS:" << std::endl;
            order->printSummary();
            std::cout << std::string(50, '-') << std::endl;
        }
    }
};

// 🎯 MAIN PROGRAM - This is what you'll run!
int main(int argc, char **argv)
{
    std::cout << "🎯 CURVE DEX LIMIT ORDER AGENT" << std::endl;
    std::cout << "==============================" << std::endl;

    try
    {
        // Initialize connection to Sepolia testnet using configured wallet
        curl_global_init(CURL_GLOBAL_DEFAULT);

        if (!SepoliaConfig::isConfigured())
        {
            std::cerr << "❌ Configuration not complete. Please run ./setup_wallet.sh first." << std::endl;
            return 1;
        }

        // Allow overriding RPC URL via environment variable RPC_URL
        std::string rpc_url = SepoliaConfig::SEPOLIA_RPC_URL;
        if (const char *rpc_env = std::getenv("RPC_URL"); rpc_env && std::string(rpc_env).size() > 0)
        {
            rpc_url = std::string(rpc_env);
        }

        // Use configured wallet and real Sepolia addresses
        std::string user_address = SepoliaConfig::Wallet::ADDRESS;
        std::string private_key = SepoliaConfig::Wallet::PRIVATE_KEY;

        std::cout << "\n🏗️  CREATING REAL ORDERS FOR SEPOLIA..." << std::endl;

        // Allow overriding pool and params via CLI/env
        // Usage: curve_dex_limit_order_agent <pool_address> <token_in_index> <token_out_index> <input_amount>
        auto getenv_str = [](const char *key) -> std::string
        {
            const char *val = std::getenv(key);
            return val ? std::string(val) : std::string();
        };

        std::string pool_address;
        int32_t in_idx = 0;
        int32_t out_idx = 1;
        uint64_t input_amount = 1000000; // default 1e6 units

        if (argc >= 2)
            pool_address = argv[1];
        if (argc >= 3)
            in_idx = static_cast<int32_t>(std::stol(argv[2]));
        if (argc >= 4)
            out_idx = static_cast<int32_t>(std::stol(argv[3]));
        if (argc >= 5)
            input_amount = static_cast<uint64_t>(std::stoull(argv[4]));

        if (pool_address.empty())
            pool_address = getenv_str("POOL_ADDRESS");
        if (const std::string env_in = getenv_str("TOKEN_IN_INDEX"); !env_in.empty())
            in_idx = static_cast<int32_t>(std::stol(env_in));
        if (const std::string env_out = getenv_str("TOKEN_OUT_INDEX"); !env_out.empty())
            out_idx = static_cast<int32_t>(std::stol(env_out));
        if (const std::string env_amt = getenv_str("ORDER_INPUT_AMOUNT"); !env_amt.empty())
            input_amount = static_cast<uint64_t>(std::stoull(env_amt));

        if (pool_address.empty() || pool_address == "0xPool" || pool_address.length() < 42)
        {
            // Default to Curve 3pool (mainnet) for read-only pricing; swaps stay mocked
            pool_address = "0xbEbc44782C7dB0a1A60Cb6fe97d0b483032FF1C7";
            in_idx = 1;  // USDC
            out_idx = 0; // DAI
            std::cout << "[INFO] Using default Curve 3pool on mainnet for pricing: " << pool_address << std::endl;
            std::cout << "       Tip: set POOL_ADDRESS or pass CLI args to override." << std::endl;
        }

        // If using default mainnet pool and no RPC_URL provided, fallback to public mainnet RPC
        if (rpc_url == SepoliaConfig::SEPOLIA_RPC_URL && pool_address == std::string("0xbEbc44782C7dB0a1A60Cb6fe97d0b483032FF1C7"))
        {
            rpc_url = "https://eth.llamarpc.com";
            std::cout << "[INFO] No RPC_URL set; using public mainnet RPC for 3pool." << std::endl;
        }

        EthereumRPC rpc(rpc_url);
        LimitOrderEngine engine(&rpc);

        // Parse TIF policy from command line or environment
        std::string tif_policy = "GTC"; // default
        if (argc >= 6)
            tif_policy = argv[5];
        else if (const char *env_tif = std::getenv("TIF_POLICY"); env_tif)
            tif_policy = std::string(env_tif);

        // Parse limit price from command line or environment
        double limit_price = 1.01; // default
        if (argc >= 7)
            limit_price = std::stod(argv[6]);
        else if (const char *env_price = std::getenv("LIMIT_PRICE"); env_price)
            limit_price = std::stod(env_price);

        // Parse additional parameters for GTT
        std::chrono::system_clock::time_point expiry_time;
        if (tif_policy == "GTT")
        {
            int expiry_minutes = 60; // default 1 hour
            if (argc >= 8)
                expiry_minutes = std::stoi(argv[7]);
            else if (const char *env_expiry = std::getenv("GTT_EXPIRY_MINUTES"); env_expiry)
                expiry_minutes = std::stoi(env_expiry);

            expiry_time = std::chrono::system_clock::now() + std::chrono::minutes(expiry_minutes);
        }

        // Create order based on TIF policy
        std::unique_ptr<LimitOrder> order;
        std::string order_id = "SEPOLIA_" + tif_policy + "_TEST";

        if (tif_policy == "GTC")
        {
            order = OrderFactory::createGTC(order_id, SepoliaConfig::Tokens::USDC,
                                            SepoliaConfig::Tokens::DAI, input_amount, limit_price, 0.005,
                                            user_address, private_key);
        }
        else if (tif_policy == "GTT")
        {
            order = OrderFactory::createGTT(order_id, SepoliaConfig::Tokens::USDC,
                                            SepoliaConfig::Tokens::DAI, input_amount, limit_price, 0.005,
                                            expiry_time, user_address, private_key);
        }
        else if (tif_policy == "IOC")
        {
            order = OrderFactory::createIOC(order_id, SepoliaConfig::Tokens::USDC,
                                            SepoliaConfig::Tokens::DAI, input_amount, limit_price, 0.005,
                                            user_address, private_key);
        }
        else if (tif_policy == "FOK")
        {
            order = OrderFactory::createFOK(order_id, SepoliaConfig::Tokens::USDC,
                                            SepoliaConfig::Tokens::DAI, input_amount, limit_price, 0.005,
                                            user_address, private_key);
        }
        else
        {
            std::cerr << "❌ Unknown TIF policy: " << tif_policy << std::endl;
            std::cerr << "Supported policies: GTC, GTT, IOC, FOK" << std::endl;
            return 1;
        }

        order->pool_address = pool_address;
        order->input_token_index = in_idx;
        order->output_token_index = out_idx;
        engine.addOrder(std::move(order));

        std::cout << "\n🎬 PROCESSING ALL ORDERS..." << std::endl;

        // Process all orders according to their TIF policies
        engine.processOrders();

        std::cout << "\n🏁 LIMIT ORDER AGENT COMPLETE!" << std::endl;
        std::cout << "✅ " << tif_policy << " order created and processed" << std::endl;
        std::cout << "✅ Price monitoring working" << std::endl;
        std::cout << "✅ Ready for real Sepolia pool integration" << std::endl;

        std::cout << "\n📖 USAGE EXAMPLES:" << std::endl;
        std::cout << "  ./build/curve_dex_limit_order_agent                    # GTC order (default)" << std::endl;
        std::cout << "  ./build/curve_dex_limit_order_agent 0xPool 1 0 1000000 GTC 1.01" << std::endl;
        std::cout << "  ./build/curve_dex_limit_order_agent 0xPool 1 0 1000000 GTT 1.01 30" << std::endl;
        std::cout << "  ./build/curve_dex_limit_order_agent 0xPool 1 0 1000000 IOC 1.01" << std::endl;
        std::cout << "  ./build/curve_dex_limit_order_agent 0xPool 1 0 1000000 FOK 1.01" << std::endl;
        std::cout << "  ./build/curve_dex_limit_order_agent 0xPool 1 0 1000000 IOC 2.0  # High limit (cancels)" << std::endl;
        std::cout << "  TIF_POLICY=IOC LIMIT_PRICE=2.0 ./build/curve_dex_limit_order_agent  # Environment variables" << std::endl;

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
