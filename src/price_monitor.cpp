#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <iomanip>
#include "../include/sepolia_config.h"

using json = nlohmann::json;

// Include the basic utilities from our main file
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

// Simple RPC client
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

// Price data structure
struct PricePoint
{
    std::chrono::system_clock::time_point timestamp;
    uint64_t input_amount;
    uint64_t output_amount;
    double exchange_rate;

    PricePoint(uint64_t input, uint64_t output)
        : timestamp(std::chrono::system_clock::now()),
          input_amount(input),
          output_amount(output)
    {
        exchange_rate = input > 0 ? static_cast<double>(output) / static_cast<double>(input) : 0.0;
    }
};

// Price Monitor class - core of the price monitoring system
class PriceMonitor
{
private:
    EthereumRPC *rpc;
    std::string pool_address;
    int32_t token_in_index;
    int32_t token_out_index;
    uint64_t test_amount;
    std::vector<PricePoint> price_history;
    bool monitoring;

public:
    PriceMonitor(EthereumRPC *ethereum_rpc,
                 const std::string &pool_addr,
                 int32_t in_idx,
                 int32_t out_idx,
                 uint64_t amount)
        : rpc(ethereum_rpc), pool_address(pool_addr),
          token_in_index(in_idx), token_out_index(out_idx),
          test_amount(amount), monitoring(false) {}

    // Get current price using get_dy
    uint64_t getCurrentPrice()
    {
        // Function signature for get_dy(int128,int128,uint256) - 0x5e0d443f
        std::string function_signature = "0x5e0d443f";

        std::string encoded_i = encodeUint256(static_cast<uint64_t>(token_in_index));
        std::string encoded_j = encodeUint256(static_cast<uint64_t>(token_out_index));
        std::string encoded_dx = encodeUint256(test_amount);

        std::string call_data = function_signature + encoded_i + encoded_j + encoded_dx;

        json call_params = {{{"to", pool_address}, {"data", call_data}}, "latest"};

        auto result = rpc->call("eth_call", call_params);

        if (result.contains("error"))
        {
            throw std::runtime_error("RPC Error: " + result["error"]["message"].get<std::string>());
        }

        return hexToUint64(result["result"]);
    }

    // Add price point to history
    void recordPrice(uint64_t output_amount)
    {
        price_history.emplace_back(test_amount, output_amount);

        // Keep only last 100 price points
        if (price_history.size() > 100)
        {
            price_history.erase(price_history.begin());
        }
    }

    // Start monitoring prices in a loop
    void startMonitoring(int duration_seconds = 60, int poll_interval_ms = 1000)
    {
        std::cout << "\n=== Starting Price Monitoring ===" << std::endl;
        std::cout << "Pool: " << pool_address << std::endl;
        std::cout << "Monitoring " << token_in_index << " -> " << token_out_index << std::endl;
        std::cout << "Test amount: " << test_amount << std::endl;
        std::cout << "Duration: " << duration_seconds << " seconds" << std::endl;
        std::cout << "Poll interval: " << poll_interval_ms << " ms" << std::endl;

        monitoring = true;
        auto start_time = std::chrono::system_clock::now();
        auto end_time = start_time + std::chrono::seconds(duration_seconds);

        int poll_count = 0;
        uint64_t last_price = 0;

        while (monitoring && std::chrono::system_clock::now() < end_time)
        {
            try
            {
                uint64_t current_output = getCurrentPrice();
                recordPrice(current_output);

                poll_count++;

                // Calculate price change
                double price_change = 0.0;
                if (last_price > 0 && current_output > 0)
                {
                    price_change = ((static_cast<double>(current_output) - static_cast<double>(last_price)) / static_cast<double>(last_price)) * 100.0;
                }

                // Print current price info
                auto now = std::chrono::system_clock::now();
                auto time_t = std::chrono::system_clock::to_time_t(now);

                std::cout << "[" << std::put_time(std::localtime(&time_t), "%H:%M:%S") << "] "
                          << "Poll #" << poll_count << " | "
                          << "Input: " << test_amount << " -> "
                          << "Output: " << current_output;

                if (last_price > 0)
                {
                    std::cout << " | Change: " << std::fixed << std::setprecision(4) << price_change << "%";
                }

                std::cout << std::endl;

                last_price = current_output;

                // Sleep for the specified interval
                std::this_thread::sleep_for(std::chrono::milliseconds(poll_interval_ms));
            }
            catch (const std::exception &e)
            {
                std::cerr << "Price monitoring error: " << e.what() << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(poll_interval_ms));
            }
        }

        monitoring = false;
        std::cout << "\n=== Price Monitoring Complete ===" << std::endl;
        std::cout << "Total polls: " << poll_count << std::endl;
        std::cout << "Price history size: " << price_history.size() << std::endl;
    }

    // Stop monitoring
    void stopMonitoring()
    {
        monitoring = false;
    }

    // Get price statistics
    void printPriceStats()
    {
        if (price_history.empty())
        {
            std::cout << "No price data available" << std::endl;
            return;
        }

        std::cout << "\n=== Price Statistics ===" << std::endl;

        uint64_t min_output = price_history[0].output_amount;
        uint64_t max_output = price_history[0].output_amount;
        double sum_rates = 0.0;

        for (const auto &point : price_history)
        {
            if (point.output_amount < min_output)
                min_output = point.output_amount;
            if (point.output_amount > max_output)
                max_output = point.output_amount;
            sum_rates += point.exchange_rate;
        }

        double avg_rate = sum_rates / price_history.size();
        double min_rate = static_cast<double>(min_output) / static_cast<double>(test_amount);
        double max_rate = static_cast<double>(max_output) / static_cast<double>(test_amount);

        std::cout << "Data points: " << price_history.size() << std::endl;
        std::cout << "Min output: " << min_output << " (rate: " << min_rate << ")" << std::endl;
        std::cout << "Max output: " << max_output << " (rate: " << max_rate << ")" << std::endl;
        std::cout << "Avg rate: " << avg_rate << std::endl;
        std::cout << "Price volatility: " << ((max_rate - min_rate) / avg_rate * 100.0) << "%" << std::endl;
    }

    // Check if current price meets a target
    bool isPriceAboveTarget(double target_rate)
    {
        if (price_history.empty())
            return false;
        return price_history.back().exchange_rate >= target_rate;
    }

    // Get latest price
    PricePoint getLatestPrice()
    {
        if (price_history.empty())
        {
            throw std::runtime_error("No price data available");
        }
        return price_history.back();
    }
};

// Test the price monitoring system
int main(int argc, char **argv)
{
    try
    {
        // Use configured Sepolia wallet
        if (!SepoliaConfig::isConfigured())
        {
            std::cerr << "❌ Configuration not complete. Please run ./setup_wallet.sh first." << std::endl;
            return 1;
        }

        curl_global_init(CURL_GLOBAL_DEFAULT);

        // Allow overriding via CLI args or env vars
        // Usage: price_monitor <pool_address> <token_in_index> <token_out_index> <amount>
        auto getenv_str = [](const char *key) -> std::string
        {
            const char *val = std::getenv(key);
            return val ? std::string(val) : std::string();
        };

        std::string pool_arg;
        int32_t in_idx = 0;
        int32_t out_idx = 1;
        uint64_t amount = 1000000; // 1e6 default units

        if (argc >= 2)
        {
            pool_arg = argv[1];
        }
        if (argc >= 3)
        {
            in_idx = static_cast<int32_t>(std::stol(argv[2]));
        }
        if (argc >= 4)
        {
            out_idx = static_cast<int32_t>(std::stol(argv[3]));
        }
        if (argc >= 5)
        {
            amount = static_cast<uint64_t>(std::stoull(argv[4]));
        }

        if (pool_arg.empty())
        {
            pool_arg = getenv_str("POOL_ADDRESS");
        }
        if (const std::string env_in = getenv_str("TOKEN_IN_INDEX"); !env_in.empty())
        {
            in_idx = static_cast<int32_t>(std::stol(env_in));
        }
        if (const std::string env_out = getenv_str("TOKEN_OUT_INDEX"); !env_out.empty())
        {
            out_idx = static_cast<int32_t>(std::stol(env_out));
        }
        if (const std::string env_amt = getenv_str("TEST_AMOUNT"); !env_amt.empty())
        {
            amount = static_cast<uint64_t>(std::stoull(env_amt));
        }

        if (pool_arg.empty())
        {
            // Default to Curve 3pool (mainnet) for read-only price monitoring
            pool_arg = "0xbEbc44782C7dB0a1A60Cb6fe97d0b483032FF1C7";
            // Default token indices USDC(1) -> DAI(0)
            in_idx = 1;
            out_idx = 0;
            std::cout << "[INFO] Using default Curve 3pool on mainnet: " << pool_arg << std::endl;
            std::cout << "       Tip: set POOL_ADDRESS or pass CLI args to override." << std::endl;
        }

        // Resolve RPC URL after pool selection
        std::string rpc_url = SepoliaConfig::SEPOLIA_RPC_URL;
        const char *rpc_env = std::getenv("RPC_URL");
        if (rpc_env && std::string(rpc_env).size() > 0)
        {
            rpc_url = std::string(rpc_env);
        }
        else
        {
            // If using mainnet 3pool by default and no RPC_URL provided, switch to a public mainnet RPC
            if (pool_arg == std::string("0xbEbc44782C7dB0a1A60Cb6fe97d0b483032FF1C7"))
            {
                rpc_url = "https://eth.llamarpc.com";
                std::cout << "[INFO] No RPC_URL set; using public mainnet RPC for 3pool." << std::endl;
            }
        }

        EthereumRPC rpc(rpc_url);

        // Create price monitor
        PriceMonitor monitor(&rpc, pool_arg, in_idx, out_idx, amount);

        std::cout << "=== Price Monitor Test ===" << std::endl;

        // Test single price fetch
        try
        {
            uint64_t price = monitor.getCurrentPrice();
            std::cout << "Single price fetch: " << price << std::endl;
        }
        catch (const std::exception &e)
        {
            std::cout << "Single price fetch failed (expected for demo pool): " << e.what() << std::endl;
        }

        // Demonstrate monitoring loop (shortened for demo)
        std::cout << "\nDemonstrating price monitoring loop..." << std::endl;
        std::cout << "Note: This will fail with demo pool address, but shows the monitoring structure" << std::endl;

        try
        {
            monitor.startMonitoring(10, 2000); // 10 seconds, poll every 2 seconds
            monitor.printPriceStats();
        }
        catch (const std::exception &e)
        {
            std::cout << "Monitoring failed (expected for demo): " << e.what() << std::endl;
        }

        std::cout << "\n✅ Price monitoring system structure complete!" << std::endl;
        std::cout << "Ready for integration with real Curve pools" << std::endl;

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
