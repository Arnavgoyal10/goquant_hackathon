#ifndef LIMIT_ORDER_H
#define LIMIT_ORDER_H

#include <string>
#include <chrono>
#include <memory>
#include <iostream>
#include <ctime>

// Time-in-Force policy enumeration
enum class TimeInForce
{
    GTC, // Good-Till-Canceled: Order remains active until filled or manually canceled
    GTT, // Good-Till-Time: Order expires at a specific timestamp
    IOC, // Immediate-Or-Cancel: Execute immediately, cancel any unfilled portion
    FOK  // Fill-Or-Kill: Execute entire order immediately or cancel completely
};

// Order status enumeration
enum class OrderStatus
{
    PENDING,          // Order created but not yet monitored
    ACTIVE,           // Order is being monitored for execution
    PARTIALLY_FILLED, // Part of the order has been executed (IOC only)
    FILLED,           // Order completely executed
    CANCELED,         // Order canceled (manually or by policy)
    EXPIRED,          // Order expired (GTT only)
    FAILED            // Order failed due to error
};

// Limit Order structure - core data for all order types
struct LimitOrder
{
    // Basic order identification
    std::string order_id;
    std::chrono::system_clock::time_point created_at;

    // Token pair and amounts
    std::string input_token_address;
    std::string output_token_address;
    uint64_t input_amount;      // Amount of input token to swap
    uint64_t min_output_amount; // Minimum acceptable output (calculated from limit price)

    // Pool information
    std::string pool_address;
    int32_t input_token_index;  // Token index in the Curve pool (e.g., 0, 1)
    int32_t output_token_index; // Token index in the Curve pool

    // Price and slippage settings
    double limit_price;        // Target exchange rate (output/input)
    double slippage_tolerance; // Maximum acceptable slippage (e.g., 0.005 for 0.5%)

    // Time-in-Force policy
    TimeInForce tif_policy;
    std::chrono::system_clock::time_point expiry_time; // Used for GTT orders

    // Execution settings
    std::string user_address; // Address to receive output tokens
    std::string private_key;  // Private key for signing transactions (TODO: secure storage)

    // Order state
    OrderStatus status;
    uint64_t filled_amount;       // Amount of input token that has been filled
    uint64_t received_amount;     // Amount of output token received
    std::string transaction_hash; // Hash of execution transaction (if any)
    std::string failure_reason;   // Reason for failure/cancellation

    // Monitoring data
    std::chrono::system_clock::time_point last_price_check;
    uint64_t last_quoted_output; // Last get_dy result
    int price_check_count;       // Number of price checks performed

    // Constructor for creating new limit orders
    LimitOrder(const std::string &id,
               const std::string &input_token,
               const std::string &output_token,
               uint64_t input_amt,
               double limit_rate,
               double slippage,
               TimeInForce tif,
               const std::string &user_addr,
               const std::string &priv_key)
        : order_id(id),
          created_at(std::chrono::system_clock::now()),
          input_token_address(input_token),
          output_token_address(output_token),
          input_amount(input_amt),
          limit_price(limit_rate),
          slippage_tolerance(slippage),
          tif_policy(tif),
          user_address(user_addr),
          private_key(priv_key),
          status(OrderStatus::PENDING),
          filled_amount(0),
          received_amount(0),
          last_quoted_output(0),
          price_check_count(0)
    {

        // Calculate minimum output based on limit price
        min_output_amount = static_cast<uint64_t>(input_amount * limit_price);

        // Set expiry time for GTT orders (default to 1 hour if not specified)
        if (tif_policy == TimeInForce::GTT)
        {
            expiry_time = created_at + std::chrono::hours(1);
        }
    }

    // Check if order has expired (for GTT orders)
    bool isExpired() const
    {
        if (tif_policy != TimeInForce::GTT)
            return false;
        return std::chrono::system_clock::now() >= expiry_time;
    }

    // Check if order can still be executed
    bool isExecutable() const
    {
        return status == OrderStatus::ACTIVE && !isExpired();
    }

    // Calculate current fill percentage
    double getFillPercentage() const
    {
        if (input_amount == 0)
            return 0.0;
        return static_cast<double>(filled_amount) / static_cast<double>(input_amount) * 100.0;
    }

    // Check if price meets limit order criteria
    bool isPriceMet(uint64_t current_output) const
    {
        if (input_amount == 0)
            return false;
        double current_rate = static_cast<double>(current_output) / static_cast<double>(input_amount);
        return current_rate >= limit_price;
    }

    // Check if price meets limit order criteria for a specific amount
    bool isPriceMetForAmount(uint64_t current_output, uint64_t check_amount) const
    {
        if (check_amount == 0)
            return false;
        double current_rate = static_cast<double>(current_output) / static_cast<double>(check_amount);
        return current_rate >= limit_price;
    }

    // Calculate maximum fillable amount at current price (for partial fills)
    uint64_t getMaxFillableAmount(uint64_t current_output) const
    {
        if (input_amount == 0)
            return 0;
        
        // Calculate how much input we can swap at the current rate
        uint64_t remaining_amount = input_amount - filled_amount;
        if (remaining_amount == 0)
            return 0;
            
        // If current rate meets limit, we can fill the remaining amount
        if (isPriceMet(current_output))
            return remaining_amount;
            
        return 0;
    }

    // Calculate minimum acceptable output considering slippage
    uint64_t getMinOutputWithSlippage(uint64_t current_market_output) const
    {
        return static_cast<uint64_t>(current_market_output * (1.0 - slippage_tolerance));
    }

    // Update order status
    void updateStatus(OrderStatus new_status, const std::string &reason = "")
    {
        status = new_status;
        if (!reason.empty())
        {
            failure_reason = reason;
        }
    }

    // Record a price check
    void recordPriceCheck(uint64_t quoted_output)
    {
        last_price_check = std::chrono::system_clock::now();
        last_quoted_output = quoted_output;
        price_check_count++;
    }

    // Set expiry time for GTT orders
    void setExpiryTime(const std::chrono::system_clock::time_point &expiry)
    {
        if (tif_policy == TimeInForce::GTT)
        {
            expiry_time = expiry;
        }
    }

    // Convert TIF enum to string for display
    std::string getTifString() const
    {
        switch (tif_policy)
        {
        case TimeInForce::GTC:
            return "GTC";
        case TimeInForce::GTT:
            return "GTT";
        case TimeInForce::IOC:
            return "IOC";
        case TimeInForce::FOK:
            return "FOK";
        default:
            return "UNKNOWN";
        }
    }

    // Convert status enum to string for display
    std::string getStatusString() const
    {
        switch (status)
        {
        case OrderStatus::PENDING:
            return "PENDING";
        case OrderStatus::ACTIVE:
            return "ACTIVE";
        case OrderStatus::PARTIALLY_FILLED:
            return "PARTIALLY_FILLED";
        case OrderStatus::FILLED:
            return "FILLED";
        case OrderStatus::CANCELED:
            return "CANCELED";
        case OrderStatus::EXPIRED:
            return "EXPIRED";
        case OrderStatus::FAILED:
            return "FAILED";
        default:
            return "UNKNOWN";
        }
    }

    // Print order summary
    void printSummary() const
    {
        std::cout << "\n=== Order Summary ===" << std::endl;
        std::cout << "ID: " << order_id << std::endl;
        std::cout << "Status: " << getStatusString() << std::endl;
        std::cout << "TIF: " << getTifString() << std::endl;
        std::cout << "Input: " << input_amount << " tokens" << std::endl;
        std::cout << "Limit Price: " << limit_price << std::endl;
        std::cout << "Slippage: " << (slippage_tolerance * 100) << "%" << std::endl;
        std::cout << "Filled: " << getFillPercentage() << "%" << std::endl;
        std::cout << "Price Checks: " << price_check_count << std::endl;

        if (tif_policy == TimeInForce::GTT)
        {
            auto time_t = std::chrono::system_clock::to_time_t(expiry_time);
            std::cout << "Expires: " << std::ctime(&time_t);
        }

        if (!transaction_hash.empty())
        {
            std::cout << "Transaction: " << transaction_hash << std::endl;
        }

        if (!failure_reason.empty())
        {
            std::cout << "Reason: " << failure_reason << std::endl;
        }
    }
};

// Helper function to create orders with different TIF policies
namespace OrderFactory
{

    // Create GTC order
    std::unique_ptr<LimitOrder> createGTC(
        const std::string &id,
        const std::string &input_token,
        const std::string &output_token,
        uint64_t input_amount,
        double limit_price,
        double slippage,
        const std::string &user_address,
        const std::string &private_key)
    {

        return std::make_unique<LimitOrder>(
            id, input_token, output_token, input_amount,
            limit_price, slippage, TimeInForce::GTC,
            user_address, private_key);
    }

    // Create GTT order with custom expiry
    std::unique_ptr<LimitOrder> createGTT(
        const std::string &id,
        const std::string &input_token,
        const std::string &output_token,
        uint64_t input_amount,
        double limit_price,
        double slippage,
        const std::chrono::system_clock::time_point &expiry,
        const std::string &user_address,
        const std::string &private_key)
    {

        auto order = std::make_unique<LimitOrder>(
            id, input_token, output_token, input_amount,
            limit_price, slippage, TimeInForce::GTT,
            user_address, private_key);
        order->setExpiryTime(expiry);
        return order;
    }

    // Create IOC order
    std::unique_ptr<LimitOrder> createIOC(
        const std::string &id,
        const std::string &input_token,
        const std::string &output_token,
        uint64_t input_amount,
        double limit_price,
        double slippage,
        const std::string &user_address,
        const std::string &private_key)
    {

        return std::make_unique<LimitOrder>(
            id, input_token, output_token, input_amount,
            limit_price, slippage, TimeInForce::IOC,
            user_address, private_key);
    }

    // Create FOK order
    std::unique_ptr<LimitOrder> createFOK(
        const std::string &id,
        const std::string &input_token,
        const std::string &output_token,
        uint64_t input_amount,
        double limit_price,
        double slippage,
        const std::string &user_address,
        const std::string &private_key)
    {

        return std::make_unique<LimitOrder>(
            id, input_token, output_token, input_amount,
            limit_price, slippage, TimeInForce::FOK,
            user_address, private_key);
    }
}

#endif // LIMIT_ORDER_H
