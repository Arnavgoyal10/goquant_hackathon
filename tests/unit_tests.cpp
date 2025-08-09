#include "../include/limit_order.h"
#include "../include/transaction_signer.h"
#include <iostream>
#include <cassert>
#include <vector>
#include <memory>
#include <chrono>

// Simple test framework
class TestFramework
{
private:
    int tests_run = 0;
    int tests_passed = 0;

public:
    void run_test(const std::string &test_name, bool result)
    {
        tests_run++;
        if (result)
        {
            tests_passed++;
            std::cout << "✅ " << test_name << " PASSED" << std::endl;
        }
        else
        {
            std::cout << "❌ " << test_name << " FAILED" << std::endl;
        }
    }

    template <typename T>
    void assert_equal(const std::string &test_name, T expected, T actual)
    {
        run_test(test_name, expected == actual);
    }

    void assert_true(const std::string &test_name, bool condition)
    {
        run_test(test_name, condition);
    }

    void assert_false(const std::string &test_name, bool condition)
    {
        run_test(test_name, !condition);
    }

    void print_summary()
    {
        std::cout << "\n📊 TEST SUMMARY" << std::endl;
        std::cout << "Tests Run: " << tests_run << std::endl;
        std::cout << "Tests Passed: " << tests_passed << std::endl;
        std::cout << "Tests Failed: " << (tests_run - tests_passed) << std::endl;
        std::cout << "Success Rate: " << (100.0 * tests_passed / tests_run) << "%" << std::endl;

        if (tests_passed == tests_run)
        {
            std::cout << "🎉 ALL TESTS PASSED!" << std::endl;
        }
    }
};

// Test limit order creation and basic functionality
void test_limit_order_creation(TestFramework &tf)
{
    std::cout << "\n🧪 Testing Limit Order Creation" << std::endl;

    // Test GTC order creation
    auto gtc_order = OrderFactory::createGTC(
        "TEST_GTC",
        "0xToken1",
        "0xToken2",
        1000000,
        1.05,
        0.01,
        "0xUser",
        "private_key");

    tf.assert_equal("GTC Order ID", std::string("TEST_GTC"), gtc_order->order_id);
    tf.assert_equal("GTC TIF Policy", TimeInForce::GTC, gtc_order->tif_policy);
    tf.assert_equal("GTC Input Amount", static_cast<uint64_t>(1000000), gtc_order->input_amount);
    tf.assert_equal("GTC Limit Price", 1.05, gtc_order->limit_price);
    tf.assert_equal("GTC Status", OrderStatus::PENDING, gtc_order->status);

    // Test calculated minimum output
    uint64_t expected_min = static_cast<uint64_t>(1000000 * 1.05);
    tf.assert_equal("GTC Min Output Calculation", expected_min, gtc_order->min_output_amount);
}

void test_price_validation(TestFramework &tf)
{
    std::cout << "\n🧪 Testing Price Validation Logic" << std::endl;

    auto order = OrderFactory::createGTC("PRICE_TEST", "0xA", "0xB", 1000000, 1.02, 0.01, "0xUser", "key");

    // Test price below limit
    uint64_t low_output = 1010000; // Rate: 1.01 (below 1.02 limit)
    tf.assert_true("Price Below Limit", !order->isPriceMet(low_output));

    // Test price at exact limit
    uint64_t exact_output = 1020000; // Rate: 1.02 (exactly at limit)
    tf.assert_true("Price At Limit", order->isPriceMet(exact_output));

    // Test price above limit
    uint64_t high_output = 1050000; // Rate: 1.05 (above 1.02 limit)
    tf.assert_true("Price Above Limit", order->isPriceMet(high_output));
}

void test_slippage_calculation(TestFramework &tf)
{
    std::cout << "\n🧪 Testing Slippage Protection" << std::endl;

    auto order = OrderFactory::createGTC("SLIP_TEST", "0xA", "0xB", 1000000, 1.00, 0.02, "0xUser", "key");

    uint64_t market_output = 1000000; // 1.00 rate
    uint64_t min_with_slippage = order->getMinOutputWithSlippage(market_output);

    // With 2% slippage tolerance, minimum should be 98% of market output
    uint64_t expected_min = static_cast<uint64_t>(market_output * 0.98);
    tf.assert_equal("Slippage Calculation", expected_min, min_with_slippage);
}

void test_order_expiry(TestFramework &tf)
{
    std::cout << "\n🧪 Testing Order Expiry Logic" << std::endl;

    // Test GTT order expiry
    auto future_time = std::chrono::system_clock::now() + std::chrono::seconds(1);
    auto past_time = std::chrono::system_clock::now() - std::chrono::seconds(1);

    auto gtt_future = OrderFactory::createGTT("GTT_FUTURE", "0xA", "0xB", 1000, 1.0, 0.01,
                                              future_time, "0xUser", "key");
    auto gtt_past = OrderFactory::createGTT("GTT_PAST", "0xA", "0xB", 1000, 1.0, 0.01,
                                            past_time, "0xUser", "key");

    tf.assert_true("Future GTT Not Expired", !gtt_future->isExpired());
    tf.assert_true("Past GTT Is Expired", gtt_past->isExpired());

    // Test non-GTT orders never expire
    auto gtc_order = OrderFactory::createGTC("GTC_TEST", "0xA", "0xB", 1000, 1.0, 0.01, "0xUser", "key");
    tf.assert_true("GTC Never Expires", !gtc_order->isExpired());
}

void test_order_status_transitions(TestFramework &tf)
{
    std::cout << "\n🧪 Testing Order Status Transitions" << std::endl;

    auto order = OrderFactory::createIOC("STATUS_TEST", "0xA", "0xB", 1000, 1.0, 0.01, "0xUser", "key");

    // Test initial status
    tf.assert_equal("Initial Status", OrderStatus::PENDING, order->status);

    // Test status updates
    order->updateStatus(OrderStatus::ACTIVE);
    tf.assert_equal("Updated to Active", OrderStatus::ACTIVE, order->status);

    order->updateStatus(OrderStatus::FILLED, "Order executed successfully");
    tf.assert_equal("Updated to Filled", OrderStatus::FILLED, order->status);
    tf.assert_equal("Failure Reason Set", std::string("Order executed successfully"), order->failure_reason);
}

void test_partial_fills(TestFramework &tf)
{
    std::cout << "\n🧪 Testing Partial Fill Logic" << std::endl;

    auto order = OrderFactory::createIOC("PARTIAL_TEST", "0xA", "0xB", 1000000, 1.0, 0.01, "0xUser", "key");

    // Test 0% fill
    tf.assert_equal("0% Fill", 0.0, order->getFillPercentage());

    // Test 50% fill
    order->filled_amount = 500000;
    tf.assert_equal("50% Fill", 50.0, order->getFillPercentage());

    // Test 100% fill
    order->filled_amount = 1000000;
    tf.assert_equal("100% Fill", 100.0, order->getFillPercentage());
}

void test_tif_policy_differences(TestFramework &tf)
{
    std::cout << "\n🧪 Testing TIF Policy Differences" << std::endl;

    auto gtc = OrderFactory::createGTC("GTC", "0xA", "0xB", 1000, 1.0, 0.01, "0xUser", "key");
    auto gtt = OrderFactory::createGTT("GTT", "0xA", "0xB", 1000, 1.0, 0.01,
                                       std::chrono::system_clock::now() + std::chrono::hours(1),
                                       "0xUser", "key");
    auto ioc = OrderFactory::createIOC("IOC", "0xA", "0xB", 1000, 1.0, 0.01, "0xUser", "key");
    auto fok = OrderFactory::createFOK("FOK", "0xA", "0xB", 1000, 1.0, 0.01, "0xUser", "key");

    tf.assert_equal("GTC TIF String", std::string("GTC"), gtc->getTifString());
    tf.assert_equal("GTT TIF String", std::string("GTT"), gtt->getTifString());
    tf.assert_equal("IOC TIF String", std::string("IOC"), ioc->getTifString());
    tf.assert_equal("FOK TIF String", std::string("FOK"), fok->getTifString());
}

void test_transaction_signing(TestFramework &tf)
{
    std::cout << "\n🧪 Testing Transaction Signing" << std::endl;

    TransactionSigner signer("test_private_key_123");

    // Test basic transaction creation
    EthereumTransaction tx;
    tx.to_address = "0x1234567890123456789012345678901234567890";
    tx.data = "0xa9059cbb000000000000000000000000";
    tx.nonce = 42;

    std::string signed_tx = signer.signTransaction(tx);

    tf.assert_true("Signed TX Not Empty", !signed_tx.empty());
    tf.assert_true("Signed TX Has Reasonable Length", signed_tx.length() > 100);

    // Test nonce retrieval
    uint64_t nonce = signer.getCurrentNonce("0xUser");
    tf.assert_equal("Mock Nonce", static_cast<uint64_t>(42), nonce);
}

void test_price_check_recording(TestFramework &tf)
{
    std::cout << "\n🧪 Testing Price Check Recording" << std::endl;

    auto order = OrderFactory::createGTC("PRICE_RECORD_TEST", "0xA", "0xB", 1000, 1.0, 0.01, "0xUser", "key");

    // Initial state
    tf.assert_equal("Initial Price Checks", 0, order->price_check_count);
    tf.assert_equal("Initial Last Output", static_cast<uint64_t>(0), order->last_quoted_output);

    // Record first price check
    order->recordPriceCheck(1050000);
    tf.assert_equal("First Price Check Count", 1, order->price_check_count);
    tf.assert_equal("First Quoted Output", static_cast<uint64_t>(1050000), order->last_quoted_output);

    // Record second price check
    order->recordPriceCheck(1020000);
    tf.assert_equal("Second Price Check Count", 2, order->price_check_count);
    tf.assert_equal("Second Quoted Output", static_cast<uint64_t>(1020000), order->last_quoted_output);
}

void test_partial_fill_logic(TestFramework &tf)
{
    std::cout << "\n🧪 Testing Partial Fill Logic" << std::endl;

    auto order = OrderFactory::createIOC("PARTIAL_FILL_TEST", "0xA", "0xB", 1000000, 1.02, 0.01, "0xUser", "key");

    // Test price check for specific amount
    uint64_t output_for_full = 1020000; // 1.02 rate - meets limit
    uint64_t output_for_half = 510000;  // 1.02 rate for 500k input

    tf.assert_true("Price Met for Full Amount", order->isPriceMetForAmount(output_for_full, 1000000));
    tf.assert_true("Price Met for Half Amount", order->isPriceMetForAmount(output_for_half, 500000));
    tf.assert_false("Price Not Met for Lower Rate", order->isPriceMetForAmount(1000000, 1000000));

    // Test max fillable amount calculation
    uint64_t max_fillable = order->getMaxFillableAmount(output_for_full);
    tf.assert_equal("Max Fillable at Good Price", static_cast<uint64_t>(1000000), max_fillable);

    // Test after partial fill
    order->filled_amount = 500000;
    max_fillable = order->getMaxFillableAmount(output_for_full);
    tf.assert_equal("Max Fillable After Partial", static_cast<uint64_t>(500000), max_fillable);

    // Test when price doesn't meet limit
    max_fillable = order->getMaxFillableAmount(1000000); // 1.0 rate
    tf.assert_equal("Max Fillable at Bad Price", static_cast<uint64_t>(0), max_fillable);
}

int main()
{
    std::cout << "🧪 COMPREHENSIVE UNIT TEST SUITE" << std::endl;
    std::cout << "=================================" << std::endl;

    TestFramework tf;

    // Run all test suites
    test_limit_order_creation(tf);
    test_price_validation(tf);
    test_slippage_calculation(tf);
    test_order_expiry(tf);
    test_order_status_transitions(tf);
    test_partial_fills(tf);
    test_tif_policy_differences(tf);
    test_transaction_signing(tf);
    test_price_check_recording(tf);
    test_partial_fill_logic(tf);

    // Print final results
    tf.print_summary();

    return 0;
}
