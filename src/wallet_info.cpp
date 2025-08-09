#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include "../include/sepolia_config.h"

using json = nlohmann::json;

static std::string strip0x(const std::string &hex)
{
    if (hex.rfind("0x", 0) == 0 || hex.rfind("0X", 0) == 0)
        return hex.substr(2);
    return hex;
}

static uint64_t hexToUint64(const std::string &hex)
{
    std::string clean = strip0x(hex);
    if (clean.empty())
        return 0;
    if (clean.length() > 16)
        clean = clean.substr(clean.length() - 16);
    try
    {
        return std::stoull(clean, nullptr, 16);
    }
    catch (...)
    {
        return 0;
    }
}

class EthereumRPC
{
    std::string rpc_url;
    CURL *curl;
    static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *response)
    {
        size_t totalSize = size * nmemb;
        response->append((char *)contents, totalSize);
        return totalSize;
    }

public:
    explicit EthereumRPC(const std::string &url) : rpc_url(url)
    {
        curl = curl_easy_init();
        if (!curl)
            throw std::runtime_error("Failed to initialize CURL");
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
            throw std::runtime_error("CURL request failed: " + std::string(curl_easy_strerror(res)));
        return json::parse(response);
    }
};

static std::string encodeAddress(const std::string &address)
{
    std::string clean = strip0x(address);
    return std::string(24, '0') + clean;
}

static uint64_t getEthBalance(EthereumRPC &rpc, const std::string &address)
{
    auto resp = rpc.call("eth_getBalance", json::array({address, "latest"}));
    return resp.contains("result") ? hexToUint64(resp["result"]) : 0;
}

static uint64_t getErc20Balance(EthereumRPC &rpc, const std::string &token, const std::string &owner)
{
    // balanceOf(address) -> 0x70a08231
    std::string data = std::string("0x70a08231") + encodeAddress(owner);
    json params = json::array({json({{"to", token}, {"data", data}}), "latest"});
    auto resp = rpc.call("eth_call", params);
    return resp.contains("result") ? hexToUint64(resp["result"]) : 0;
}

int main(int argc, char **argv)
{
    try
    {
        curl_global_init(CURL_GLOBAL_DEFAULT);

        // Resolve RPC URL (allow override)
        std::string rpc_url = SepoliaConfig::SEPOLIA_RPC_URL;
        if (const char *env = std::getenv("RPC_URL"); env && std::string(env).size() > 0)
            rpc_url = env;

        EthereumRPC rpc(rpc_url);

        // Resolve wallet address (CLI arg > env > config)
        std::string address;
        if (argc >= 2)
            address = argv[1];
        if (address.empty())
            if (const char *env = std::getenv("WALLET_ADDRESS"))
                address = env;
        if (address.empty())
            address = SepoliaConfig::Wallet::ADDRESS;

        std::cout << "\n=== Wallet Info ===" << std::endl;
        std::cout << "RPC: " << rpc_url << std::endl;
        std::cout << "Address: " << address << std::endl;

        // ETH balance (wei)
        uint64_t eth_balance_wei = getEthBalance(rpc, address);
        std::cout << "ETH (wei): " << eth_balance_wei << std::endl;

        // Token balances
        auto weth = SepoliaConfig::Tokens::WETH;
        auto usdc = SepoliaConfig::Tokens::USDC;
        auto dai = SepoliaConfig::Tokens::DAI;

        uint64_t bal_weth = getErc20Balance(rpc, weth, address);
        uint64_t bal_usdc = getErc20Balance(rpc, usdc, address);
        uint64_t bal_dai = getErc20Balance(rpc, dai, address);

        std::cout << "WETH balance (raw): " << bal_weth << std::endl;
        std::cout << "USDC balance (raw): " << bal_usdc << std::endl;
        std::cout << "DAI balance (raw):  " << bal_dai << std::endl;

        curl_global_cleanup();
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        curl_global_cleanup();
        return 1;
    }
}
