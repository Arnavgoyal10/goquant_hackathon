#!/bin/bash

echo "🎯 Curve DEX Limit Order Agent - Wallet Setup"
echo "=============================================="
echo ""

echo "This script will help you set up your wallet for the hackathon challenge."
echo ""

# Check if we're in the right directory
if [ ! -f "include/sepolia_config.h" ]; then
    echo "❌ Error: Please run this script from the hackathon project root directory"
    exit 1
fi

echo "📋 Prerequisites:"
echo "1. Create a new MetaMask wallet (never use your main wallet!)"
echo "2. Switch to Sepolia testnet"
echo "3. Get Sepolia ETH from a faucet"
echo "4. Have your RPC endpoint ready (Alchemy, Infura, etc.)"
echo ""

read -p "Do you have a new MetaMask wallet ready? (y/n): " wallet_ready

if [ "$wallet_ready" != "y" ]; then
    echo ""
    echo "🔐 Please create a new MetaMask wallet first:"
    echo "1. Open MetaMask"
    echo "2. Click 'Create Account'"
    echo "3. Name it 'Hackathon Testnet'"
    echo "4. Switch to Sepolia testnet"
    echo "5. Run this script again"
    echo ""
    exit 1
fi

echo ""
echo "📝 Wallet Configuration"
echo "======================"

# Get wallet address
read -p "Enter your wallet address (0x...): " wallet_address

# Validate address format
if [[ ! $wallet_address =~ ^0x[a-fA-F0-9]{40}$ ]]; then
    echo "❌ Invalid wallet address format. Must be 0x followed by 40 hex characters."
    exit 1
fi

# Get private key
echo ""
echo "⚠️  SECURITY WARNING:"
echo "- Only use this wallet for the hackathon testnet"
echo "- Never share your private key"
echo "- This is for educational purposes only"
echo ""

read -p "Enter your private key (0x...): " private_key

# Validate private key format
if [[ ! $private_key =~ ^0x[a-fA-F0-9]{64}$ ]]; then
    echo "❌ Invalid private key format. Must be 0x followed by 64 hex characters."
    exit 1
fi

# Get RPC endpoint
echo ""
echo "🌐 RPC Configuration"
echo "==================="
echo "Choose your RPC provider:"
echo "1. Alchemy (recommended)"
echo "2. Infura"
echo "3. Public endpoint"
echo "4. Custom URL"
echo ""

read -p "Enter your choice (1-4): " rpc_choice

case $rpc_choice in
    1)
        read -p "Enter your Alchemy project ID: " project_id
        rpc_url="https://eth-sepolia.g.alchemy.com/v2/$project_id"
        ;;
    2)
        read -p "Enter your Infura project ID: " project_id
        rpc_url="https://sepolia.infura.io/v3/$project_id"
        ;;
    3)
        rpc_url="https://rpc.sepolia.org"
        echo "⚠️  Using public endpoint. This may be slower and less reliable."
        ;;
    4)
        read -p "Enter your custom RPC URL: " rpc_url
        ;;
    *)
        echo "❌ Invalid choice. Using public endpoint."
        rpc_url="https://rpc.sepolia.org"
        ;;
esac

echo ""
echo "🔧 Updating configuration files..."
echo ""

# Check if configuration already exists
if grep -q "YOUR_RPC_URL_HERE\|YOUR_PRIVATE_KEY_HERE\|YOUR_WALLET_ADDRESS_HERE" include/sepolia_config.h; then
    echo "📝 Updating placeholder configuration..."
else
    echo "⚠️  Configuration already contains real values."
    echo "   This will overwrite your current configuration."
    read -p "Do you want to continue? (y/n): " overwrite_confirm
    if [ "$overwrite_confirm" != "y" ]; then
        echo "❌ Configuration update cancelled."
        exit 1
    fi
    echo "📝 Overwriting existing configuration..."
fi

# Update sepolia_config.h with proper replacements
sed -i.bak "s|YOUR_RPC_URL_HERE|$rpc_url|g" include/sepolia_config.h
sed -i.bak "s|YOUR_PRIVATE_KEY_HERE|$private_key|g" include/sepolia_config.h
sed -i.bak "s|YOUR_WALLET_ADDRESS_HERE|$wallet_address|g" include/sepolia_config.h

# Remove backup files
rm -f include/sepolia_config.h.bak

echo "✅ Configuration updated in include/sepolia_config.h"

echo "✅ Configuration updated successfully!"
echo ""

echo "📊 Next Steps:"
echo "1. Get Sepolia ETH from a faucet:"
echo "   - https://sepoliafaucet.com (Alchemy)"
echo "   - https://infura.io/faucet/sepolia"
echo ""
echo "2. Test your connection:"
echo "   make price_monitor"
echo ""
echo "3. Run the main agent:"
echo "   make main"
echo "   ./build/curve_dex_limit_order_agent"
echo ""

echo "🎉 Wallet setup complete! You're ready for the hackathon challenge."
echo ""

# Test compilation (optional)
echo "🧪 Testing compilation..."
if make clean > /dev/null 2>&1 && make price_monitor > /dev/null 2>&1; then
    echo "✅ Compilation successful!"
else
    echo "⚠️  Compilation test skipped. You can test manually with:"
    echo "   make main"
    echo "   ./build/curve_dex_limit_order_agent"
fi

echo ""
echo "🚀 You're all set! Good luck with the hackathon!"
