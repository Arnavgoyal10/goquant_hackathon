# 🔐 Minimal Testnet Wallet Setup

Use a dedicated testnet wallet so you can safely run balance checks and (optionally) broadcast demo transactions.

## 1) Create a new MetaMask wallet
- Install MetaMask from `https://metamask.io`
- Create a new wallet (do not use a personal/production wallet)
- Store the seed phrase offline

## 2) Add Sepolia testnet
Use any provider (Alchemy/Infura):
```
Network Name: Sepolia Testnet
RPC URL: https://sepolia.infura.io/v3/YOUR_KEY (or Alchemy)
Chain ID: 11155111
Currency Symbol: SepoliaETH
Block Explorer: https://sepolia.etherscan.io
```

## 3) Fund with faucet ETH
- `https://sepoliafaucet.com` (Alchemy) or `https://infura.io/faucet/sepolia`

## 4) Configure this project
Option A: Use the setup script (recommended)
```bash
./setup_wallet.sh
```

Option B: Manual config (edit `include/sepolia_config.h`)
- Set `SEPOLIA_RPC_URL`, `Wallet::ADDRESS`, and (optionally) `Wallet::PRIVATE_KEY` for demo signing

## 5) Sanity check balances
```bash
make wallet_info
# Or explicitly set RPC/Address
RPC_URL=https://eth-sepolia.g.alchemy.com/v2/YOUR_KEY ./build/wallet_info 0xYourSepoliaAddress
```

## 6) Optional: Attempt broadcast (demo signer)
Swaps are mocked by default. To sign or broadcast on testnet:
```bash
# Sign only (no broadcast)
EXECUTE_ONCHAIN=1 ./build/curve_dex_limit_order_agent

# Try broadcasting (requires valid RPC & key configured in sepolia_config.h)
EXECUTE_ONCHAIN=1 BROADCAST_TX=1 RPC_URL=https://eth-sepolia.g.alchemy.com/v2/YOUR_KEY ./build/curve_dex_limit_order_agent
```

That’s it. Keep the wallet testnet-only for safety.
