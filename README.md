# 🎯 Curve DEX Limit Order Agent - Hackathon Challenge

**Objective:** Build a sophisticated off-chain trading agent that executes programmatic trades on the Curve DEX with precise execution controls, including slippage protection and multiple Time-in-Force (TIF) policies.

## 🚀 Challenge Overview

This hackathon challenge requires you to extend a basic C++ swap example to create a sophisticated trading agent that:

1. **Monitors on-chain prices** by querying Curve liquidity pools directly
2. **Implements limit order execution** with TIF policies (GTC, GTT, IOC, FOK)
3. **Provides slippage protection** to ensure trades execute within acceptable price ranges
4. **Manages transaction execution** on the Ethereum blockchain via Sepolia testnet

## 🏗️ Architecture

The system is divided into three main components:

### Part 1: Price Monitoring (`src/price_monitor.cpp`)
- Connects to Sepolia testnet via JSON-RPC
- Fetches real-time prices from Curve pools using `get_dy` function
- Provides continuous price feed for trading decisions

### Part 2: Limit Order Agent (`src/curve_dex_limit_order_agent.cpp`)
- Implements all TIF policies (GTC, GTT, IOC, FOK)
- Manages order lifecycle and execution logic
- Handles slippage protection and price validation
- **NEW:** Supports IOC partial fills and FOK liquidity verification
- Executes trades when conditions are met

### Part 3: Testing Suite (`tests/`)
- Unit tests for core functionality
- End-to-end tests for complete trading flow
- Validation of TIF policies and slippage protection

## 🔧 Setup Instructions

### Prerequisites
- C++17 compatible compiler (GCC 7+, Clang 5+)
- libcurl development libraries
- nlohmann/json library
- macOS/Linux environment

### 1. Install Dependencies

#### macOS (using Homebrew):
```bash
brew install curl nlohmann-json
```

#### Ubuntu/Debian:
```bash
sudo apt-get install libcurl4-openssl-dev nlohmann-json3-dev
```

### 2. Wallet Setup

**⚠️ CRITICAL SECURITY STEP:** Create a NEW MetaMask wallet specifically for this hackathon!

1. **Create New Wallet:**
   - Open MetaMask
   - Click "Create Account"
   - Name it "Hackathon Testnet"
   - **NEVER use your main wallet or production wallet**

2. **Switch to Sepolia Testnet:**
   - In MetaMask, click the network dropdown
   - Select "Sepolia test network"
   - If not visible, add custom network:
     - Network Name: Sepolia
     - RPC URL: `https://sepolia.infura.io/v3/YOUR_PROJECT_ID`
     - Chain ID: 11155111
     - Currency Symbol: ETH

3. **Get Testnet ETH:**
   - Visit [Sepolia Faucet](https://sepoliafaucet.com)
   - Or [Infura Faucet](https://infura.io/faucet/sepolia)
   - Request Sepolia ETH for gas fees

4. **Get RPC Endpoint:**
   - Sign up for [Alchemy](https://alchemy.com) (recommended)
   - Or [Infura](https://infura.io)
   - Create a new project for Sepolia testnet
   - Copy your project ID/API key

### 3. Configure the System

Run the automated setup script:
```bash
./setup_wallet.sh
```

The script will:
- Validate your wallet address and private key
- Configure your RPC endpoint
- Update the configuration files
- Test compilation

**Manual Configuration Alternative:**
If you prefer to configure manually, edit `include/sepolia_config.h`:
```cpp
// Replace these values with your actual credentials
const std::string SEPOLIA_RPC_URL = "https://eth-sepolia.g.alchemy.com/v2/YOUR_PROJECT_ID";
const std::string PRIVATE_KEY = "0xYOUR_PRIVATE_KEY_HERE";
const std::string ADDRESS = "0xYOUR_WALLET_ADDRESS_HERE";
```

## 🚀 Usage

### Compile the System
```bash
# Compile main limit order agent
make main

# Compile price monitor
make price_monitor

# Compile all components
make verify
```

### Run Price Monitoring (Part 1)
```bash
make price_monitor
./build/price_monitor
```

This will:
- Connect to Sepolia testnet
- Query a Curve pool for real-time prices
- Display price history and statistics

**Notes:**
- By default, the monitor uses Curve 3pool on mainnet (read-only) and a public RPC if `RPC_URL` is not set.
- Override pool or RPC:
  - `POOL_ADDRESS=0x... TOKEN_IN_INDEX=1 TOKEN_OUT_INDEX=0 TEST_AMOUNT=1000000 RPC_URL=https://eth.llamarpc.com ./build/price_monitor`

### Run the Limit Order Agent (Part 2)

**Basic Usage:**
```bash
make main
./build/curve_dex_limit_order_agent
```

**Advanced Usage with CLI Arguments:**
```bash
# GTC order (default)
./build/curve_dex_limit_order_agent

# GTC order with custom parameters
./build/curve_dex_limit_order_agent 0xPoolAddress 1 0 1000000 GTC

# GTT order with 30-minute expiry
./build/curve_dex_limit_order_agent 0xPoolAddress 1 0 1000000 GTT 30

# IOC order (supports partial fills)
./build/curve_dex_limit_order_agent 0xPoolAddress 1 0 1000000 IOC

# FOK order (with liquidity verification)
./build/curve_dex_limit_order_agent 0xPoolAddress 1 0 1000000 FOK

# Using environment variables
TIF_POLICY=IOC ./build/curve_dex_limit_order_agent
```

**CLI Arguments:**
```
./build/curve_dex_limit_order_agent [pool_address] [token_in_index] [token_out_index] [input_amount] [tif_policy] [expiry_minutes]
```

**Environment Variables:**
- `TIF_POLICY`: GTC, GTT, IOC, or FOK
- `GTT_EXPIRY_MINUTES`: Expiry time for GTT orders (default: 60)
- `SKIP_LIQUIDITY_CHECK`: Set to "1" to skip FOK liquidity verification
- `EXECUTE_ONCHAIN`: Set to "1" to enable real transaction signing
- `BROADCAST_TX`: Set to "1" to broadcast transactions to network

**Notes:**
- Prices are live via `get_dy`; swap execution is mocked by default.
- To sign locally without broadcasting: `EXECUTE_ONCHAIN=1 ./build/curve_dex_limit_order_agent`
- To attempt broadcasting (experimental): `EXECUTE_ONCHAIN=1 BROADCAST_TX=1 RPC_URL=... ./build/curve_dex_limit_order_agent`

### Run Tests (Part 3)
```bash
# Run unit tests
make unit_tests

# Run end-to-end tests
make e2e_tests

# Run all tests
make test_all
```

### Wallet Info (Balances)
```bash
make wallet_info
# Or specify a different address/RPC
RPC_URL=https://eth-sepolia.g.alchemy.com/v2/YOUR_KEY ./build/wallet_info 0xYourSepoliaAddress
```

Shows ETH (wei) and ERC-20 balances (WETH/USDC/DAI) for the configured network.

## 📊 TIF Policies Explained

### GTC (Good-Till-Canceled)
- Order remains active indefinitely until filled or manually canceled
- Continuously monitors price and executes when limit price is met
- Best for long-term trading strategies

### GTT (Good-Till-Time)
- Order expires at a specified timestamp
- Useful for time-sensitive trading strategies
- Automatically cancels if not filled by expiry

### IOC (Immediate-Or-Cancel)
- **NEW:** Supports partial fills for any executable portion
- Executes immediately for any fillable amount
- Cancels unfilled remainder
- Good for partial fills in volatile markets

### FOK (Fill-Or-Kill)
- **NEW:** Includes optional liquidity verification
- Entire order must be fillable immediately
- No partial fills allowed
- Verifies pool can handle full order size
- Best for precise execution requirements

## 🛡️ Slippage Protection

The system implements slippage protection by:
1. **Calculating minimum output** based on limit price
2. **Adding slippage tolerance** (e.g., 0.5%)
3. **Validating execution** against minimum acceptable output
4. **Failing gracefully** if slippage exceeds tolerance

## 🔍 Pool Discovery

**Important:** Curve may not have full deployment on Sepolia testnet. You may need to:

1. **Discover actual pools** by querying the registry
2. **Use alternative testnets** (Goerli, Mumbai)
3. **Deploy test pools** locally for development

Run pool discovery:
```bash
make discover_pools
./build/discover_pools
```

## 🧪 Testing Strategy

### Unit Tests
- Price calculation logic
- TIF policy validation
- Slippage calculations
- Order lifecycle management
- **NEW:** Partial fill logic
- **NEW:** Liquidity verification

### End-to-End Tests
- Complete trading flow
- Price monitoring → Order creation → Execution
- Multiple TIF policy scenarios
- Error handling and edge cases
- **NEW:** IOC partial fill scenarios
- **NEW:** FOK liquidity verification

## 🔐 Security Best Practices

### Why Use a Dedicated Testnet Wallet?

**🚨 CRITICAL:** This project requires you to create a NEW MetaMask wallet specifically for testing. Here's why this is essential:

**Risks of Using Your Mainnet Wallet:**
- **Private Key Exposure:** Even on testnet, private keys could be accidentally logged or exposed
- **Human Error:** You might accidentally use the same wallet on mainnet later
- **Malicious Code:** Testnet projects may contain experimental or insecure code
- **Key Management:** Mixing test and production keys creates security confusion

**Benefits of a Dedicated Testnet Wallet:**
- **Isolation:** Complete separation from your real assets
- **Learning:** Safe environment to experiment with DeFi protocols
- **Confidence:** You can test without fear of losing real funds
- **Best Practice:** Mirrors professional development workflows

**Security Checklist:**
- ✅ **Use dedicated testnet wallet** (never mainnet)
- ✅ **Get ETH from official faucets only**
- ✅ **Never share private keys**
- ✅ **Test thoroughly before mainnet**
- ✅ **Implement proper error handling**
- ✅ **Use environment variables for sensitive data**

## 🆘 Troubleshooting

### Common Issues

1. **Compilation Errors:**
   ```bash
   make clean
   make main
   ```

2. **RPC Connection Issues:**
   - Verify your RPC endpoint
   - Check network connectivity
   - Ensure project ID is correct

3. **Wallet Issues:**
   - Verify Sepolia testnet is selected
   - Ensure sufficient ETH for gas
   - Check private key format

4. **Pool Discovery Issues:**
   - Curve may not be fully deployed on Sepolia
   - Consider using alternative testnets
   - Check pool addresses in configuration

### Getting Help

- Check the test output for error messages
- Verify your configuration in `include/sepolia_config.h`
- Ensure all dependencies are properly installed
- Test with simple price monitoring first

## 📝 Submission Requirements

Your hackathon submission must include:

1. **Video Demonstration (5-10 minutes):**
   - Walk through your C++ source code
   - Explain TIF policies and slippage protection
   - **NEW:** Demonstrate IOC partial fills and FOK liquidity checks
   - Run live execution on Sepolia testnet
   - Demonstrate GTC and IOC order types

2. **Complete Codebase:**
   - All source files and headers
   - Working compilation and execution
   - Proper error handling and logging
   - **NEW:** Enhanced TIF policy support

3. **README.md (this file):**
   - Clear setup instructions
   - RPC provider used
   - Wallet security measures taken
   - Sepolia wallet address used

## 🎉 Success Criteria

Your hackathon submission is successful when:

1. ✅ **Price monitoring works** on Sepolia testnet
2. ✅ **Limit orders execute** with proper TIF policies
3. ✅ **Slippage protection** prevents unfavorable trades
4. ✅ **All tests pass** (unit and e2e)
5. ✅ **Live demonstration** shows working system
6. ✅ **Security practices** are properly implemented
7. ✅ **NEW:** IOC partial fills work correctly
8. ✅ **NEW:** FOK liquidity verification is functional

## 🚀 Next Steps

After completing the hackathon:

1. **Extend functionality** with additional TIF policies
2. **Implement advanced slippage** strategies
3. **Add multi-pool support** for arbitrage
4. **Deploy to mainnet** (with proper security audit)
5. **Build web interface** for order management

## 📊 Project Structure

```
hackathon/
├── src/
│   ├── curve_dex_limit_order_agent.cpp  # Main limit order engine
│   ├── price_monitor.cpp                # Real-time price monitoring
│   ├── discover_pools.cpp               # Pool discovery tool
│   └── wallet_info.cpp                  # Balance checking
├── include/
│   ├── limit_order.h                    # Order structure and TIF policies
│   ├── sepolia_config.h                 # Network configuration
│   ├── transaction_signer.h             # Transaction signing
│   └── wallet_generator.h               # Wallet utilities
├── tests/
│   ├── unit_tests.cpp                   # Unit test suite
│   └── e2e_tests.cpp                    # End-to-end tests
├── docs/
│   └── WALLET_SETUP.md                  # Wallet setup guide
├── Makefile                             # Build system
├── setup_wallet.sh                      # Automated setup script
├── curve_swap_example.cpp               # Original starter code
└── README.md                            # This file
```
