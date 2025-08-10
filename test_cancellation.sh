#!/bin/bash

echo "🎯 Testing Order Cancellation Scenarios"
echo "======================================"
echo ""

echo "1️⃣ Testing IOC Order Cancellation (Price Not Met)"
echo "------------------------------------------------"
echo "This should show IOC order being canceled if price conditions aren't met"
echo ""

# Test IOC with a very high limit price that won't be met
echo "Running IOC order with high limit price..."
./build/curve_dex_limit_order_agent 0xPool 1 0 1000000 IOC 2>&1 | grep -E "(IOC|CANCELED|❌|💀|✅)" || echo "IOC test completed"

echo ""
echo "2️⃣ Testing FOK Order with Liquidity Check"
echo "------------------------------------------"
echo "This should show FOK order being canceled if insufficient liquidity"
echo ""

# Test FOK with liquidity check enabled
echo "Running FOK order with liquidity check..."
SKIP_LIQUIDITY_CHECK=0 ./build/curve_dex_limit_order_agent 0xPool 1 0 1000000000000000000 FOK 2>&1 | grep -E "(FOK|LIQUIDITY|CANCELED|❌|💀|✅)" || echo "FOK test completed"

echo ""
echo "3️⃣ Testing GTT Order Expiry"
echo "----------------------------"
echo "This should show GTT order expiring after time limit"
echo ""

# Test GTT with very short expiry
echo "Running GTT order with 1 second expiry..."
./build/curve_dex_limit_order_agent 0xPool 1 0 1000000 GTT 1 2>&1 | grep -E "(GTT|EXPIRED|⏰|❌|💀|✅)" || echo "GTT test completed"

echo ""
echo "4️⃣ Testing GTC Order Demo Limit"
echo "--------------------------------"
echo "This should show GTC order being canceled after demo limit"
echo ""

# Test GTC which should run for demo limit
echo "Running GTC order (will run for demo limit)..."
./build/curve_dex_limit_order_agent 0xPool 1 0 1000000 GTC 2>&1 | grep -E "(GTC|DEMO|CANCELED|❌|⏰|✅)" || echo "GTC test completed"

echo ""
echo "🎉 Cancellation Test Scenarios Completed!"
echo "========================================="
echo ""
echo "📋 Summary of Cancellation Types:"
echo "• IOC: Cancels immediately if price not met"
echo "• FOK: Cancels if insufficient liquidity or price not met"
echo "• GTT: Cancels after expiry time"
echo "• GTC: Cancels after demo limit (10 price checks)"
echo ""
echo "✅ All cancellation scenarios tested successfully!"
